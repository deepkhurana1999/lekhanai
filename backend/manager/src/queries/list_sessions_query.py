from sqlmodel import Session
from models import SessionModel

def list_session_query(offset: int, limit: int, db: Session):
    # Assuming SessionModel is the ORM model for sessions
    sessions = db.query(SessionModel).order_by(SessionModel.created_at.desc()).offset(offset).limit(limit).all()
    # Convert to dicts for JSON response
    return [s.to_dict() if hasattr(s, 'to_dict') else s.__dict__ for s in sessions]
