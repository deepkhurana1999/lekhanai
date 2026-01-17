
from fastapi import FastAPI, UploadFile, File, Form, Depends
from sqlmodel import Session
from models.schemas import CreateSessionCommand
from database.db import get_session
from commands.save_audio_file_command import upload_audio_command
from commands.create_session_command import create_session_command
from queries.queries import get_session_info_query

app = FastAPI()

@app.post("/api/v1/session/create")
def create_session(cmd: CreateSessionCommand, db: Session = Depends(get_session)):
    return create_session_command(cmd.user_id, db)

@app.post("/api/v1/audio/upload")
async def upload_audio(session_id: str = Form(...), file: UploadFile = File(...), db: Session = Depends(get_session)):
    return await upload_audio_command(session_id, file, db)

@app.get("/api/v1/session/{session_id}")
def get_session_info(session_id: str, db: Session = Depends(get_session)):
    return get_session_info_query(session_id, db)
