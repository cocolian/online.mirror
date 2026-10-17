FROM debian:trixie-slim@sha256:a29215f6a35e51e22adffa17f89e9d2ef06214e64a2bad10d765c46aea49f11f
ENV DEBIAN_FRONTEND=noninteractive
RUN sed -i 's|deb.debian.org/debian|mirrors.ustc.edu.cn/debian|g' /etc/apt/sources.list.d/debian.sources && apt-get update && apt-get install -y --no-install-recommends \
 build-essential autoconf automake libtool pkg-config git ccache gperf nasm \
 flex bison zip unzip rsync curl wget ca-certificates bash perl python3 \
 python3-dev python3-lxml python3-polib meson ninja-build gettext xz-utils bzip2 \
 libx11-dev libxrender-dev libxext-dev libxrandr-dev libxinerama-dev \
 libfontconfig-dev libfreetype6-dev zlib1g-dev libpng-dev libssl-dev libcap-dev \
 libpam0g-dev libzstd-dev libcppunit-dev libkrb5-dev uuid-dev libexpat1-dev \
 nodejs npm cpio file patch locales && rm -rf /var/lib/apt/lists/*
RUN useradd -m -u 1000 builder
USER builder
ENV LANG=C.UTF-8 LC_ALL=C.UTF-8
WORKDIR /workspace
