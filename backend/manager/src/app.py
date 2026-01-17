from fastapi import FastAPI, UploadFile, File, Form, Depends, Query
from fastapi.responses import JSONResponse
from sqlmodel import Session
from fastapi.middleware.cors import CORSMiddleware

from database.db import get_session
from models.schemas import CreateSessionCommand, UpdateSessionCommand
from commands.proxy_stt_command import proxy_transcribe_audio
from queries import get_session_info_query, list_session_query
from commands.save_audio_file_command import upload_audio_command
from commands.create_session_command import create_session_command
from commands.update_session_command import update_session_command

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:1420"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"]
)

@app.post("/api/v1/session/create")
def create_session(cmd: CreateSessionCommand, db: Session = Depends(get_session)):
    return create_session_command(cmd.user_id, db)

@app.post("/api/v1/audio/upload")
async def upload_audio(session_id: str = Form(...), file: UploadFile = File(...), db: Session = Depends(get_session)):
    return await upload_audio_command(session_id, file, db)

@app.get("/api/v1/session/{session_id}")
def get_session_info(session_id: str, db: Session = Depends(get_session)):
    return get_session_info_query(session_id, db)

@app.put("/api/v1/session/{session_id}")
def update_session(cmd: UpdateSessionCommand, db: Session = Depends(get_session)):
    return update_session_command(cmd.session_id, db, cmd.transcript, cmd.summary, cmd.status)

@app.post("/api/v1/audio/transcribe")
async def transcribe_audio(file: UploadFile = File(...)):
    try:
        result = await proxy_transcribe_audio(file)
        return JSONResponse(content=result)
    except Exception as e:
        return JSONResponse(content={"error": str(e)}, status_code=500)

@app.get("/api/v1/sessions")
def get_sessions(limit: int = Query(10, ge=1, le=100), offset: int = Query(0, ge=0), db: Session = Depends(get_session)):
    return list_session_query(offset, limit, db)
