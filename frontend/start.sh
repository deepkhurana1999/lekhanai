#!/bin/sh

docker build -t lekhanai -f ./docker/Dockerfile.dev .
docker run -dt -v $PWD:/src/frontend --name lekhanai lekhanai
