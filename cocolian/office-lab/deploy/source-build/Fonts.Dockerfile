ARG BASE_IMAGE
FROM ${BASE_IMAGE}
USER root
COPY fonts/ /usr/local/share/fonts/source-han-sc/
RUN chmod -R a+rX /usr/local/share/fonts/source-han-sc && fc-cache -f
USER 1001:1001
