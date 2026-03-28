mod audio_engine;
mod audio_capture;
use std::sync::{Arc, Mutex};
use tauri::State;
use tauri::Manager;
use tauri::Emitter;
use crate::audio_engine::AudioDevice;
use crate::audio_capture::AudioCapture;

pub struct AppState {
    /// The central audio capture state manager
    pub audio_capture: AudioCapture,
    /// Active WebSocket sender for forwarding audio to Manager
    /// Set during start_recording, cleared during stop_recording
    pub ws_sender: Arc<Mutex<Option<tokio::sync::mpsc::UnboundedSender<Vec<u8>>>>>,
}

// ============================================================================
// TAURI COMMANDS
// ============================================================================

#[tauri::command]
fn greet(name: &str) -> String {
    format!("Hello, {}! You've been greeted from Rust!", name)
}

#[tauri::command]
fn get_input_devices(
    state: State<'_, AppState>,
) -> Result<Vec<AudioDevice>, String> {
    state.audio_capture.get_devices()
}

/// Start recording from the selected microphone and forward audio to Manager WebSocket.
///
/// # Arguments
/// * `device_id`   - Device ID from get_input_devices (numeric string index)
/// * `session_id`  - Session ID created beforehand via POST /api/v1/session/create
/// * `manager_url` - Manager base URL, e.g. "ws://localhost:5000"
#[tauri::command]
async fn start_recording(
    device_id: String,
    session_id: String,
    manager_url: String,
    state: State<'_, AppState>,
    app: tauri::AppHandle,
) -> Result<String, String> {
    use tokio_tungstenite::connect_async;
    use tokio_tungstenite::tungstenite::Message;
    use futures_util::SinkExt;
    use futures_util::StreamExt;

    // Build WebSocket URL: ws://host:port/ws/transcribe/{session_id}
    let ws_url = format!("{}/ws/transcribe/{}", manager_url.trim_end_matches('/'), session_id);

    // Connect to Manager WebSocket
    let (ws_stream, _) = connect_async(&ws_url)
        .await
        .map_err(|e| format!("Failed to connect to Manager WebSocket {ws_url}: {e}"))?;

    let (mut ws_write, mut ws_read) = ws_stream.split();

    // Channel: audio thread → WebSocket sender task
    let (pcm_tx, mut pcm_rx) = tokio::sync::mpsc::unbounded_channel::<Vec<u8>>();

    // Store sender so stop_recording can close it
    {
        let mut sender_lock = state.ws_sender.lock().unwrap();
        *sender_lock = Some(pcm_tx.clone());
    }

    // Spawn task: drain pcm_rx → WebSocket binary frames
    let app_handle_send = app.clone();
    tokio::spawn(async move {
        while let Some(pcm_chunk) = pcm_rx.recv().await {
            if let Err(e) = ws_write.send(Message::Binary(pcm_chunk)).await {
                eprintln!("WS send error: {e}");
                break;
            }
        }
        // Flush and close
        let _ = ws_write.close().await;
    });

    // Spawn task: receive transcription messages → emit to React
    let app_handle_recv = app.clone();
    tokio::spawn(async move {
        while let Some(msg) = ws_read.next().await {
            match msg {
                Ok(Message::Text(text)) => {
                    if let Ok(json) = serde_json::from_str::<serde_json::Value>(&text) {
                        if json.get("type").and_then(|v| v.as_str()) == Some("transcription") {
                            let _ = app_handle_recv.emit("transcription", &json);
                        }
                    }
                }
                Ok(Message::Close(_)) => break,
                Err(e) => {
                    eprintln!("WS recv error: {e}");
                    break;
                }
                _ => {}
            }
        }
    });

    // Start CPAL mic capture; audio callback sends PCM chunks down pcm_tx
    state.audio_capture.start_with_ws(device_id, pcm_tx)
}

/// Stop the active recording session and close the Manager WebSocket.
#[tauri::command]
fn stop_recording(state: State<'_, AppState>) -> Result<String, String> {
    // Drop the WS sender — signals the send task to close the connection
    {
        let mut sender_lock = state.ws_sender.lock().unwrap();
        *sender_lock = None;
    }
    state.audio_capture.stop()
}

// ============================================================================
// TAURI APP SETUP
// ============================================================================

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .setup(|app| {
            let window = app
                .get_webview_window("main")
                .expect("No main window found");
            let audio_capture = AudioCapture::new(window);
            let app_state = AppState {
                audio_capture,
                ws_sender: Arc::new(Mutex::new(None)),
            };
            app.manage(app_state);
            Ok(())
        })
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![
            greet,
            get_input_devices,
            start_recording,
            stop_recording,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
