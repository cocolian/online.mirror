#!/usr/bin/env bash
set -euo pipefail
cd /workspace
mkdir -p engine stage logs cache
export CCACHE_DIR=/workspace/cache/ccache
export CCACHE_MAXSIZE=8G
printf '%s\n' ENGINE_CONFIGURE
cd engine
../source/engine/autogen.sh --with-distro=CPLinux-LOKit \
 --disable-epm --without-package-format --disable-symbols \
 --without-java --disable-python \
 --without-myspell-dicts --without-fonts --disable-noto-font \
 --with-lang='en-US zh-CN' --with-parallelism=3
printf '%s\n' ENGINE_COMPILE
make -j3 build
printf '%s\n' ENGINE_BUILD_COMPLETE
cd /workspace/source
./autogen.sh
./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var \
 --disable-tests --enable-experimental --with-lo-builddir=/workspace/engine \
 --with-lokit-path=/workspace/source/engine/include --with-lo-path=/opt/collaboraoffice
printf '%s\n' ONLINE_COMPILE
make -j3
DESTDIR=/workspace/stage make install
mkdir -p /workspace/stage/opt
cp -a /workspace/engine/instdir /workspace/stage/opt/collaboraoffice
printf '%s\n' ALL_SOURCE_BUILDS_COMPLETE
