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

/// Audio data payload sent to frontend
/// Contains PCM samples and metadata
#[derive(Serialize, Clone)]
pub struct AudioData {
    /// Sample rate in Hz (e.g., 48000)
    pub sample_rate: u32,
    /// Number of channels (e.g., 1 for mono, 2 for stereo)
    pub channels: u16,
    /// Raw PCM samples as f32 (-1.0 to 1.0)
    pub data: Vec<f32>,
    /// Timestamp in milliseconds since UNIX epoch
    pub timestamp: u64,
}

// ============================================================================
// RECORDING STATE ENUM
// ============================================================================

/// Current recording state
#[derive(Debug, Clone, Copy, PartialEq, Serialize)]
pub enum RecordingState {
    /// Not recording, stream is stopped
    Stopped,
    /// Currently recording, stream is active
    Recording,
    /// Error occurred during recording
    Error,
}

// ============================================================================
// AUDIO CAPTURE STATE MANAGER
// ============================================================================

/// Manages audio capture lifecycle and state
///
/// This is the central state manager that:
/// 1. Owns the CPAL audio stream
/// 2. Tracks recording state
/// 3. Coordinates start/stop operations
/// 4. Emits audio events to frontend
/// 5. Delegates CPAL operations to AudioEngine
///
/// The stream MUST be kept alive in Arc<Mutex<>> to prevent it
/// from being dropped, which would stop audio capture.
pub struct AudioCapture {
    /// Active audio stream (Option allows None when not recording)
    /// CRITICAL: Must be wrapped in Arc<Mutex<>> to keep stream alive
    /// while it's in use and to allow thread-safe access
    stream: Arc<Mutex<Option<Stream>>>,

    /// Current recording state
    /// Tracks: Stopped, Recording, or Error
    state: Arc<Mutex<RecordingState>>,

    /// Device configuration of the current recording session
    /// Contains: sample_rate, channels, CPAL StreamConfig
    /// Only Some() while recording
    config: Arc<Mutex<Option<DeviceConfig>>>,

    /// Reference to Tauri window for emitting events
    /// Used to send audio-data events to frontend
    window: WebviewWindow,
}

// ============================================================================
// AUDIO CAPTURE IMPLEMENTATION
// ============================================================================

impl AudioCapture {
    /// Create a new AudioCapture manager
    ///
    /// # Arguments
    /// * `window` - Tauri window reference for emitting events
    ///
    /// # Returns
    /// AudioCapture initialized with no active stream
    ///
    /// # Example
    /// ```rust
    /// let audio_capture = AudioCapture::new(window);
    /// ```
    pub fn new(window: WebviewWindow) -> Self {
        Self {
            stream: Arc::new(Mutex::new(None)),
            state: Arc::new(Mutex::new(RecordingState::Stopped)),
            config: Arc::new(Mutex::new(None)),
            window,
        }
    }

    // ========================================================================
    // STATE QUERY METHODS
    // ========================================================================

    /// Check if currently recording
    ///
    /// # Returns
    /// true if RecordingState is Recording, false otherwise
    ///
    /// # Example
    /// ```rust
    /// if audio_capture.is_recording() {
    ///     println!("Recording in progress");
    /// }
    /// ```
    pub fn is_recording(&self) -> bool {
        let state = self.state.lock().unwrap();
        *state == RecordingState::Recording
    }

    /// Get current recording state
    ///
    /// # Returns
    /// Current RecordingState (Stopped, Recording, or Error)
    ///
    /// # Example
    /// ```rust
    /// match audio_capture.get_state() {
    ///     RecordingState::Recording => println!("Recording"),
    ///     RecordingState::Stopped => println!("Stopped"),
    ///     RecordingState::Error => println!("Error"),
    /// }
    /// ```
    pub fn get_state(&self) -> RecordingState {
        let state = self.state.lock().unwrap();
        *state
    }

    /// Get current device configuration
    ///
    /// # Returns
    /// Some(DeviceConfig) if recording, None otherwise
    /// DeviceConfig contains: sample_rate, channels, CPAL config
    ///
    /// # Example
    /// ```rust
    /// if let Some(config) = audio_capture.get_config() {
    ///     println!("Sample rate: {}", config.sample_rate);
    ///     println!("Channels: {}", config.channels);
    /// }
    /// ```
    pub fn get_config(&self) -> Option<DeviceConfig> {
        let config = self.config.lock().unwrap();
        config.clone()
    }

    // ========================================================================
    // DEVICE ENUMERATION
    // ========================================================================

    /// Get list of available microphones
    ///
    /// # Returns
    /// Result with Vec of AudioDevice on success
    /// - id: unique identifier (device index as string)
    /// - name: display name (from OS)
    /// - channels: number of channels
    ///
    /// # Example
    /// ```rust
    /// let devices = audio_capture.get_devices()?;
    /// for device in devices {
    ///     println!("{}: {} ({}ch)", device.id, device.name, device.channels);
    /// }
    /// ```
    pub fn get_devices(&self) -> Result<Vec<AudioDevice>, String> {
        // Delegate to AudioEngine (pure CPAL operations)
        AudioEngine::list_input_devices()
    }

    // ========================================================================
    // RECORDING START
    // ========================================================================

    /// Start capturing audio from a device
    ///
    /// # Arguments
    /// * `device_id` - Device ID string (from get_devices)
    ///
    /// # Returns
    /// Ok(message) on success
    /// Err(reason) if already recording or device not found
    ///
    /// # What it does
    /// 1. Checks if not already recording (prevents double-start)
    /// 2. Parses device_id to numeric index
    /// 3. Gets device configuration via AudioEngine
    /// 4. Builds CPAL stream with audio callback via AudioEngine
    /// 5. Callback processes samples and emits to frontend
    /// 6. Starts the stream (begins audio capture)
    /// 7. Stores stream in Arc<Mutex<>> to keep it alive
    /// 8. Updates state to Recording
    ///
    /// # Example
    /// ```rust
    /// audio_capture.start("0")?;
    /// println!("Recording started");
    /// ```
    pub fn start(&self, device_id: String) -> Result<String, String> {
        // ====================================================================
        // STEP 1: Validate not already recording
        // ====================================================================
        if self.is_recording() {
            return Err("Already recording".to_string());
        }

        // ====================================================================
        // STEP 2: Parse device ID to numeric index
        // ====================================================================
        let device_index: usize = device_id
            .parse()
            .map_err(|_| "Invalid device ID format".to_string())?;

        // ====================================================================
        // STEP 3: Get device configuration from AudioEngine
        // ====================================================================
        let device_config = AudioEngine::get_device_config(device_index)
            .map_err(|e| format!("Failed to get device config: {}", e))?;

        // Extract config for use in callback
        let window = self.window.clone();
        let sample_rate = device_config.sample_rate;
        let channels = device_config.channels;

        // ====================================================================
        // STEP 4: Build audio stream with callback
        // ====================================================================
        // The callback is called 100+ times per second with audio samples
        let (stream, config) = AudioEngine::build_stream(
            device_index,
            move |data: &[f32], _info: &cpal::InputCallbackInfo| {
                // ============================================================
                // AUDIO CALLBACK (runs in separate thread)
                // ============================================================
                // This closure is called whenever new audio data arrives
                // from the microphone

                // Take first 1024 samples for frontend visualization
                let chunk_size = 1024;
                let samples: Vec<f32> = data.iter()
                    .take(chunk_size)
                    .copied()
                    .collect();

                // Only emit if we have data
                if !samples.is_empty() {
                    // Create AudioData payload
                    let payload = AudioData {
                        sample_rate,
                        channels,
                        data: samples,
                        timestamp: std::time::SystemTime::now()
                            .duration_since(std::time::UNIX_EPOCH)
                            .unwrap()
                            .as_millis() as u64,
                    };

                    // Emit event to frontend
                    // This sends audio-data event that JS listens for
                    if let Err(e) = window.emit("audio-data", &payload) {
                        eprintln!("Failed to emit audio-data event: {}", e);
                    }
                }
            },
        )
        .map_err(|e| format!("Failed to build stream: {}", e))?;

        // ====================================================================
        // STEP 5: Start the stream
        // ====================================================================
        // This tells CPAL to begin capturing audio
        stream.play()
            .map_err(|e| format!("Failed to play stream: {}", e))?;

        // ====================================================================
        // STEP 6: Store stream in state to keep it alive
        // ====================================================================
        // CRITICAL: The stream MUST be kept alive, otherwise it will be
        // dropped and audio capture will immediately stop.
        // We store it in Arc<Mutex<Option<Stream>>> for thread safety.
        {
            let mut s = self.stream.lock().unwrap();
            *s = Some(stream);
        }

        // ====================================================================
        // STEP 7: Store device config
        // ====================================================================
        {
            let mut c = self.config.lock().unwrap();
            *c = Some(config);
        }

        // ====================================================================
        // STEP 8: Update state to Recording
        // ====================================================================
        {
            let mut st = self.state.lock().unwrap();
            *st = RecordingState::Recording;
        }

        println!("Recording started from device {}", device_id);
        Ok(format!("Successfully started capturing from device {}", device_id))
    }

    // ========================================================================
    // RECORDING STOP
    // ========================================================================

    /// Stop capturing audio
    ///
    /// # Returns
    /// Ok(message) on success
    /// Err(reason) if not currently recording
    ///
    /// # What it does
    /// 1. Checks if currently recording
    /// 2. Drops the stream from state
    /// 3. CPAL automatically stops audio capture
    /// 4. Callback stops being called
    /// 5. No more events emitted to frontend
    /// 6. Updates state to Stopped
    ///
    /// # Example
    /// ```rust
    /// audio_capture.stop()?;
    /// println!("Recording stopped");
    /// ```
    pub fn stop(&self) -> Result<String, String> {
        // ====================================================================
        // STEP 1: Verify we're recording
        // ====================================================================
        if !self.is_recording() {
            return Err("Not currently recording".to_string());
        }

        // ====================================================================
        // STEP 2: Drop the stream
        // ====================================================================
        // Setting to None drops the stream, which CPAL automatically stops.
        // Once dropped, the callback is never called again.
        {
            let mut s = self.stream.lock().unwrap();
            *s = None;
        }

        // ====================================================================
        // STEP 3: Clear device config
        // ====================================================================
        {
            let mut c = self.config.lock().unwrap();
            *c = None;
        }

        // ====================================================================
        // STEP 4: Update state to Stopped
        // ====================================================================
        {
            let mut st = self.state.lock().unwrap();
            *st = RecordingState::Stopped;
        }

        println!("Recording stopped");
        Ok("Audio capture stopped".to_string())
    }
}

// ============================================================================
// USAGE EXAMPLE
// ============================================================================
//
// In main.rs:
//
// let window = app.get_webview_window("main")?;
// let audio_capture = AudioCapture::new(window);
//
// From frontend:
//
// // Get devices
// const devices = await invoke('get_audio_devices');
// // → calls: audio_capture.get_devices()
//
// // Start recording
// await invoke('start_audio_capture', { deviceId: "0" });
// // → calls: audio_capture.start("0")
//
// // Listen for audio events
// listen('audio-data', (event) => {
//     const data = event.payload;  // AudioData struct
//     drawWaveform(data.data);
// });
//
// // Stop recording
// await invoke('stop_audio_capture');
// // → calls: audio_capture.stop()
//
// ============================================================================
// LIFECYCLE DIAGRAM
// ============================================================================
//
// Creation (new)
//   ↓
//   state = Stopped
//   stream = None
//   config = None
//   ↓
// Frontend calls start_audio_capture
//   ↓
// start(device_id)
//   1. Parse device_id: "0" → 0
//   2. Get config from AudioEngine
//   3. Build stream with callback
//   4. stream.play() starts capture
//   5. Store stream in Arc<Mutex<>>
//   6. state = Recording
//   7. Callback fires 100+ times/sec
//   ↓
// Frontend listens to 'audio-data' events
// Callback emits 1024 samples each time
//   ↓
// Frontend calls stop_audio_capture
//   ↓
// stop()
//   1. Check is_recording() = true
//   2. Drop stream (= None)
//   3. CPAL stops capture automatically
//   4. Callback stops firing
//   5. state = Stopped
//   ↓
// Can call start() again anytime
//
// ===========================================================================