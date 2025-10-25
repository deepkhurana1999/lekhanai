#!/bin/sh

docker compose up -d && docker exec -it lekhanai-ollama ollama pull qwen3:1.7b