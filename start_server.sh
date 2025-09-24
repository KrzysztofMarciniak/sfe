#!/bin/sh

set -e

docker build -t sfe .

if docker ps -a --format '{{.Names}}' | grep -q "^sfe$"; then
  docker rm -f sfe
fi

docker run -d --name sfe -p 8080:8080 sfe

