from sqlmodel import Session
from models import SessionModel

# Command: Update Session
def update_session_command(session_id: str, db: Session, transcript: str = None, summary: str = None, status: str = None):
    session_obj = db.get(SessionModel, session_id)
    if not session_obj:
        return {"error": "Session not found"}
    
    if transcript is not None:
        session_obj.transcript = transcript
    if summary is not None:
        session_obj.summary = summary
    if status is not None:
        session_obj.status = status

    db.add(session_obj)
    db.commit()
    db.refresh(session_obj)
    return {
        "session_id": session_obj.session_id,
        "transcript": session_obj.transcript,
        "summary": session_obj.summary,
        "status": session_obj.status
    }