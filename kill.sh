#!/bin/sh

docker stop lekhanai-stt
docker stop lekhanai-manager
docker stop lekhanai-ollama

docker rm lekhanai-stt
docker rm lekhanai-manager
docker image remove lekhanai-stt
docker image remove lekhanai-manager
# Remove ollama container if needed
# docker rm lekhanai-ollama