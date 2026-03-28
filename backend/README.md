# Lekhanai Backend Microservices

## Structure

- `backend/stt/` : C++ VAD and Whisper STT services
- `backend/nlp/` : Python NLU + Post-processing (intent, entity, sentiment, context, summary, confidence)
- `backend/api/` : Python API Gateway, Audio Ingestion, Session Manager

## Quick Start

1. Build and run with Docker Compose:
   ```bash
   docker-compose up --build
   ```
2. API Gateway: http://localhost:5000
3. NLP Service: http://localhost:8004
4. STT Service: http://localhost:8080

## .env Template

Example for backend/stt/template.env:
```
REDIS_URL=redis://localhost:6379
DATABASE_URL=postgresql://user:pass@localhost:5432/stt_db
MINIO_ENDPOINT=localhost:9000
WHISPER_MODEL_PATH=/models/ggml-base.en.bin
OPENBLAS_NUM_THREADS=4
```
