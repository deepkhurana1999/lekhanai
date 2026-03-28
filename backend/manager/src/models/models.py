from sqlmodel import SQLModel, Field
from datetime import datetime
import uuid

class SessionModel(SQLModel, table=True):
    __tablename__ = "session_model"
    session_id: str = Field(default_factory=lambda: str(uuid.uuid4()), primary_key=True)
    user_id: str
    transcript: str = Field(default="")
    summary: str = Field(default="")
    created_at: datetime = Field(default_factory=datetime.utcnow)
    status: str = "active"

class SegmentModel(SQLModel, table=True):
    __tablename__ = "segment_model"
    segment_id: str = Field(default_factory=lambda: str(uuid.uuid4()), primary_key=True)
    session_id: str = Field(foreign_key="session_model.session_id")
    audio_url: str
