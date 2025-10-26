#!/bin/sh

docker stop lekhanai-backend
docker rm lekhanai-backend
docker image remove lekhanai-backend
docker stop lekhanai-ollama
# Remove ollama container if needed
# docker rm lekhanai-ollama