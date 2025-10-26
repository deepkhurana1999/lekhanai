# lekhanai

LekhanAI is a open-source AI note taker.

## Project Structure

- `backend/` - Backend source code, libraries, and server logic
- `frontend/` - Frontend code and related scripts
- `docker-compose.yml` - Docker Compose configuration for multi-container setup
- `start.sh`, `kill.sh` - Helper scripts to start and stop services

## Getting Started

### Prerequisites
- Docker & Docker Compose
- CMake (for backend build)
- (Optional) Python, Node.js for development tools

### Running with Docker Compose

```bash
docker-compose up --build
```

### Building Backend Manually

```bash
cd backend
mkdir build && cd build
cmake ..
make
```

### Starting Frontend

Refer to `frontend/index.html` to run simplified frontend.

## Contributing
See [Contributing.md](Contributing.md) for guidelines.

## License
See [LICENSE](LICENSE) for details.

## Third-Party Libraries
### Ollama
- **Purpose**: Large language model provider for transcript summarization
- **Integration**: Used as the backend provider for summary generation (requires running Ollama server separately)
- **License**: [MIT](https://github.com/ollama/ollama/blob/main/LICENSE)

### whisper.cpp
- **Purpose**: Fast, local speech-to-text engine
- **Integration**: Added as a git submodule (`libraries/whisper.cpp`), built via CMake
- **Branch**: v1.8.0_parallel (A modified version to handle)
- **License**: [MIT](https://github.com/ggml-org/whisper.cpp/blob/master/LICENSE)

### silero-vad
- **Purpose**: Voice Activity Detection (VAD)
- **Integration**: Silero VAD ONNX model downloaded at build time (`/models/vad/silero_vad.onnx`)
- **License**: [Apache 2.0](https://github.com/snakers4/silero-vad/blob/master/LICENSE)

### Ollama provider (ollama-hpp)
- **Purpose**: Transcript summarization
- **Integration**: Added as a git submodule (`libraries/ollama-hpp`), used for summary processing
- **Version**: v0.9.7
- **License**: [MIT](https://github.com/jmont-dev/ollama-hpp/blob/master/LICENSE)

### libdatachannel
- **Purpose**: WebRTC data channel library
- **Integration**: Added as a git submodule (`libraries/libdatachannel`), built via CMake
- **Version**: v0.23.2
- **License**: [MIT](https://github.com/paullouisageneau/libdatachannel/blob/master/LICENSE)

### cpprestsdk
- **Purpose**: REST API client/server
- **Integration**: Installed via system package (`libcpprest-dev`), linked in backend
- **License**: [MIT](https://github.com/microsoft/cpprestsdk/blob/master/LICENSE)

### nlohmann_json
- **Purpose**: JSON for Modern C++
- **Integration**: Installed via system package (`nlohmann-json3-dev`), linked in backend
- **License**: [MIT](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT)

### onnxruntime
- **Purpose**: ONNX model runtime (used for Silero VAD)
- **Integration**: Installed via system package (`libonnxruntime-dev`) or Python pip fallback
- **License**: [MIT](https://github.com/microsoft/onnxruntime/blob/main/LICENSE)

See individual library folders and upstream repositories for more details.
