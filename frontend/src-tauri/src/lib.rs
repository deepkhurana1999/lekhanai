mod audio_engine;
mod audio_capture;
use tauri::State;
use tauri::Manager;
use crate::audio_engine::{AudioDevice};
use crate::audio_capture::{AudioCapture};

pub struct AppState {
    /// The central audio capture state manager
    pub audio_capture: AudioCapture,
}

#[tauri::command]
fn greet(name: &str) -> String {
    format!("Hello, {}! You've been greeted from Rust!", name)
}

#[tauri::command]
fn get_input_devices(
    state: State<'_, AppState>,
) -> Result<Vec<AudioDevice>, String> {
    // Delegate to AudioCapture
    state.audio_capture.get_devices()
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .setup(|app| {
            let window = app
                .get_webview_window("main")
                .expect("No main window found");
            let audio_capture = AudioCapture::new(window);
            let app_state = AppState { audio_capture };
            app.manage(app_state);
            Ok(())
        })
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![
            greet,
            get_input_devices
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
