from sqlmodel import Session
from models import SessionModel

# Command: Create Session
def create_session_command(user_id: str, db: Session):
    session_obj = SessionModel(user_id=user_id)
    session_obj.transcript = ""
    session_obj.summary = ""
    db.add(session_obj)
    db.commit()
    db.refresh(session_obj)
    return {"session_id": session_obj.session_id, "status": session_obj.status}