#!/bin/sh

docker build -t srotalekh -f ./docker/Dockerfile.dev .
docker run -dt -v $PWD:/src/frontend --name srotalekh srotalekh