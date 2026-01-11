# API Gateway + Audio Ingestion + Session Manager (Python)
from fastapi import FastAPI, Request, UploadFile, File
import redis
import uuid
import aiofiles

app = FastAPI()
r = redis.Redis(host='localhost', port=6379, db=0)

@app.post('/api/v1/audio/stream')
async def stream_audio(session_id: str, file: UploadFile = File(...)):
    audio_path = f"audio/{session_id}.wav"
    async with aiofiles.open(audio_path, 'wb') as out_file:
        content = await file.read()
        await out_file.write(content)
    # Buffering, normalization, publish to VAD (simulate)
    r.set(f"session:{session_id}", "active")
    return {"status": "buffered", "audio_path": audio_path}

@app.post('/api/v1/session/create')
def create_session():
    session_id = str(uuid.uuid4())
    r.set(f"session:{session_id}", "active")
    return {"session_id": session_id, "status": "created"}

@app.get('/api/v1/session/{session_id}')
def get_session(session_id: str):
    status = r.get(f"session:{session_id}")
    return {"session_id": session_id, "status": status.decode() if status else "not found"}
