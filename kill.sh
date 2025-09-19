#!/bin/sh

docker stop hikki-backend
docker rm hikki-backend
docker image remove hikki-backend