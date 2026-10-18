FROM debian:trixie-slim@sha256:a29215f6a35e51e22adffa17f89e9d2ef06214e64a2bad10d765c46aea49f11f
ARG SOURCE_COMMIT
LABEL org.opencontainers.image.source="https://github.com/cocolian/online.mirror" org.opencontainers.image.revision=$SOURCE_COMMIT
ENV DEBIAN_FRONTEND=noninteractive LANG=C.UTF-8 LC_ALL=C.UTF-8
RUN sed -i 's|deb.debian.org/debian|mirrors.ustc.edu.cn/debian|g' /etc/apt/sources.list.d/debian.sources && apt-get update && apt-get install -y --no-install-recommends \
 ca-certificates fontconfig fonts-noto-cjk fonts-dejavu-core cpio findutils \
 libcap2-bin libpam0g libssl3t64 libzstd1 libpng16-16t64 libstdc++6 libgcc-s1 \
 libx11-6 libxext6 libxrender1 libxcb1 libxcb-render0 libxcb-shm0 \
 libfontconfig1 libfreetype6 libexpat1 zlib1g libuuid1 libkrb5-3 \
 libgssapi-krb5-2 openssl tzdata curl && rm -rf /var/lib/apt/lists/*
RUN groupadd -g 1001 cool && useradd -u 1001 -g 1001 -d /opt/cool -m cool
COPY stage/ /
COPY fonts/ /usr/local/share/fonts/office-fonts/
RUN mkdir -p /opt/cool/child-roots /opt/cool/cache && \
 coolwsd-systemplate-setup /opt/cool/systemplate /opt/collaboraoffice && \
 touch /var/log/coolwsd.log && \
 chown -R 1001:1001 /opt/cool /etc/coolwsd /var/log/coolwsd.log && \
 fc-cache -f
USER 1001:1001
EXPOSE 9980
ENTRYPOINT ["/usr/bin/coolwsd", "--use-env-vars", "--o:sys_template_path=/opt/cool/systemplate", "--o:child_root_path=/opt/cool/child-roots", "--o:file_server_root_path=/usr/share/coolwsd", "--o:cache_files.path=/opt/cool/cache", "--o:logging.color=false", "--o:stop_on_config_change=true"]
