import aiofiles
import os

AUDIO_DIR = os.getenv("AUDIO_DIR", "audio_files")
os.makedirs(AUDIO_DIR, exist_ok=True)

def get_audio_path(audio_id: str) -> str:
    return os.path.join(AUDIO_DIR, f"{audio_id}.wav")

async def save_audio_file(file, audio_id: str) -> str:
    audio_path = get_audio_path(audio_id)
    async with aiofiles.open(audio_path, 'wb') as out_file:
        content = await file.read()
        await out_file.write(content)
    return audio_path
