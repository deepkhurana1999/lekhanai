use cpal::traits::{DeviceTrait, HostTrait};
use cpal::{Stream, StreamConfig};
use serde::Serialize;

/// Audio device information
#[derive(Serialize, Clone, Debug)]
pub struct AudioDevice {
    pub id: String,
    pub name: String,
    pub channels: u16,
}

/// Device configuration
#[derive(Clone, Debug)]
pub struct DeviceConfig {
    pub sample_rate: u32,
    pub channels: u16,
    pub config: StreamConfig,
}

pub struct AudioEngine;

/// Sentinel id for the system's default input device (see `resolve_device`).
pub const DEFAULT_DEVICE_ID: &str = "default";

impl AudioEngine {
    /// Resolve a device id (either `DEFAULT_DEVICE_ID` or a device name from
    /// `list_input_devices`) to a concrete `cpal::Device`.
    ///
    /// `host.default_input_device()` is used for the sentinel rather than
    /// hunting for a "default"/"pipewire" entry inside `host.input_devices()`:
    /// that enumeration eagerly opens both playback AND capture handles just
    /// to decide whether to list a builtin PCM at all (see cpal's
    /// `alsa::enumerate::Devices::next`), so on a PipeWire-managed system it
    /// can silently drop "default"/"pipewire" from the list if the capture
    /// side transiently fails to open — even though PipeWire's shared PCM
    /// works fine moments later. `default_input_device()` instead always
    /// returns a `Device` wrapping the "default" PCM id without eagerly
    /// opening anything, deferring the real open to actual stream build /
    /// config query time — which is exactly what we want for a device meant
    /// to be opened concurrently with PipeWire itself.
    ///
    /// Non-default devices are looked up by NAME, not positional index:
    /// `host.input_devices()`'s enumeration order can shift between two
    /// separate calls (e.g. a builtin PCM flips from unavailable to
    /// available), so an index captured by `list_input_devices` can point at
    /// a different device — or nothing at all — by the time a stream is
    /// actually built. The device's ALSA PCM name is stable across calls.
    fn resolve_device(device_id: &str) -> Result<cpal::Device, String> {
        let host = cpal::default_host();

        if device_id == DEFAULT_DEVICE_ID {
            return host
                .default_input_device()
                .ok_or("No default input device available".to_string());
        }

        host.input_devices()
            .map_err(|e| format!("Failed to get devices: {e}"))?
            .find(|d| d.name().map(|n| n == device_id).unwrap_or(false))
            .ok_or(format!("Device '{device_id}' not found"))
    }

    /// List available input audio devices, including a synthetic "System
    /// Default Microphone" entry (see `resolve_device`) ahead of the
    /// per-card devices — this is the one that should work regardless of
    /// which physical device PipeWire currently has claimed.
    pub fn list_input_devices() -> Result<Vec<AudioDevice>, String> {
        let host = cpal::default_host();
        let mut devices = Vec::new();

        if let Some(default_device) = host.default_input_device() {
            let channels = default_device
                .default_input_config()
                .map(|c| c.channels())
                .unwrap_or(1);
            devices.push(AudioDevice {
                id: DEFAULT_DEVICE_ID.to_string(),
                name: "System Default Microphone".to_string(),
                channels,
            });
        }

        match host.input_devices() {
            Ok(inputs) => {
                for device in inputs {
                    let Ok(name) = device.name() else { continue };
                    // Already represented by the synthetic default entry above.
                    if name == DEFAULT_DEVICE_ID {
                        continue;
                    }
                    match device.default_input_config() {
                        Ok(config) => {
                            let channels = config.channels();
                            devices.push(AudioDevice {
                                id: name.clone(),
                                name,
                                channels
                            });
                            continue;
                        }
                        Err(_) => {
                            continue;
                        }
                    }
                }
                Ok(devices)
            }
            Err(_) => {
                if devices.is_empty() {
                    Err("Failed to get input devices".to_string())
                } else {
                    // Per-card enumeration failed, but the default device is
                    // still usable — don't fail the whole list over it.
                    Ok(devices)
                }
            }
        }
    }

    /// Get configuration for a specific device.
    ///
    /// Input: device id, either `DEFAULT_DEVICE_ID` or a device name from
    /// `list_input_devices`.
    /// Returns: Sample rate, channels, and CPAL config
    pub fn get_device_config(device_id: &str) -> Result<DeviceConfig, String> {
        let device = Self::resolve_device(device_id)?;

        let config = device.default_input_config().map_err(|_| "Failed to get default input config".to_string())?;
        let sample_rate = config.sample_rate().0;
        let channels = config.channels();
        let stream_config: StreamConfig = config.into();

        Ok(DeviceConfig {
            sample_rate,
            channels,
            config: stream_config,
        })
    }


    /// Build audio stream from a device
    ///
    /// Input: device id (see `get_device_config`), callback function
    /// Returns: (Stream, DeviceConfig)
    ///
    /// The callback is called 100+ times per second with audio samples
    pub fn build_stream<F>(
        device_id: &str,
        mut on_audio_data: F,
    ) -> Result<(Stream, DeviceConfig), String>
    where
        F: FnMut(&[f32], &cpal::InputCallbackInfo) + Send + 'static,
    {
        let device = Self::resolve_device(device_id)?;
        let device_config = Self::get_device_config(device_id)?;
        let device_name = device.name().unwrap_or("Unknown".to_string());

        println!("Building stream for device: {}", device_name);

        let stream = device
            .build_input_stream(
                &device_config.config,
                move |data: &[f32], _info: &cpal::InputCallbackInfo| {
                    on_audio_data(data, _info)
                },
                |err| eprintln!("Stream error: {}", err),
                None,
            )
            .map_err(|e| format!("Failed to build stream: {}", e))?;

        Ok((stream, device_config))
    }
}