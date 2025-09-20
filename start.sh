#!/bin/sh

docker build -t hikki-backend -f ./backend/docker/Dockerfile.dev .
docker run -p 8080:8080 -dt --name hikki-backend hikki-backend