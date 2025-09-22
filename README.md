# srotalekh

srotalekh is a open-source transcriber, using whisper.cpp.

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

Refer to `frontend/start.sh` for frontend startup instructions.

## Contributing
See [Contributing.md](Contributing.md) for guidelines.

## License
See [LICENSE](LICENSE) for details.
