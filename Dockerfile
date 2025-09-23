FROM alpine:latest

# Install dependencies
RUN apk add --no-cache lighttpd tcc sqlite sqlite-dev musl-dev ninja tcc-libs-static curl unzip

WORKDIR /app

# Copy web + backend
COPY web/ ./web/
COPY backend/ ./backend/

# Add Pico.css and Alpine.js locally
WORKDIR /app/web/assets
RUN mkdir -p . \
    && curl -fsSL https://github.com/picocss/pico/archive/refs/heads/main.zip -o pico.zip \
    && unzip pico.zip \
    && cp pico-main/css/pico.min.css ./pico.min.css \
    && rm -rf pico.zip pico-main \
    && curl -fsSL https://cdn.jsdelivr.net/npm/alpinejs@3.x.x/dist/cdn.min.js -o alpine.min.js

RUN mkdir -p /tmp/lighttpd/deflate && chmod 1777 /tmp/lighttpd/deflate

# Build backend with Ninja
WORKDIR /app/backend
RUN ninja -C .

# Copy lighttpd config
WORKDIR /app/web
COPY web/lighttpd.conf ./lighttpd.conf

# Create SQLite DB and table, then set permissions
RUN mkdir -p /data && \
    sqlite3 /data/alpsc.db "CREATE TABLE IF NOT EXISTS visits (id INTEGER PRIMARY KEY AUTOINCREMENT, ts DATETIME DEFAULT CURRENT_TIMESTAMP);" && \
    chown -R nobody:nogroup /data && \
    chmod -R 777 /data


EXPOSE 8080
CMD ["lighttpd", "-D", "-f", "lighttpd.conf"]

