ARG BASE_IMAGE
FROM ${BASE_IMAGE}
USER root
COPY fonts/ /usr/local/share/fonts/source-han-sc/
RUN chmod -R a+rX /usr/local/share/fonts/source-han-sc && \
 mkdir -p /opt/cool/systemplate/usr/local/share/fonts && \
 cp -a /usr/local/share/fonts/source-han-sc /opt/cool/systemplate/usr/local/share/fonts/ && \
 fc-cache -f && chown -R 1001:1001 /opt/cool/systemplate/usr/local/share/fonts/source-han-sc
USER 1001:1001
