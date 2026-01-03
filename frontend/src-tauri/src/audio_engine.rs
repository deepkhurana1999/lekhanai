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

impl AudioEngine {
    /// List available input audio devices
    pub fn list_input_devices() -> Result<Vec<AudioDevice>, String> {
        let host = cpal::default_host();
        let mut devices = Vec::new();

        match host.input_devices() {
            Ok(inputs) => {
                for (index, device) in inputs.enumerate() {
                    match device.default_input_config() {
                        Ok(config) => {
                            let name = device.name().unwrap_or_else(|_| format!("Device {}", index));
                            let channels = config.channels();
                            devices.push(AudioDevice {
                                id: index.to_string(),
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
            Err(_) => Err("Failed to get input devices".to_string()),
        }
    }

    /// Get configuration for a specific device
    /// 
    /// Input: device_index (from enumerate_devices)
    /// Returns: Sample rate, channels, and CPAL config
    pub fn get_device_config(device_index: usize) -> Result<DeviceConfig, String> {
        let host = cpal::default_host();
        let inputs = host.input_devices().map_err(|_| "Failed to get input devices".to_string())?;
        let device = inputs.into_iter().nth(device_index).ok_or("Invalid device index".to_string())?;

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
    /// Input: device_index, callback function
    /// Returns: (Stream, DeviceConfig)
    /// 
    /// The callback is called 100+ times per second with audio samples
    pub fn build_stream<F>(
        device_index: usize,
        mut on_audio_data: F,
    ) -> Result<(Stream, DeviceConfig), String>
    where
        F: FnMut(&[f32], &cpal::InputCallbackInfo) + Send + 'static,
    {
        let host = cpal::default_host();
        let device = host
            .input_devices()
            .map_err(|e| format!("Failed to get devices: {}", e))?
            .nth(device_index)
            .ok_or(format!("Device {} not found", device_index))?;

        let device_config = Self::get_device_config(device_index)?;
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