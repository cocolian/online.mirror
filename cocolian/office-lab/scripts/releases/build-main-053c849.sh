#!/usr/bin/env bash
set -euo pipefail
root=/home/codex/projects/cocolian-office
out="$root/artifacts/main-053c849-20261020"
program="$root/build/source-19b4efae7fb8/engine/instdir/program"
mkdir -p "$out/image/program"
for lib in libmergedlo.so libswlo.so libsw_writerfilterlo.so; do
 test "$(stat -c %s "$program/$lib")" -gt 1000000
 test "$(od -An -tx1 -N4 "$program/$lib" | tr -d " ")" = 7f454c46
 cp "$program/$lib" "$out/image/program/"
done
cat > "$out/image/Dockerfile" <<'DOCKER'
ARG BASE
FROM ${BASE}
USER root
COPY program/ /opt/collaboraoffice/program/
RUN coolwsd-systemplate-setup /opt/cool/systemplate /opt/collaboraoffice && chown -R 1001:1001 /opt/cool/systemplate
LABEL org.opencontainers.image.revision="053c8491238" org.opencontainers.image.source="https://github.com/cocolian/online.mirror"
USER 1001:1001
DOCKER
cat > "$out/image/Conversion.Dockerfile" <<'DOCKER'
ARG BASE
FROM ${BASE}
USER root
COPY program/ /opt/collaboraoffice/program/
LABEL org.opencontainers.image.revision="053c8491238" org.opencontainers.image.source="https://github.com/cocolian/online.mirror"
USER 1001:1001
DOCKER
for component in collabora-source collabora-wordmac libreoffice-source; do
 case "$component" in
 collabora-source) base=cocolian/collabora-source:editor-aids-20261020; recipe=Dockerfile;;
 collabora-wordmac) base=cocolian/collabora-wordmac:editor-aids-20261020; recipe=Dockerfile;;
 libreoffice-source) base=cocolian/libreoffice-source:fonts-ui-20261018; recipe=Conversion.Dockerfile;;
 esac
 image="cocolian/$component:main-053c849-20261020"
 docker build --network=none --build-arg BASE="$base" -f "$out/image/$recipe" -t "$image" "$out/image"
 docker save "$image" -o "$out/$component.docker.tar"
 skopeo copy "docker-archive:$out/$component.docker.tar" "oci-archive:$out/$component.oci.tar"
 skopeo inspect --format '{{.Digest}}' "oci-archive:$out/$component.oci.tar" > "$out/$component.digest"
done
