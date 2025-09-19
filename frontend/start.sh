#!/bin/sh

docker build -t hikki -f ./docker/Dockerfile.dev .
docker run -dt -v $PWD:/src/frontend --name hikki hikki