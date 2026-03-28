from typing import Optional
from pydantic import BaseModel

class CreateSessionCommand(BaseModel):
    user_id: str

class UpdateSessionCommand(BaseModel):
    session_id: str
    transcript: Optional[str] = None
    summary: Optional[str] = None
    status: Optional[str] = None

class UploadAudioCommand(BaseModel):
    session_id: str
    # file: UploadFile  # Not used directly in schema, handled in endpoint

class SessionQuery(BaseModel):
    session_id: str
