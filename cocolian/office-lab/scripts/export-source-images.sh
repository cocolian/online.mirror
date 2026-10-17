#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
build="$root/build/source-19b4efae7fb8"
sha=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["commit"])' "$build/source-lock.json")
test -f "$build/stage/usr/bin/coolwsd"
test -f "$build/stage/opt/collaboraoffice/program/soffice.bin"
mkdir -p "$root/artifacts" "$build/runtime-tools"
cp "$root/deploy/source-build/source-conversion-service.py" "$build/runtime-tools/"
components=("$@")
if test "${#components[@]}" -eq 0; then components=(collabora libreoffice); fi
for component in "${components[@]}"; do
  case "$component" in collabora|libreoffice) ;; *) echo "Unsupported component" >&2; exit 1 ;; esac
  recipe=Runtime.Dockerfile
  if test "$component" = libreoffice; then recipe=Conversion.Dockerfile; fi
  image="docker.io/cocolian/$component-source:src-${sha:0:12}"
  archive="$root/artifacts/$component-source-${sha:0:12}.tar"
  test ! -e "$archive" || { echo "Refusing existing archive $archive" >&2; exit 1; }
  raw="$archive.raw"
  docker buildx build --platform linux/amd64 --network=host \
    --build-arg http_proxy=http://192.168.124.1:7770 \
    --build-arg https_proxy=http://192.168.124.1:7770 \
    --build-arg "SOURCE_COMMIT=$sha" --provenance=false --sbom=false \
    --output "type=oci,name=$image,dest=$raw" \
    -f "$root/deploy/source-build/$recipe" "$build"
  python3 "$root/scripts/normalize-source-oci.py" "$raw" "$archive" "$image" > "$archive.json"
done
