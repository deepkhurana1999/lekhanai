import { useState } from 'react';
import './AudioSourceSelector.css';

function AudioSourceSelector({ devices = [], onDeviceSelect, selectedDevice }) {
    return (
        <div className="audio-selector glass-card slide-in">
            <h3 className="selector-title">Audio Source</h3>
            <select
                className="select"
                value={selectedDevice || ''}
                onChange={(e) => onDeviceSelect(e.target.value)}
            >
                <option value="">Select a microphone...</option>
                {devices.map((device, index) => (
                    <option key={index} value={device.name}>
                        {device.name || `Device ${index + 1}`}
                    </option>
                ))}
            </select>
            {devices.length === 0 && (
                <p className="no-devices">No audio devices found</p>
            )}
        </div>
    );
}

export default AudioSourceSelector;
