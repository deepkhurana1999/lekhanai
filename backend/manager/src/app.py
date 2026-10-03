from fastapi import FastAPI, UploadFile, File, Form, Depends, Query, WebSocket, WebSocketDisconnect
from fastapi.responses import JSONResponse
from fastapi.middleware.cors import CORSMiddleware
from sqlmodel import Session
import asyncio
import json

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
    # http://localhost:1420 is only the Vite dev server. The packaged Tauri
    # app (AppImage/.deb/.rpm) serves its UI from tauri://localhost instead,
    # which the default allowlist rejected with "Disallowed CORS origin" on
    # every request, including the CreateSession preflight the app makes on
    # every "start recording" click.
    allow_origins=["http://localhost:1420", "tauri://localhost"],
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
async def websocket_transcribe(websocket: WebSocket, session_id: str, db: Session = Depends(get_session)):
    """
    WebSocket bridge: Tauri client → Manager → STT /api/v1/process.

    Protocol:
      - Binary frames: [1 source tag byte][raw PCM int16 audio bytes] (16kHz mono).
                       Tag byte: 0x00 = mic, 0x01 = system audio.
      - Text frames:   JSON commands, e.g. {"type": "generate_summary", "text": "..."}
      - Server sends:  {"type": "transcription", "text": "...", "source": "mic"|"system"}
                       or {"type": "error", "message": "..."}
    """
    await websocket.accept()
    session_obj = get_session_info_query(session_id, db)
    if not session_obj or session_obj.get('error'):
        await websocket.send_json({"type": "error", "message": f"Session {session_id} not found"})
        await websocket.close(code=4004)
        return

    # Read transcript while session_obj is still in scope (within the DB session)
    accumulated_transcript = session_obj['session']['transcript'] or ""
    last_source = None

    # Decouple STT processing from the receive loop: chunks are queued here
    # and transcribed by a background worker at whatever pace Whisper can
    # sustain, instead of blocking receive() on every chunk. This is what
    # was causing the WS connection to stall/disconnect under slow or
    # back-to-back-busy STT calls — the loop can now always keep draining
    # the socket regardless of how far behind transcription falls.
    chunk_queue: asyncio.Queue = asyncio.Queue()

    async def process_queue():
        nonlocal accumulated_transcript, last_source
        while True:
            item = await chunk_queue.get()
            if item is None:
                break
            source, pcm_bytes = item

            try:
                text = await realtime_transcribe_chunk(pcm_bytes)
            except Exception as e:
                # A single chunk's STT failure (transient network blip, an
                # odd-sized chunk, etc.) shouldn't tear down the whole
                # recording session — skip this chunk and keep going.
                print(f"Skipping chunk after transcribe error for session {session_id}: {e!r}")
                continue

            if not text:
                continue

            # Prefix with a speaker label only when the source changes, so a
            # continuous run from the same source isn't relabeled every chunk.
            labeled_text = text
            if source != last_source:
                label = "[System] " if source == "system" else "[You] "
                labeled_text = label + text
                last_source = source

            # Append to running transcript
            accumulated_transcript = (accumulated_transcript.rstrip() + " " + labeled_text).strip()

            # Persist incrementally to DB
            update_session_command(session_id, db, accumulated_transcript, "", "active")

            try:
                await websocket.send_json({"type": "transcription", "text": text, "source": source})
            except Exception as e:
                # Client may have already disconnected by the time a
                # lagging-behind chunk finishes transcribing — nothing to do.
                print(f"Failed to send transcription for session {session_id}: {e!r}")

    worker_task = asyncio.create_task(process_queue())

    try:
        while True:
            message = await websocket.receive()

            if message["type"] == "websocket.disconnect":
                # Raw ASGI receive() delivers this exactly once when the
                # client's connection actually drops; calling receive() again
                # after it is a hard error (WebSocketDisconnected), not a
                # retriable condition — stop the loop cleanly instead.
                print(f"WebSocket disconnect message received for session {session_id}")
                break

            if "bytes" in message and message["bytes"]:
                # Binary: [1 tag byte][raw PCM int16 chunk] from Tauri
                frame = message["bytes"]
                source = "system" if frame[0] == 1 else "mic"
                pcm_bytes = frame[1:]
                chunk_queue.put_nowait((source, pcm_bytes))

            elif "text" in message and message["text"]:
                try:
                    data = json.loads(message["text"])
                    if data.get("type") == "generate_summary":
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
    finally:
        # Let the worker finish any chunks already queued, then stop it.
        await chunk_queue.put(None)
        await worker_task
