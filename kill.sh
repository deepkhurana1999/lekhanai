#!/bin/sh

docker stop lekhanai-backend
docker rm lekhanai-backend
docker image remove lekhanai-backend
docker stop lekhanai-ollama
docker rm lekhanai-ollama