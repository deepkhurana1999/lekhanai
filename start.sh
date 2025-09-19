#!/bin/sh

docker build -t hikki-backend -f ./backend/docker/Dockerfile.dev .
docker run -dt --name hikki-backend hikki-backend