# The base contains the existing pinned source-built LibreOffice engine.
FROM cocolian/libreoffice-source:src-19b4efae7fb8
COPY --chmod=755 source-conversion-service.py /usr/local/bin/source-conversion-service.py
