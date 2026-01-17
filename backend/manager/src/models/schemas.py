from pydantic import BaseModel

class CreateSessionCommand(BaseModel):
    user_id: str

class UploadAudioCommand(BaseModel):
    session_id: str
    # file: UploadFile  # Not used directly in schema, handled in endpoint

class SessionQuery(BaseModel):
    session_id: str
