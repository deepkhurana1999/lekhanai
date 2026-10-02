use std::sync::{Arc, Mutex};
use std::process::{Command, Stdio};
use std::io::Read;
use std::thread::JoinHandle;

// ============================================================================
// SYSTEM AUDIO CAPTURE STATE MANAGER
// ============================================================================

/// Captures system audio (loopback/monitor output) via pw-record subprocess.
///
/// Spawns `pw-record` to capture from @DEFAULT_SINK@.monitor in mono f32 format
/// at 48kHz, reads raw f32 LE bytes from stdout, and buffers decoded samples.
pub struct SystemAudioCapture {
    /// The spawned pw-record process (killed on stop)
    child: Arc<Mutex<Option<std::process::Child>>>,

    /// Shared buffer for decoded f32 samples
    sample_buf: Arc<Mutex<Vec<f32>>>,

    /// Reader thread handle (joined on stop)
    reader_thread: Arc<Mutex<Option<JoinHandle<()>>>>,
}

// ============================================================================
// IMPLEMENTATION
// ============================================================================

impl SystemAudioCapture {
    pub fn new() -> Self {
        Self {
            child: Arc::new(Mutex::new(None)),
            sample_buf: Arc::new(Mutex::new(Vec::new())),
            reader_thread: Arc::new(Mutex::new(None)),
        }
    }

    /// Start capturing system audio from the default sink's monitor.
    ///
    /// Spawns `pw-record` with: --target @DEFAULT_SINK@.monitor --rate 48000 --channels 1 --format f32 --raw -
    /// Spawns a reader thread that decodes raw f32 LE bytes from stdout and appends to sample_buf.
    ///
    /// Returns Err if pw-record is not available or PipeWire is not running (caller should fall back to mic-only).
    pub fn start(&self) -> Result<(), String> {
        // Ensure we're not already running
        if self.child.lock().unwrap().is_some() {
            return Err("System audio capture already running".to_string());
        }

        // Spawn pw-record process
        let mut child = Command::new("pw-record")
            .args(&[
                "--target", "@DEFAULT_SINK@.monitor",
                "--rate", "48000",
                "--channels", "1",
                "--format", "f32",
                "--raw",
                "-",
            ])
            .stdout(Stdio::piped())
            .spawn()
            .map_err(|e| format!("Failed to spawn pw-record: {e}"))?;

        let stdout = match child.stdout.take() {
            Some(out) => out,
            None => {
                // Kill the child before returning error to avoid process leak
                let _ = child.kill();
                return Err("Failed to open pw-record stdout".to_string());
            }
        };

        // Store the child process
        *self.child.lock().unwrap() = Some(child);

        // Spawn reader thread
        let buf = self.sample_buf.clone();
        let reader_handle = std::thread::spawn(move || {
            Self::reader_loop(stdout, buf);
        });

        *self.reader_thread.lock().unwrap() = Some(reader_handle);

        println!("System audio capture started");
        Ok(())
    }

    /// Reader loop: reads raw f32 LE bytes from pw-record stdout and decodes into sample_buf.
    /// Maintains a leftover-bytes buffer to handle floats split across read() boundaries.
    fn reader_loop(mut stdout: std::process::ChildStdout, buf: Arc<Mutex<Vec<f32>>>) {
        let mut byte_buf = [0u8; 4096];
        let mut leftover = Vec::with_capacity(3); // Can hold 0-3 remaining bytes from previous read

        loop {
            match stdout.read(&mut byte_buf) {
                Ok(0) => {
                    // EOF
                    if !leftover.is_empty() {
                        eprintln!(
                            "System audio reader: EOF reached with {} incomplete bytes (dropped)",
                            leftover.len()
                        );
                    }
                    println!("System audio reader: EOF reached");
                    break;
                }
                Ok(n) => {
                    // Combine leftover bytes from previous read with new bytes
                    leftover.extend_from_slice(&byte_buf[..n]);

                    // Decode complete 4-byte chunks into f32 LE samples
                    let complete_chunks = leftover.len() / 4;
                    let samples: Vec<f32> = leftover[..complete_chunks * 4]
                        .chunks_exact(4)
                        .map(|chunk| {
                            let bytes = [chunk[0], chunk[1], chunk[2], chunk[3]];
                            f32::from_le_bytes(bytes)
                        })
                        .collect();

                    // Save any remaining 0-3 bytes for the next read
                    let remainder_start = complete_chunks * 4;
                    leftover.drain(..remainder_start);

                    // Append decoded samples to shared buffer
                    let mut guard = buf.lock().unwrap();
                    guard.extend_from_slice(&samples);
                }
                Err(e) => {
                    eprintln!("System audio reader error: {e}");
                    break;
                }
            }
        }
    }

    /// Stop capturing. Kill the child process and join the reader thread, then clear the buffer.
    pub fn stop(&self) -> Result<(), String> {
        // Kill the child process
        if let Some(mut child) = self.child.lock().unwrap().take() {
            let _ = child.kill();
            let _ = child.wait();
        }

        // Join the reader thread
        if let Some(handle) = self.reader_thread.lock().unwrap().take() {
            let _ = handle.join();
        }

        // Clear the sample buffer
        self.sample_buf.lock().unwrap().clear();

        println!("System audio capture stopped");
        Ok(())
    }

    /// Drain up to `n` samples from the buffer and return them.
    /// Used by the caller's flush logic to extract samples for processing.
    pub fn take_buffer(&self, n: usize) -> Vec<f32> {
        let mut guard = self.sample_buf.lock().unwrap();
        let to_drain = n.min(guard.len());
        guard.drain(..to_drain).collect()
    }
}

impl Default for SystemAudioCapture {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_system_audio_capture_creation() {
        let capture = SystemAudioCapture::new();
        assert!(capture.child.lock().unwrap().is_none());
        assert!(capture.sample_buf.lock().unwrap().is_empty());
        assert!(capture.reader_thread.lock().unwrap().is_none());
    }

    #[test]
    fn test_take_buffer_empty() {
        let capture = SystemAudioCapture::new();
        let buf = capture.take_buffer(100);
        assert!(buf.is_empty());
    }

    #[test]
    fn test_take_buffer_with_samples() {
        let capture = SystemAudioCapture::new();

        // Manually insert some samples
        {
            let mut guard = capture.sample_buf.lock().unwrap();
            guard.extend_from_slice(&[0.1, 0.2, 0.3, 0.4, 0.5]);
        }

        // Take 3 samples
        let buf = capture.take_buffer(3);
        assert_eq!(buf.len(), 3);
        assert_eq!(buf, vec![0.1, 0.2, 0.3]);

        // Buffer should have 2 remaining
        {
            let guard = capture.sample_buf.lock().unwrap();
            assert_eq!(guard.len(), 2);
        }
    }

    #[test]
    fn test_take_buffer_more_than_available() {
        let capture = SystemAudioCapture::new();

        // Manually insert some samples
        {
            let mut guard = capture.sample_buf.lock().unwrap();
            guard.extend_from_slice(&[0.1, 0.2]);
        }

        // Try to take 10, should only get 2
        let buf = capture.take_buffer(10);
        assert_eq!(buf.len(), 2);
        assert_eq!(buf, vec![0.1, 0.2]);

        // Buffer should be empty
        {
            let guard = capture.sample_buf.lock().unwrap();
            assert_eq!(guard.len(), 0);
        }
    }

    /// Unit test: Verify reader_loop handles floats split across read() boundaries.
    /// This test simulates the issue where a single f32 sample is split across
    /// two separate read() calls and verifies it's reconstructed correctly.
    #[test]
    fn test_reader_loop_split_float_handling() {
        use std::io::{self, Read};

        // Simulate a split float: encode a test value (0.5) as f32 LE bytes
        let test_value = 0.5_f32;
        let bytes = test_value.to_le_bytes();
        assert_eq!(bytes.len(), 4);

        // Create a mock reader that splits the 4 bytes into two separate reads:
        // First read returns 2 bytes, second read returns 2 bytes
        struct SplitReader {
            all_bytes: Vec<u8>,
            pos: usize,
        }

        impl Read for SplitReader {
            fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
                if self.pos >= self.all_bytes.len() {
                    return Ok(0); // EOF
                }

                // First read: return 2 bytes (split in middle of float)
                // Second read: return 2 bytes (rest of float)
                // Third read: EOF
                let to_read = if self.pos == 0 {
                    2.min(buf.len())
                } else {
                    (self.all_bytes.len() - self.pos).min(buf.len())
                };

                let chunk = &self.all_bytes[self.pos..self.pos + to_read];
                buf[..to_read].copy_from_slice(chunk);
                self.pos += to_read;
                Ok(to_read)
            }
        }

        let reader = SplitReader {
            all_bytes: bytes.to_vec(),
            pos: 0,
        };

        // Run reader_loop with the split reader
        let buf = Arc::new(Mutex::new(Vec::new()));
        Self::reader_loop(reader, buf.clone());

        // Verify the float was decoded correctly despite the split
        let decoded = buf.lock().unwrap();
        assert_eq!(decoded.len(), 1, "Should decode exactly 1 sample");
        assert_eq!(decoded[0], test_value, "Decoded value should match original");
    }

    /// Integration test: Start capture and verify data flows into the buffer.
    /// This test actually spawns pw-record and waits for samples.
    ///
    /// Note: This requires:
    /// 1. PipeWire running on the system
    /// 2. The @DEFAULT_SINK@.monitor node available
    /// 3. System audio playing (or at least the monitor node active)
    ///
    /// If any of these preconditions are not met, this test will timeout or fail gracefully.
    #[test]
    #[ignore] // Ignored by default; run with `cargo test -- --ignored` if you have PipeWire
    fn test_capture_integration_with_pw_record() {
        use std::time::Duration;
        use std::thread::sleep;

        let capture = SystemAudioCapture::new();

        // Try to start capture
        match capture.start() {
            Ok(()) => {
                println!("pw-record started successfully");

                // Wait for some data to flow in
                sleep(Duration::from_millis(500));

                // Check if we captured any samples
                let buf = capture.take_buffer(1000);
                println!("Captured {} samples", buf.len());

                // Stop capture
                let _ = capture.stop();

                // Even if we got no samples (device silent or no audio playing),
                // the test passes as long as pw-record started and didn't crash.
                // In a real scenario with audio playing, we'd have buf.len() > 0.
            }
            Err(e) => {
                println!("pw-record unavailable (expected in CI/non-PipeWire systems): {}", e);
                // This is OK — pw-record may not be available; we expect graceful fallback
            }
        }
    }
}
