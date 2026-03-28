
from fastapi import UploadFile
from sqlmodel import Session
import uuid
from models import SegmentModel
from commands.save_audio_filesystem_command import save_audio_file

# Command: Upload Audio
async def upload_audio_command(session_id: str, file: UploadFile, db: Session):
    audio_id = str(uuid.uuid4())
    audio_path = await save_audio_file(file, audio_id)
    segment = SegmentModel(segment_id=audio_id, session_id=session_id, audio_url=audio_path)
    db.add(segment)
    db.commit()
    db.refresh(segment)
    return {"audio_id": segment.segment_id, "audio_path": segment.audio_url}
