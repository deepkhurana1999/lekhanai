import os
import io
import httpx
import numpy as np
import soundfile as sf
import librosa
from fastapi import UploadFile, File

STT_BASE_URL = os.getenv("STT_BASE_URL", "")

# Chunk parameters
SAMPLE_RATE = 16000
CHUNK_DURATION_S = 30
OVERLAP_S = 0.5
CHUNK_SAMPLES = CHUNK_DURATION_S * SAMPLE_RATE          # 480,000 samples
OVERLAP_SAMPLES = int(OVERLAP_S * SAMPLE_RATE)          # 8,000 samples


def _float32_to_pcm16(audio: np.ndarray) -> bytes:
    """Convert float32 numpy array [-1, 1] to raw PCM int16 bytes."""
    clipped = np.clip(audio, -1.0, 1.0)
    int16_audio = (clipped * 32767).astype(np.int16)
    return int16_audio.tobytes()


def _deduplicate_boundary(prev_text: str, next_text: str, window_words: int = 6) -> str:
    """
    Remove words from the start of next_text that are already at the end of
    prev_text, to avoid duplicates caused by the 0.5s overlap region.
    """
    if not prev_text or not next_text:
        return next_text

    prev_words = prev_text.split()
    next_words = next_text.split()

    # Check overlap of up to `window_words` words at the boundary
    for overlap in range(min(window_words, len(prev_words), len(next_words)), 0, -1):
        if prev_words[-overlap:] == next_words[:overlap]:
            return " ".join(next_words[overlap:])

    return next_text


async def _transcribe_chunk(client: httpx.AsyncClient, pcm_bytes: bytes) -> str:
    """Send a single binary PCM chunk to STT /api/v1/process and return text."""
    resp = await client.post(
        f"{STT_BASE_URL}/api/v1/process",
        content=pcm_bytes,
        headers={"Content-Type": "application/octet-stream"},
        timeout=120.0,
    )
    resp.raise_for_status()
    return resp.json().get("text", "").strip()


async def proxy_transcribe_audio(file: UploadFile = File(...)):
    """
    Transcribe an uploaded audio file via the STT service.

    Steps:
    1. Decode audio to float32 mono, 16kHz
    2. Split into 30s chunks with 0.5s overlap
    3. Send each chunk as raw binary PCM int16 to STT /api/v1/process
    4. Deduplicate overlap boundaries between chunks
    5. Return concatenated transcript
    """
    if not STT_BASE_URL:
        raise RuntimeError("STT_BASE_URL environment variable is not set")

    try:
        audio_bytes = await file.read()

        # Decode audio → float32 mono 16kHz
        try:
            data, samplerate = sf.read(io.BytesIO(audio_bytes), dtype="float32")
        except Exception:
            data, samplerate = librosa.load(io.BytesIO(audio_bytes), sr=None, mono=False)

        # Ensure mono
        if len(data.shape) > 1:
            data = np.mean(data, axis=1)

        # Resample to 16kHz if needed
        if samplerate != SAMPLE_RATE:
            data = librosa.resample(data, orig_sr=samplerate, target_sr=SAMPLE_RATE)

        total_samples = len(data)
        print(f"Audio: {total_samples} samples @ {SAMPLE_RATE}Hz "
              f"({total_samples / SAMPLE_RATE:.1f}s), "
              f"chunk_size={CHUNK_DURATION_S}s, overlap={OVERLAP_S}s")

        # Build chunk boundaries
        # Each chunk starts at: i * (CHUNK_SAMPLES - OVERLAP_SAMPLES)
        step = CHUNK_SAMPLES - OVERLAP_SAMPLES
        chunk_starts = list(range(0, total_samples, step))
        total_chunks = len(chunk_starts)
        print(f"Total chunks: {total_chunks}")

        full_text = ""
        async with httpx.AsyncClient() as client:
            for idx, start in enumerate(chunk_starts):
                end = min(start + CHUNK_SAMPLES, total_samples)
                chunk_audio = data[start:end]

                # Skip chunks that are too short to produce meaningful speech
                if len(chunk_audio) < SAMPLE_RATE // 2:  # < 0.5s
                    continue

                pcm_bytes = _float32_to_pcm16(chunk_audio)
                print(f"Chunk {idx + 1}/{total_chunks}: samples={len(chunk_audio)}, bytes={len(pcm_bytes)}")

                chunk_text = await _transcribe_chunk(client, pcm_bytes)
                print(f"Chunk {idx + 1} transcript: {chunk_text[:80]}...")

                if idx == 0:
                    full_text = chunk_text
                else:
                    # Remove duplicate words at the overlap boundary
                    chunk_text = _deduplicate_boundary(full_text, chunk_text)
                    if chunk_text:
                        full_text = full_text.rstrip() + " " + chunk_text

        return {"text": full_text.strip()}

    except Exception as e:
        print(f"Error in proxy_transcribe_audio: {e!r}")
        raise