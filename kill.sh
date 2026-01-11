#!/bin/sh

docker stop lekhanai-stt
docker rm lekhanai-stt
docker image remove lekhanai-stt
docker stop lekhanai-ollama
# Remove ollama container if needed
# docker rm lekhanai-ollama