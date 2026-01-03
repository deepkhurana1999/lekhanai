import { useState, useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import "./App.css";

const AppState = {
  IDLE: "idle",
  RECORDING: "recording",
  VIEWING: "viewing"
}

// Mock historical sessions
const MOCK_SESSIONS = [
  {
    id: 1,
    timestamp: new Date(Date.now() - 2 * 60 * 60 * 1000), // 2 hours ago
    audioSource: "Built-in Microphone",
    transcript: "This is a sample transcription from earlier today. The user spoke about various topics and the system captured their words accurately in real-time.",
    summary: "Discussion about various topics with accurate real-time transcription.",
    status: "completed"
  },
  {
    id: 2,
    timestamp: new Date(Date.now() - 5 * 60 * 60 * 1000), // 5 hours ago
    audioSource: "Built-in Microphone",
    transcript: "Another sample transcription showing how the system works. This demonstrates the library functionality and how past sessions are stored.",
    summary: "Demo of library functionality and session storage.",
    status: "completed"
  },
  {
    id: 3,
    timestamp: new Date(Date.now() - 24 * 60 * 60 * 1000), // Yesterday
    audioSource: "External Microphone",
    transcript: "A transcription from yesterday. This helps demonstrate the grouping of sessions by time periods like 'Recent' and 'Earlier'.",
    summary: "Example of time-based session grouping.",
    status: "completed"
  }
];

function App() {
  // Core state
  const [appState, setAppState] = useState(AppState.IDLE); // 'idle', 'recording', 'viewing'
  const [microphones, setMicrophones] = useState([]);
  const [selectedMicrophone, setSelectedMicrophone] = useState(null);
  const [showMicModal, setShowMicModal] = useState(false);

  // Recording state
  const [currentTranscript, setCurrentTranscript] = useState("");
  const [elapsedTime, setElapsedTime] = useState(0);
  const [timerInterval, setTimerInterval] = useState(null);

  // Library state
  const [showLibrary, setShowLibrary] = useState(false);
  const [sessions, setSessions] = useState(MOCK_SESSIONS);
  const [selectedSession, setSelectedSession] = useState(null);

  // Load microphones on mount
  useEffect(() => {
    loadMicrophones();
  }, []);

  // Timer effect
  useEffect(() => {
    if (appState === AppState.RECORDING && !timerInterval) {
      const interval = setInterval(() => {
        setElapsedTime(prev => prev + 1);
      }, 1000);
      setTimerInterval(interval);
    } else if (appState !== AppState.RECORDING && timerInterval) {
      clearInterval(timerInterval);
      setTimerInterval(null);
    }

    return () => {
      if (timerInterval) clearInterval(timerInterval);
    };
  }, [appState]);

  async function loadMicrophones() {
    try {
      const devices = await invoke("get_input_devices");
      setMicrophones(devices || []);
      // Auto-select first mic if available
      if (devices && devices.length > 0 && !selectedMicrophone) {
        setSelectedMicrophone(devices[0].name);
      }
    } catch (error) {
      console.error("Failed to load microphones:", error);
      setMicrophones([]);
    }
  }

  function handleRecordClick() {
    // Always show microphone selector modal when clicking Record
    setShowMicModal(true);
  }

  function startRecording() {
    setShowMicModal(false);
    setAppState(AppState.RECORDING);
    setCurrentTranscript("");
    setElapsedTime(0);
    setSelectedSession(null);

    // Simulate transcription (in real app, this would be from Tauri backend)
    setTimeout(() => {
      setCurrentTranscript("This is a simulated transcription. ");
      setTimeout(() => {
        setCurrentTranscript(prev => prev + "The words appear in real-time as you speak. ");
        setTimeout(() => {
          setCurrentTranscript(prev => prev + "The interface remains clean and minimal throughout the process.");
        }, 2000);
      }, 2000);
    }, 1000);
  }

  function handleStopRecording() {
    setAppState(AppState.IDLE);

    // Save session
    if (currentTranscript) {
      const newSession = {
        id: Date.now(),
        timestamp: new Date(),
        audioSource: selectedMicrophone || "Unknown",
        transcript: currentTranscript,
        summary: "AI-generated summary of the transcription...",
        status: "completed"
      };
      setSessions(prev => [newSession, ...prev]);
    }

    setCurrentTranscript("");
    setElapsedTime(0);
  }

  function handleViewSession(session) {
    setSelectedSession(session);
    setAppState(AppState.VIEWING);
    setShowLibrary(false);
  }

  function handleNewRecording() {
    setAppState(AppState.IDLE);
    setSelectedSession(null);
    setCurrentTranscript("");
  }

  function formatTime(seconds) {
    const mins = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
  }

  function formatSessionTime(date) {
    const now = new Date();
    const diff = now - date;
    const hours = Math.floor(diff / (1000 * 60 * 60));

    if (hours < 1) {
      const mins = Math.floor(diff / (1000 * 60));
      return `${mins}m ago`;
    } else if (hours < 24) {
      return date.toLocaleTimeString('en-US', { hour: 'numeric', minute: '2-digit' });
    } else {
      return date.toLocaleDateString('en-US', { month: 'short', day: 'numeric' });
    }
  }

  function groupSessions(sessions) {
    const now = new Date();
    const groups = {
      recent: [],
      earlier: [],
      archive: []
    };

    sessions.forEach(session => {
      const diff = now - session.timestamp;
      const hours = diff / (1000 * 60 * 60);

      if (hours < 6) {
        groups.recent.push(session);
      } else if (hours < 48) {
        groups.earlier.push(session);
      } else {
        groups.archive.push(session);
      }
    });

    return groups;
  }

  const groupedSessions = groupSessions(sessions);

  // Determine what to display
  let displayContent, primaryActionText, primaryActionHandler, showSecondaryAction;

  if (appState === AppState.IDLE) {
    displayContent = <div className="empty-state">Start your first transcription</div>;
    primaryActionText = "Record";
    primaryActionHandler = handleRecordClick;
    showSecondaryAction = true;
  } else if (appState === AppState.RECORDING) {
    displayContent = (
      <div className="transcription-text">
        {currentTranscript || "Listening..."}
      </div>
    );
    primaryActionText = "Stop";
    primaryActionHandler = handleStopRecording;
    showSecondaryAction = false;
  } else if (appState === AppState.VIEWING && selectedSession) {
    const sessionDate = selectedSession.timestamp.toLocaleString('en-US', {
      month: 'short',
      day: 'numeric',
      hour: 'numeric',
      minute: '2-digit'
    });

    displayContent = (
      <>
        <div className="session-meta">{sessionDate} • {selectedSession.audioSource}</div>
        <div className="transcription-text">{selectedSession.transcript}</div>
        <div className="summary-section">
          <div className="summary-header">Summary</div>
          <div className="summary-text">{selectedSession.summary}</div>
          <button className="btn-regenerate">Regenerate Summary</button>
        </div>
      </>
    );
    primaryActionText = "New Recording";
    primaryActionHandler = handleNewRecording;
    showSecondaryAction = false;
  }

  return (
    <div className="app">
      {/* Header */}
      <header className="header">
        <div className="header-left">
          <h1 className="app-title">Lekhan</h1>
          <button className="library-button" onClick={() => setShowLibrary(true)}>
            Library
          </button>
        </div>
        <div className="header-right">
          {appState === AppState.RECORDING && <span className="timer">{formatTime(elapsedTime)}</span>}
          <div className={`status-dot ${appState === AppState.RECORDING ? AppState.RECORDING : ""}`}></div>
        </div>
      </header>

      {/* Content Area */}
      <main className="content-area">
        <div className="content-wrapper fade-in">
          {displayContent}
        </div>
      </main>

      {/* Primary Action */}
      <div className="primary-action">
        <button
          className={`btn-primary ${appState === AppState.RECORDING ? AppState.RECORDING : ""}`}
          onClick={primaryActionHandler}
        >
          {primaryActionText}
        </button>
        {showSecondaryAction && (
          <button className="secondary-link" onClick={() => alert("Upload feature coming soon!")}>
            or upload a file
          </button>
        )}
      </div>

      {/* Library Drawer */}
      {showLibrary && <div className="drawer-overlay visible" onClick={() => setShowLibrary(false)}></div>}
      <div className={`drawer ${showLibrary ? "open" : ""}`}>
        <div className="drawer-header">
          <h2 className="drawer-title">Library</h2>
          <button className="close-button" onClick={() => setShowLibrary(false)}>×</button>
        </div>
        <div className="session-list">
          {groupedSessions.recent.length > 0 && (
            <div className="session-group">
              <div className="session-group-title">Recent</div>
              {groupedSessions.recent.map(session => (
                <div
                  key={session.id}
                  className={`session-item ${selectedSession?.id === session.id ? "active" : ""}`}
                  onClick={() => handleViewSession(session)}
                >
                  <div className="session-time">{formatSessionTime(session.timestamp)}</div>
                  <div className="session-preview">{session.transcript}</div>
                </div>
              ))}
            </div>
          )}

          {groupedSessions.earlier.length > 0 && (
            <div className="session-group">
              <div className="session-group-title">Earlier</div>
              {groupedSessions.earlier.map(session => (
                <div
                  key={session.id}
                  className={`session-item ${selectedSession?.id === session.id ? "active" : ""}`}
                  onClick={() => handleViewSession(session)}
                >
                  <div className="session-time">{formatSessionTime(session.timestamp)}</div>
                  <div className="session-preview">{session.transcript}</div>
                </div>
              ))}
            </div>
          )}

          {groupedSessions.archive.length > 0 && (
            <div className="session-group">
              <div className="session-group-title">Archive</div>
              {groupedSessions.archive.map(session => (
                <div
                  key={session.id}
                  className={`session-item ${selectedSession?.id === session.id ? "active" : ""}`}
                  onClick={() => handleViewSession(session)}
                >
                  <div className="session-time">{formatSessionTime(session.timestamp)}</div>
                  <div className="session-preview">{session.transcript}</div>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>

      {/* Microphone Modal */}
      {showMicModal && (
        <div className="modal-overlay" onClick={() => setShowMicModal(false)}>
          <div className="modal" onClick={(e) => e.stopPropagation()}>
            {/* may be used later */}
            {/*<div className="mic-icon-modal">
              <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                <rect x="9" y="2" width="6" height="11" rx="3" />
                <path d="M5 10v2a7 7 0 0 0 14 0v-2" />
                <line x1="12" y1="19" x2="12" y2="23" />
                <line x1="8" y1="23" x2="16" y2="23" />
              </svg>
            </div>*/}
            <h3 className="modal-title">Select Microphone</h3>
            {microphones.length === 0 ? (
              <div style={{ padding: '24px', textAlign: 'center', color: 'var(--color-text-secondary)' }}>
                No microphones detected. Please check your audio settings.
              </div>
            ) : (
              <ul className="microphone-list">
                {microphones.map((mic, index) => (
                  <li
                    key={index}
                    className={`microphone-item ${selectedMicrophone === mic.name ? "selected" : ""}`}
                    onClick={() => setSelectedMicrophone(mic.name)}
                  >
                    {mic.name || `Microphone ${index + 1}`}
                  </li>
                ))}
              </ul>
            )}
            <div style={{ display: 'flex', gap: '12px', marginTop: '16px' }}>
              <button
                className="btn-primary"
                onClick={() => {
                  setShowMicModal(false);
                  if (appState === AppState.IDLE) {
                    startRecording();
                  }
                }}
                style={{ flex: 1, minWidth: 'auto' }}
              >
                {appState === AppState.IDLE ? "Start Recording" : "Done"}
              </button>
              <button
                className="secondary-link"
                onClick={() => setShowMicModal(false)}
                style={{ padding: '12px 24px' }}
              >
                Cancel
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

export default App;
