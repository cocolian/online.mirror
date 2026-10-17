#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
build="$root/build/source-19b4efae7fb8"
image=cocolian-office-builder:debian13
name=cocolian-source-19b4efae7fb8
if docker container inspect "$name" >/dev/null 2>&1; then
  echo "Container exists; inspect its state and resume with docker start -a $name" >&2
  exit 1
fi
docker run --name "$name" --network=host --cpus=3 --memory=12g \
  --user "$(id -u):$(id -g)" \
  -e http_proxy=http://192.168.124.1:7770 -e https_proxy=http://192.168.124.1:7770 \
  -e HTTP_PROXY=http://192.168.124.1:7770 -e HTTPS_PROXY=http://192.168.124.1:7770 \
  -e no_proxy=localhost,127.0.0.1,192.168.124.1 \
  -v "$build:/workspace" "$image" bash /workspace/compile.sh \
  2>&1 | tee "$build/logs/compile.log"
