use std::sync::{Arc, Mutex};
use serde::Serialize;
use cpal::Stream;
use cpal::traits::StreamTrait;
use crate::audio_engine::{AudioEngine, AudioDevice, DeviceConfig};
use tauri::Emitter;
use tauri::WebviewWindow;

// ============================================================================
// AUDIO DATA STRUCTURE
// ============================================================================

/// Audio data payload sent to frontend for waveform visualization
#[derive(Serialize, Clone)]
pub struct AudioData {
    pub sample_rate: u32,
    pub channels: u16,
    pub data: Vec<f32>,
    pub timestamp: u64,
}

// ============================================================================
// RECORDING STATE ENUM
// ============================================================================

#[derive(Debug, Clone, Copy, PartialEq, Serialize)]
pub enum RecordingState {
    Stopped,
    Recording,
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
    ///   → resample to 16kHz (if needed) → int16 PCM bytes → ws_sender channel
    ///
    /// Also emits `audio-data` events for waveform visualization in React.
    pub fn start_with_ws(
        &self,
        device_id: String,
        ws_sender: tokio::sync::mpsc::UnboundedSender<Vec<u8>>,
    ) -> Result<String, String> {
        use rubato::{
            Resampler, SincFixedIn, SincInterpolationParameters,
            SincInterpolationType, WindowFunction,
        };

        if self.is_recording() {
            return Err("Already recording".to_string());
        }

        let device_index: usize = device_id
            .parse()
            .map_err(|_| "Invalid device ID".to_string())?;

        let device_config = AudioEngine::get_device_config(device_index)
            .map_err(|e| format!("Failed to get device config: {e}"))?;

        let native_sr = device_config.sample_rate;
        let native_ch = device_config.channels as usize;
        let target_sr: u32 = 16000;

        // Buffer 5 seconds of mono f32 at native sample rate before flushing
        let buffer_cap = (native_sr as usize) * 5;
        let sample_buf: Arc<Mutex<Vec<f32>>> =
            Arc::new(Mutex::new(Vec::with_capacity(buffer_cap)));

        let buf_clone = sample_buf.clone();
        let ws_tx = ws_sender.clone();
        let window = self.window.clone();

        let (stream, config) = AudioEngine::build_stream(
            device_index,
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

                // --- Flush: resample if needed, convert to int16, send ---
                let chunk = buf.clone();
                buf.clear();
                drop(buf);

                let pcm_bytes = if native_sr == target_sr {
                    f32_mono_to_pcm16(&chunk)
                } else {
                    let ratio = target_sr as f64 / native_sr as f64;
                    let params = SincInterpolationParameters {
                        sinc_len: 256,
                        f_cutoff: 0.95,
                        interpolation: SincInterpolationType::Linear,
                        oversampling_factor: 256,
                        window: WindowFunction::BlackmanHarris2,
                    };
                    match SincFixedIn::<f32>::new(ratio, 2.0, params, chunk.len(), 1) {
                        Ok(mut resampler) => {
                            match resampler.process(&[chunk], None) {
                                Ok(out) => f32_mono_to_pcm16(&out[0]),
                                Err(e) => {
                                    eprintln!("Resample process error: {e}");
                                    return;
                                }
                            }
                        }
                        Err(e) => {
                            eprintln!("Resampler init error: {e}");
                            return;
                        }
                    }
                };

                if ws_tx.send(pcm_bytes).is_err() {
                    eprintln!("WS sender closed; stopping audio flush");
                }
            },
        )
        .map_err(|e| format!("Failed to build stream: {e}"))?;

        stream.play().map_err(|e| format!("Failed to start stream: {e}"))?;

        *self.stream.lock().unwrap() = Some(stream);
        *self.config.lock().unwrap() = Some(config);
        *self.state.lock().unwrap() = RecordingState::Recording;

        println!("Recording started (WS) from device {device_id} @ {native_sr}Hz");
        Ok(format!("Started capturing from device {device_id}"))
    }

    // ========================================================================
    // STOP
    // ========================================================================

    /// Stop capturing audio. Dropping the stream causes CPAL to stop the callback.
    pub fn stop(&self) -> Result<String, String> {
        if !self.is_recording() {
            return Err("Not currently recording".to_string());
        }

        *self.stream.lock().unwrap() = None;
        *self.config.lock().unwrap() = None;
        *self.state.lock().unwrap() = RecordingState::Stopped;

        println!("Recording stopped");
        Ok("Audio capture stopped".to_string())
    }
}