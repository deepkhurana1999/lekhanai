import './Header.css';

function Header({ status = 'idle' }) {
    return (
        <header className="header fade-in">
            <div className="header-content">
                <div className="header-left">
                    <h1 className="app-title">Lekhan AI</h1>
                    <p className="app-subtitle">Real-time Transcription</p>
                </div>
                <div className="header-right">
                    <div className="status-container">
                        <span className={`status-dot ${status}`}></span>
                        <span className="status-text">
                            {status === 'recording' && 'Recording'}
                            {status === 'processing' && 'Processing'}
                            {status === 'connected' && 'Connected'}
                            {status === 'idle' && 'Ready'}
                        </span>
                    </div>
                </div>
            </div>
        </header>
    );
}

export default Header;
