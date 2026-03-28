from fastapi import FastAPI, UploadFile, File, Form, Depends, Query, WebSocket, WebSocketDisconnect
from fastapi.responses import JSONResponse
from sqlmodel import Session
from fastapi.middleware.cors import CORSMiddleware

from database.db import get_session
from models.schemas import CreateSessionCommand, UpdateSessionCommand
from commands.proxy_stt_command import proxy_transcribe_audio
from commands.realtime_transcribe_command import realtime_transcribe_chunk
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

@app.websocket("/ws/transcribe/{session_id}")
async def websocket_transcribe(websocket: WebSocket, session_id: str):
    """
    WebSocket bridge: Tauri client → Manager → STT /api/v1/process.

    Protocol:
      - Binary frames: raw PCM int16 audio chunks (16kHz mono)
      - Text frames:   JSON commands, e.g. {"type": "generate_summary", "text": "..."}
      - Server sends:  {"type": "transcription", "text": "..."} or {"type": "error", "message": "..."}
    """
    await websocket.accept()

    # Validate session exists
    from database.db import engine
    from sqlmodel import Session as DBSession, select
    from models.models import SessionModel
    with DBSession(engine) as db:
        session_obj = db.exec(select(SessionModel).where(SessionModel.session_id == session_id)).first()
        if not session_obj:
            await websocket.send_json({"type": "error", "message": f"Session {session_id} not found"})
            await websocket.close(code=4004)
            return

    accumulated_transcript = session_obj.transcript or ""

    try:
        while True:
            message = await websocket.receive()

            if "bytes" in message and message["bytes"]:
                # Binary: raw PCM int16 chunk from Tauri
                pcm_bytes = message["bytes"]
                text = await realtime_transcribe_chunk(pcm_bytes)

                if text:
                    # Append to running transcript
                    accumulated_transcript = (accumulated_transcript.rstrip() + " " + text).strip()

                    # Persist incrementally
                    from database.db import engine
                    from sqlmodel import Session as DBSession
                    with DBSession(engine) as db:
                        obj = db.get(SessionModel, session_id)
                        if obj:
                            obj.transcript = accumulated_transcript
                            db.add(obj)
                            db.commit()

                    await websocket.send_json({"type": "transcription", "text": text})

            elif "text" in message and message["text"]:
                import json
                try:
                    data = json.loads(message["text"])
                    if data.get("type") == "generate_summary":
                        # Forward to STT summary endpoint or handle here
                        await websocket.send_json({"type": "info", "message": "Summary generation not yet implemented over WebSocket"})
                except Exception:
                    await websocket.send_json({"type": "error", "message": "Invalid JSON command"})

    except WebSocketDisconnect:
        print(f"WebSocket disconnected for session {session_id}")
    except Exception as e:
        print(f"WebSocket error for session {session_id}: {e!r}")
        try:
            await websocket.send_json({"type": "error", "message": str(e)})
        except Exception:
            pass
