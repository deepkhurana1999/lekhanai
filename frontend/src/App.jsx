import { useState, useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import "./App.css";

const COMMANDS = {
  GET_INPUT_DEVICES: "get_input_devices"
};

const AppState = {
  IDLE: "idle",
  RECORDING: "recording",
  VIEWING: "viewing",
  UPLOADING: "uploading"
}
const API_BASE_URL = "http://localhost:5000/api";
const PAGE_SIZE = 10;

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
  const [sessions, setSessions] = useState([]);
  const [selectedSession, setSelectedSession] = useState(null);
  const [sessionPage, setSessionPage] = useState(0);
  const [hasMoreSessions, setHasMoreSessions] = useState(true);

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

  // Fetch sessions on mount and when page changes
  useEffect(() => {
    fetchSessions(sessionPage);
  }, [sessionPage]);

  async function loadMicrophones() {
    try {
      const devices = await invoke(COMMANDS.GET_INPUT_DEVICES);
      setMicrophones(devices || []);
      // Auto-select first mic if available
      if (devices && devices.length > 0 && !selectedMicrophone) {
        setSelectedMicrophone(devices[0].name);
      }
    } catch (error) {
      setMicrophones([]);
    }
  }

  async function fetchSessions(page) {
    try {
      const offset = page * PAGE_SIZE;
      const response = await fetch(`${API_BASE_URL}/v1/sessions?limit=${PAGE_SIZE}&offset=${offset}`, {
        method: "GET"
      });
      const data = await response.json();
      // Convert created_at to Date objects if needed
      const parsed = data.map(s => ({
        ...s,
        created_at: new Date(s.created_at)
      }));
      if (page === 0) {
        setSessions(parsed);
      } else {
        setSessions(prev => [...prev, ...parsed]);
      }
      setHasMoreSessions(parsed.length === PAGE_SIZE);
    } catch (error) {
      // fallback: do nothing or show error
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
        created_at: new Date(),
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
      const diff = now - session.created_at;
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
    const sessionDate = selectedSession.created_at.toLocaleString('en-US', {
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
  } else if (appState == AppState.UPLOADING) {
    displayContent = <div className="empty-state">Uploading...</div>;
    primaryActionText = "New Recording";
    primaryActionHandler = handleNewRecording;
    showSecondaryAction = false;
  }

  async function handleUploadFile(event) {
    const file = event.target.files[0];
    if (!file) return;

    setAppState(AppState.UPLOADING);
    setSelectedSession(null);
    setCurrentTranscript("");

    // 1. Create session
    let sessionId = null;
    let session = null;
    try {
      const createResp = await fetch(`${API_BASE_URL}/v1/session/create`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ user_id: "local-user" })
      });
      const createData = await createResp.json();
      sessionId = createData.session_id;
      session = createData;
    } catch (error) {
      setCurrentTranscript("Error creating session: " + error.message);
      setAppState(AppState.VIEWING);
      return;
    }

    // 2. Upload audio and get transcript
    let transcript = "";
    try {
      const formData = new FormData();
      formData.append("file", file);
      formData.append("session_id", sessionId);
      const transcribeResp = await fetch(`${API_BASE_URL}/v1/audio/transcribe`, {
        method: "POST",
        body: formData
      });
      const transcribeData = await transcribeResp.json();
      transcript = transcribeData.text || "No transcription available.";
      setCurrentTranscript(transcript);
      setAppState(AppState.VIEWING);
    } catch (error) {
      setCurrentTranscript("Error transcribing audio: " + error.message);
      setAppState(AppState.VIEWING);
      return;
    }

    setCurrentTranscript(transcript);

    // 3. Update session with transcript
    try {
      await fetch(`${API_BASE_URL}/v1/session/${sessionId}`, {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ transcript, session_id: sessionId })
      });
    } catch (error) {
      // Optionally show error, but don't block UI
      return;
    }

    // Optionally, save session locally
    const newSession = {
      ...session,
      transcript,
      created_at: new Date(),
      audioSource: "Uploaded File",
      summary: "No summary.",
      status: "completed"
    };
    setSessions(prev => [newSession, ...prev]);
    handleViewSession(newSession);
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
          <>
            <label className="secondary-link" htmlFor="audio-upload-input" style={{ cursor: "pointer" }}>
              or upload a file
            </label>
            <input
              id="audio-upload-input"
              type="file"
              accept="audio/*"
              style={{ display: "none" }}
              onChange={handleUploadFile}
            />
          </>
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
                  key={session.session_id}
                  className={`session-item ${selectedSession?.session_id === session.session_id ? "active" : ""}`}
                  onClick={() => handleViewSession(session)}
                >
                  <div className="session-time">{formatSessionTime(session.created_at)}</div>
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
                  key={session.session_id}
                  className={`session-item ${selectedSession?.session_id === session.session_id ? "active" : ""}`}
                  onClick={() => handleViewSession(session)}
                >
                  <div className="session-time">{formatSessionTime(session.created_at)}</div>
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
                  key={session.session_id}
                  className={`session-item ${selectedSession?.session_id === session.session_id ? "active" : ""}`}
                  onClick={() => handleViewSession(session)}
                >
                  <div className="session-time">{formatSessionTime(session.created_at)}</div>
                  <div className="session-preview">{session.transcript}</div>
                </div>
              ))}
            </div>
          )}
        </div>
        <div className="pagination-controls">
          <button disabled={sessionPage === 0} onClick={() => setSessionPage(p => Math.max(0, p - 1))}>Previous</button>
          <button disabled={!hasMoreSessions} onClick={() => setSessionPage(p => p + 1)}>Next</button>
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
                  if (appState === AppState.session_idLE) {
                    startRecording();
                  }
                }}
                style={{ flex: 1, minWidth: 'auto' }}
              >
                {appState === AppState.session_idLE ? "Start Recording" : "Done"}
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
