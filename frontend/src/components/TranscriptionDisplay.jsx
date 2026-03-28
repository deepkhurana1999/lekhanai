import { useState, useEffect, useRef } from 'react';
import './TranscriptionDisplay.css';

function TranscriptionDisplay({ transcription = '', isRecording = false }) {
    const displayRef = useRef(null);

    useEffect(() => {
        // Auto-scroll to bottom when new transcription comes in
        if (displayRef.current) {
            displayRef.current.scrollTop = displayRef.current.scrollHeight;
        }
    }, [transcription]);

    const handleCopy = () => {
        navigator.clipboard.writeText(transcription);
    };

    return (
        <div className="transcription-container glass-card fade-in">
            <div className="transcription-header">
                <h3 className="transcription-title">Transcription</h3>
                {transcription && (
                    <button
                        className="btn btn-secondary btn-copy"
                        onClick={handleCopy}
                        title="Copy to clipboard"
                    >
                        📋 Copy
                    </button>
                )}
            </div>
            <div
                ref={displayRef}
                className={`transcription-display ${isRecording ? 'recording' : ''}`}
            >
                {transcription ? (
                    <p className="transcription-text">{transcription}</p>
                ) : (
                    <div className="transcription-placeholder">
                        <svg className="placeholder-icon" width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                            <path d="M12 1a3 3 0 0 0-3 3v8a3 3 0 0 0 6 0V4a3 3 0 0 0-3-3z"></path>
                            <path d="M19 10v2a7 7 0 0 1-14 0v-2"></path>
                            <line x1="12" y1="19" x2="12" y2="23"></line>
                            <line x1="8" y1="23" x2="16" y2="23"></line>
                        </svg>
                        <p>Start recording to see transcription...</p>
                    </div>
                )}
            </div>
        </div>
    );
}

export default TranscriptionDisplay;
