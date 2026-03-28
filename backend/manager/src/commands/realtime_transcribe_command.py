import os
import httpx

STT_BASE_URL = os.getenv("STT_BASE_URL", "")


async def realtime_transcribe_chunk(pcm_bytes: bytes) -> str:
    """
    Send a single raw PCM int16 audio chunk (16kHz, mono) to STT /api/v1/process.
    Returns the transcribed text, or an empty string if nothing was detected.

    Called by the WebSocket bridge for each audio chunk received from the Tauri client.
    """
    if not pcm_bytes or len(pcm_bytes) < 2:
        return ""

    try:
        async with httpx.AsyncClient() as client:
            resp = await client.post(
                f"{STT_BASE_URL}/api/v1/process",
                content=pcm_bytes,
                headers={"Content-Type": "application/octet-stream"},
                timeout=60.0,
            )
            resp.raise_for_status()
            data = resp.json()
            text = data.get("text", "").strip()
            # Filter out Whisper's blank audio marker
            if text in ("[BLANK_AUDIO]", "[BLANK_AUDIO] "):
                return ""
            return text
    except Exception as e:
        print(f"realtime_transcribe_chunk error: {e!r}")
        raise
