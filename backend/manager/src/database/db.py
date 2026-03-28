from sqlmodel import SQLModel, create_engine, Session
import os
from models.models import SessionModel, SegmentModel  # Ensure models are imported

DATABASE_URL = os.getenv("DATABASE_URL", "postgresql://postgres:postgres@postgres:5432/stt_db")
engine = create_engine(DATABASE_URL, echo=True)

SQLModel.metadata.create_all(engine)

def get_session():
    with Session(engine) as session:
        yield session
