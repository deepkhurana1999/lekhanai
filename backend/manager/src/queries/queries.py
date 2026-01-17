
from sqlmodel import Session, select
from models.models import SessionModel, SegmentModel

def get_session_info_query(session_id: str, db: Session):
    session_obj = db.get(SessionModel, session_id)
    if not session_obj:
        return {"error": "Session not found"}
    segments = db.exec(select(SegmentModel).where(SegmentModel.session_id == session_id)).all()
    return {
        "session": session_obj.dict(),
        "segments": [seg.dict() for seg in segments]
    }
