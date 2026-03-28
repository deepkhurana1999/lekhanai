import './ControlPanel.css';

function ControlPanel({
    isRecording = false,
    onStartRecording,
    onStopRecording,
    onUploadFile
}) {
    return (
        <div className="control-panel glass-card fade-in">
            <div className="control-buttons">
                {!isRecording ? (
                    <button
                        className="btn btn-primary btn-large record-btn"
                        onClick={onStartRecording}
                        title="Start Recording"
                    >
                        <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
                            <circle cx="12" cy="12" r="8" />
                        </svg>
                    </button>
                ) : (
                    <button
                        className="btn btn-secondary btn-large stop-btn"
                        onClick={onStopRecording}
                        title="Stop Recording"
                    >
                        <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
                            <rect x="6" y="6" width="12" height="12" rx="2" />
                        </svg>
                    </button>
                )}

                <button
                    className="btn btn-secondary"
                    onClick={onUploadFile}
                    title="Upload Audio File"
                >
                    📁 Upload File
                </button>
            </div>

            <div className="control-info">
                <p className="control-text">
                    {isRecording
                        ? 'Recording in progress...'
                        : 'Click the record button or upload an audio file to begin'
                    }
                </p>
            </div>
        </div>
    );
}

export default ControlPanel;
