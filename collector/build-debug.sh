#!/bin/sh

docker build --progress=plain --no-cache .
# docker run -it --rm debian:buster-slim bash