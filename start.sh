#!/bin/sh

docker compose up -d
sleep 10
docker exec -it lekhanai-ollama ollama pull qwen3:8b-q4_K_M