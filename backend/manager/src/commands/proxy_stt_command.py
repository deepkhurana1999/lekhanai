import os
import io
import httpx
import numpy as np
import soundfile as sf
import librosa
from fastapi import UploadFile, File

STT_BASE_URL = os.getenv("STT_BASE_URL", "")
if not STT_BASE_URL or len(STT_BASE_URL) == 0:
    raise ValueError("STT_BASE_URL environment variable is not set")


async def proxy_transcribe_audio(file: UploadFile = File(...)):
    try:
        # 1. Read audio file into bytes
        audio_bytes = await file.read()
        # 2. Decode audio to float32 numpy array (mono, 16kHz)
        # Try to decode with soundfile, fallback to librosa if needed
        try:
            data, samplerate = sf.read(io.BytesIO(audio_bytes), dtype='float32')
        except Exception:
            data, samplerate = librosa.load(io.BytesIO(audio_bytes), sr=None, mono=False)
        # Convert to mono if stereo
        if len(data.shape) > 1:
            data = np.mean(data, axis=1)
        # Resample to 16kHz if needed
        if samplerate != 16000:
            data = librosa.resample(data, orig_sr=samplerate, target_sr=16000)
            samplerate = 16000
        audio_list = data.tolist()
        print(f"Audio length (samples): {len(audio_list)}, Sample rate: {samplerate}")
        # 3. Call VAD endpoint to get segments
        batch_size = len(audio_list)  # e.g., 1 second batches at 16kHz 
        async with httpx.AsyncClient() as client:
            batches = [audio_list[i:i + batch_size ] for i in range(0, len(audio_list), batch_size)]
            total_segments = []
            vad_state = {"state": [], "context": []}
            for batch_idx, batch in enumerate(batches):
                print(f"Batch {batch_idx} length: {len(batch)}")
                payload = {
                    "audio": batch,
                    "state": vad_state.get("state", []),
                    "context": vad_state.get("context", [])
                }
                vad_resp = await client.post(f"{STT_BASE_URL}/api/v1/vad", json=payload)
                vad_resp.raise_for_status()
                segments = vad_resp.json().get("segments", [])
                vad_state["state"] = (vad_resp.json().get("state", []))
                vad_state["context"] = (vad_resp.json().get("context", []))

                for seg in segments:
                    # Adjust segment timestamps based on batch index
                    adjseg = {
                        "start": int(batch_idx * batch_size) + seg["start"],
                        "end": int(batch_idx * batch_size) + seg["end"]
                    }
                    total_segments.append(adjseg)

            segments = total_segments
            print(f"Total segments length: {len(segments)}")
            # 4. Call transcribe endpoint with segments and audio
            transcribe_payload = {"segments": segments, "audio": audio_list}
            transcribe_resp = await client.post(f"{STT_BASE_URL}/api/v1/transcribe", json=transcribe_payload, timeout=120.0)
            transcribe_resp.raise_for_status()
            return transcribe_resp.json()
    except Exception as e:
        print(f"Error in proxy_transcribe_audio: {e}")
        print(e)
        print("Exception type:", type(e))
        print("Exception repr:", repr(e))
        raise e