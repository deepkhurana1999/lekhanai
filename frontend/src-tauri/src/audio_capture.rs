use std::sync::{Arc, Mutex};
use serde::Serialize;
use cpal::Stream;
use cpal::traits::StreamTrait;
use crate::audio_engine::{AudioEngine, AudioDevice, DeviceConfig};
use crate::system_audio::SystemAudioCapture;
use tauri::Emitter;
use tauri::WebviewWindow;

/// Fixed native sample rate `pw-record` captures system audio at (see system_audio.rs).
const SYSTEM_AUDIO_SAMPLE_RATE: u32 = 48000;

/// How much louder (as a multiplier on RMS) system audio must be than the mic
/// before we prefer it over the mic chunk.
///
/// This is deliberately well below 1.0: a `pw-record` loopback tap's absolute
/// RMS can be much quieter than an acoustic mic's ambient noise floor even at
/// full system volume (measured ~0.003-0.008 RMS for a PipeWire monitor vs.
/// a mic's typical ~0.001-0.01 ambient floor on real hardware) — these two
/// signals aren't on a comparable absolute scale, so a margin >1.0 (requiring
/// system audio to be literally louder than the mic) would almost never let
/// system audio win. 0.4 lets a present-but-quiet system signal beat mic
/// ambient noise, while actual mic speech (RMS typically 0.02+) still easily
/// outweighs it.
const SYSTEM_AUDIO_RMS_MARGIN: f32 = 0.4;

/// Minimum fraction of the expected (mic-duration-equivalent) sample count a
/// system-audio chunk must have before it's even considered as a candidate.
/// Guards against a just-restarted `pw-record` (e.g. right after resume)
/// producing a tiny sliver of samples that could otherwise win the RMS
/// comparison and wipe out a full mic chunk's worth of real speech.
const SYSTEM_AUDIO_MIN_FILL_RATIO: f32 = 0.8;

// ============================================================================
// RECORDING STATE ENUM
// ============================================================================

#[derive(Debug, Clone, Copy, PartialEq, Serialize)]
pub enum RecordingState {
    Stopped,
    Recording,
    Paused,
    Error,
}

// ============================================================================
// AUDIO CAPTURE STATE MANAGER
// ============================================================================

pub struct AudioCapture {
    /// Active CPAL audio stream — kept alive in Arc<Mutex<>>
    stream: Arc<Mutex<Option<Stream>>>,
    state: Arc<Mutex<RecordingState>>,
    config: Arc<Mutex<Option<DeviceConfig>>>,
    window: WebviewWindow,
    /// Active system-audio (loopback) capture, if enabled for this session.
    system_audio: Arc<Mutex<Option<SystemAudioCapture>>>,
    /// Whether system audio was requested for the current recording session.
    /// Tracked separately from `system_audio` being `Some` because the
    /// subprocess may fail to spawn (or get stopped on pause) while the
    /// session still wants it restarted on resume.
    include_system_audio: Arc<Mutex<bool>>,
}

// ============================================================================
// HELPERS
// ============================================================================

/// Convert f32 mono audio [-1, 1] to raw int16 PCM bytes (little-endian).
pub fn f32_mono_to_pcm16(samples: &[f32]) -> Vec<u8> {
    let mut out = Vec::with_capacity(samples.len() * 2);
    for &s in samples {
        let clamped = s.clamp(-1.0, 1.0);
        let i16_val = (clamped * 32767.0) as i16;
        out.extend_from_slice(&i16_val.to_le_bytes());
    }
    out
}

/// Root-mean-square amplitude of a mono f32 sample buffer. Used to compare
/// loudness between the mic chunk and the equivalent-duration system-audio
/// chunk so the flush step can pick whichever source actually has signal.
pub fn rms(samples: &[f32]) -> f32 {
    if samples.is_empty() {
        return 0.0;
    }
    let sum_sq: f32 = samples.iter().map(|s| s * s).sum();
    (sum_sq / samples.len() as f32).sqrt()
}

/// Prefix PCM bytes with a single source tag byte: `0x00` = mic, `0x01` = system audio.
fn tag_pcm_bytes(tag: u8, pcm_bytes: Vec<u8>) -> Vec<u8> {
    let mut tagged = Vec::with_capacity(pcm_bytes.len() + 1);
    tagged.push(tag);
    tagged.extend(pcm_bytes);
    tagged
}

/// Resample a mono f32 chunk from `native_sr` to `target_sr` (if needed) and
/// convert the result to int16 PCM bytes. Returns `None` on resampler failure
/// (already logged internally via eprintln before returning).
fn resample_and_pcm16(chunk: Vec<f32>, native_sr: u32, target_sr: u32) -> Option<Vec<u8>> {
    if native_sr == target_sr {
        return Some(f32_mono_to_pcm16(&chunk));
    }
    if chunk.is_empty() {
        return Some(Vec::new());
    }

    use rubato::{
        Resampler, SincFixedIn, SincInterpolationParameters,
        SincInterpolationType, WindowFunction,
    };

    let ratio = target_sr as f64 / native_sr as f64;
    let params = SincInterpolationParameters {
        sinc_len: 256,
        f_cutoff: 0.95,
        interpolation: SincInterpolationType::Linear,
        oversampling_factor: 256,
        window: WindowFunction::BlackmanHarris2,
    };
    let chunk_len = chunk.len();
    let mut resampler = match SincFixedIn::<f32>::new(ratio, 2.0, params, chunk_len, 1) {
        Ok(r) => r,
        Err(e) => {
            eprintln!("Resampler init error: {e}");
            return None;
        }
    };
    match resampler.process(&[chunk], None) {
        Ok(out) => Some(f32_mono_to_pcm16(&out[0])),
        Err(e) => {
            eprintln!("Resample process error: {e}");
            None
        }
    }
}

// ============================================================================
// IMPLEMENTATION
// ============================================================================

impl AudioCapture {
    pub fn new(window: WebviewWindow) -> Self {
        Self {
            stream: Arc::new(Mutex::new(None)),
            state: Arc::new(Mutex::new(RecordingState::Stopped)),
            config: Arc::new(Mutex::new(None)),
            window,
            system_audio: Arc::new(Mutex::new(None)),
            include_system_audio: Arc::new(Mutex::new(false)),
        }
    }

    pub fn is_recording(&self) -> bool {
        *self.state.lock().unwrap() == RecordingState::Recording
    }

    pub fn get_state(&self) -> RecordingState {
        *self.state.lock().unwrap()
    }

    pub fn get_config(&self) -> Option<DeviceConfig> {
        self.config.lock().unwrap().clone()
    }

    pub fn get_devices(&self) -> Result<Vec<AudioDevice>, String> {
        AudioEngine::list_input_devices()
    }

    // ========================================================================
    // REALTIME RECORDING (WebSocket mode)
    // ========================================================================

    /// Start capturing audio and stream PCM chunks to the Manager WebSocket.
    ///
    /// Audio pipeline per CPAL callback invocation:
    ///   native samples → mix to mono f32 → accumulate 5s buffer
    ///   → compare RMS against an equivalent system-audio chunk (if enabled)
    ///   → resample the chosen source to 16kHz (if needed) → int16 PCM bytes
    ///   → tag with source byte → ws_sender channel
    ///
    /// Also emits `audio-data` events for waveform visualization in React.
    pub fn start_with_ws(
        &self,
        device_id: String,
        ws_sender: tokio::sync::mpsc::UnboundedSender<Vec<u8>>,
        include_system_audio: bool,
    ) -> Result<String, String> {
        if self.is_recording() {
            return Err("Already recording".to_string());
        }

        let device_config = AudioEngine::get_device_config(&device_id)
            .map_err(|e| format!("Failed to get device config: {e}"))?;

        let native_sr = device_config.sample_rate;
        let native_ch = device_config.channels as usize;
        let target_sr: u32 = 16000;

        // Buffer 5 seconds of mono f32 at native sample rate before flushing
        let buffer_cap = (native_sr as usize) * 5;
        let sample_buf: Arc<Mutex<Vec<f32>>> =
            Arc::new(Mutex::new(Vec::with_capacity(buffer_cap)));

        *self.include_system_audio.lock().unwrap() = include_system_audio;

        let buf_clone = sample_buf.clone();
        let ws_tx = ws_sender.clone();
        let window = self.window.clone();
        let system_audio_clone = self.system_audio.clone();

        let (stream, config) = AudioEngine::build_stream(
            &device_id,
            move |data: &[f32], _info: &cpal::InputCallbackInfo| {
                // --- Mix down to mono ---
                let mono: Vec<f32> = if native_ch == 1 {
                    data.to_vec()
                } else {
                    data.chunks(native_ch)
                        .map(|ch| ch.iter().sum::<f32>() / native_ch as f32)
                        .collect()
                };

                // --- Emit waveform for visualization ---
                let _ = window.emit(
                    "audio-data",
                    &serde_json::json!({
                        "sample_rate": native_sr,
                        "channels": 1,
                        "data": &mono[..mono.len().min(512)],
                    }),
                );

                // --- Accumulate into buffer ---
                let mut buf = buf_clone.lock().unwrap();
                buf.extend_from_slice(&mono);

                if buf.len() < buffer_cap {
                    return;
                }

                // --- Flush: pick the louder of mic vs. system audio, resample, tag, send ---
                let mic_chunk = buf.clone();
                buf.clear();
                drop(buf);

                // Pull a chunk from system audio covering the same wall-clock
                // duration as mic_chunk, scaled for the two sources' sample
                // rates (mic_chunk's actual length can exceed buffer_cap
                // slightly since the flush only triggers after the cap is
                // reached, not exactly at it).
                let expected_sys_len =
                    mic_chunk.len() * SYSTEM_AUDIO_SAMPLE_RATE as usize / native_sr as usize;
                let sys_chunk: Option<Vec<f32>> = system_audio_clone
                    .lock()
                    .unwrap()
                    .as_ref()
                    .map(|sa| sa.take_buffer(expected_sys_len));

                let mic_rms = rms(&mic_chunk);
                let use_system_audio = match &sys_chunk {
                    // Only consider system audio once it's filled to at least
                    // SYSTEM_AUDIO_MIN_FILL_RATIO of the expected length —
                    // otherwise a just-restarted pw-record (e.g. right after
                    // resume) could offer a tiny, misleadingly loud sliver
                    // that wins the comparison and discards a full mic chunk.
                    Some(sys)
                        if sys.len() as f32
                            >= expected_sys_len as f32 * SYSTEM_AUDIO_MIN_FILL_RATIO =>
                    {
                        rms(sys) > mic_rms * SYSTEM_AUDIO_RMS_MARGIN
                    }
                    _ => false,
                };

                let (tag, pcm_bytes) = if use_system_audio {
                    // Safe: use_system_audio is only true when sys_chunk is Some(non-empty).
                    let sys = sys_chunk.unwrap();
                    match resample_and_pcm16(sys, SYSTEM_AUDIO_SAMPLE_RATE, target_sr) {
                        Some(bytes) => (0x01u8, bytes),
                        None => return,
                    }
                } else {
                    match resample_and_pcm16(mic_chunk, native_sr, target_sr) {
                        Some(bytes) => (0x00u8, bytes),
                        None => return,
                    }
                };

                if ws_tx.send(tag_pcm_bytes(tag, pcm_bytes)).is_err() {
                    eprintln!("WS sender closed; stopping audio flush");
                }
            },
        )
        .map_err(|e| format!("Failed to build stream: {e}"))?;

        stream.play().map_err(|e| format!("Failed to start stream: {e}"))?;

        *self.stream.lock().unwrap() = Some(stream);
        *self.config.lock().unwrap() = Some(config);
        *self.state.lock().unwrap() = RecordingState::Recording;

        if include_system_audio {
            let sys_audio = SystemAudioCapture::new();
            match sys_audio.start() {
                Ok(()) => {
                    *self.system_audio.lock().unwrap() = Some(sys_audio);
                    println!("System audio capture enabled for this session");
                }
                Err(e) => {
                    eprintln!(
                        "System audio capture failed to start, continuing mic-only: {e}"
                    );
                }
            }
        }

        println!("Recording started (WS) from device {device_id} @ {native_sr}Hz");
        Ok(format!("Started capturing from device {device_id}"))
    }

    // ========================================================================
    // PAUSE / RESUME
    // ========================================================================

    /// Pause capturing audio without dropping the stream or WS connection.
    /// CPAL stops invoking the callback entirely while paused, which also
    /// stops the WS `send()` inside it — no separate "skip while paused"
    /// check is needed. Note: any audio already sitting in the callback's
    /// `sample_buf` at the moment of pause is not flushed or cleared; it
    /// resumes accumulating on `.play()`, which can produce one slightly-off
    /// transcription chunk spanning the pause boundary. Acceptable tradeoff.
    pub fn pause(&self) -> Result<String, String> {
        if *self.state.lock().unwrap() != RecordingState::Recording {
            return Err("Not currently recording".to_string());
        }

        let stream_guard = self.stream.lock().unwrap();
        let stream = stream_guard.as_ref().ok_or("No active stream")?;
        stream.pause().map_err(|e| format!("Failed to pause stream: {e}"))?;
        drop(stream_guard);

        // Best-effort: SystemAudioCapture has no true pause, so we kill the
        // pw-record subprocess here and respawn it in resume(). Errors are
        // intentionally ignored — this is a nice-to-have, not a correctness
        // requirement, and the mic path pauses regardless.
        if let Some(sys_audio) = self.system_audio.lock().unwrap().take() {
            if let Err(e) = sys_audio.stop() {
                eprintln!("Failed to stop system audio during pause (ignored): {e}");
            }
        }

        *self.state.lock().unwrap() = RecordingState::Paused;

        println!("Recording paused");
        Ok("Audio capture paused".to_string())
    }

    /// Resume a paused recording on the same stream and WS connection.
    pub fn resume(&self) -> Result<String, String> {
        if *self.state.lock().unwrap() != RecordingState::Paused {
            return Err("Not currently paused".to_string());
        }

        let stream_guard = self.stream.lock().unwrap();
        let stream = stream_guard.as_ref().ok_or("No active stream")?;
        stream.play().map_err(|e| format!("Failed to resume stream: {e}"))?;
        drop(stream_guard);

        // Restart system audio if it was enabled for this session. pw-record
        // has no true pause/resume, so respawning the subprocess is the
        // documented approach from Task 1's plan.
        if *self.include_system_audio.lock().unwrap() {
            let sys_audio = SystemAudioCapture::new();
            match sys_audio.start() {
                Ok(()) => {
                    *self.system_audio.lock().unwrap() = Some(sys_audio);
                }
                Err(e) => {
                    eprintln!(
                        "System audio capture failed to restart on resume, continuing mic-only: {e}"
                    );
                }
            }
        }

        *self.state.lock().unwrap() = RecordingState::Recording;

        println!("Recording resumed");
        Ok("Audio capture resumed".to_string())
    }

    // ========================================================================
    // STOP
    // ========================================================================

    /// Stop capturing audio. Dropping the stream causes CPAL to stop the callback.
    /// Valid from both Recording and Paused states.
    pub fn stop(&self) -> Result<String, String> {
        let current_state = *self.state.lock().unwrap();
        if current_state != RecordingState::Recording && current_state != RecordingState::Paused {
            return Err("Not currently recording".to_string());
        }

        *self.stream.lock().unwrap() = None;
        *self.config.lock().unwrap() = None;

        if let Some(sys_audio) = self.system_audio.lock().unwrap().take() {
            if let Err(e) = sys_audio.stop() {
                eprintln!("Failed to stop system audio during stop (ignored): {e}");
            }
        }
        *self.include_system_audio.lock().unwrap() = false;

        *self.state.lock().unwrap() = RecordingState::Stopped;

        println!("Recording stopped");
        Ok("Audio capture stopped".to_string())
    }
}

// ============================================================================
// TESTS
// ============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_rms_of_silence_is_zero() {
        assert_eq!(rms(&[0.0, 0.0, 0.0]), 0.0);
    }

    #[test]
    fn test_rms_of_empty_is_zero() {
        assert_eq!(rms(&[]), 0.0);
    }

    #[test]
    fn test_rms_known_value() {
        // RMS of [1.0, -1.0, 1.0, -1.0] is 1.0
        assert_eq!(rms(&[1.0, -1.0, 1.0, -1.0]), 1.0);
    }

    #[test]
    fn test_tag_pcm_bytes_mic() {
        let tagged = tag_pcm_bytes(0x00, vec![1, 2, 3]);
        assert_eq!(tagged, vec![0x00, 1, 2, 3]);
    }

    #[test]
    fn test_tag_pcm_bytes_system_audio() {
        let tagged = tag_pcm_bytes(0x01, vec![4, 5]);
        assert_eq!(tagged, vec![0x01, 4, 5]);
    }

    #[test]
    fn test_resample_and_pcm16_same_rate_passthrough() {
        let samples = vec![0.5, -0.5, 0.25];
        let bytes = resample_and_pcm16(samples.clone(), 16000, 16000).unwrap();
        assert_eq!(bytes, f32_mono_to_pcm16(&samples));
    }

    #[test]
    fn test_resample_and_pcm16_empty_chunk_returns_empty() {
        let bytes = resample_and_pcm16(Vec::new(), 48000, 16000).unwrap();
        assert!(bytes.is_empty());
    }
}