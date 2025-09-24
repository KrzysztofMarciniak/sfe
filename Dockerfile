FROM alpine:latest

RUN apk add --no-cache lighttpd tcc sqlite sqlite-dev musl-dev ninja tcc-libs-static curl unzip json-c-dev check-dev doxygen

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
COPY backend/generate_build.sh /app/backend/generate_build.sh
RUN chmod +x /app/backend/generate_build.sh \
    && ./generate_build.sh \
    && ninja -C .

# Copy lighttpd config
WORKDIR /app/web
COPY web/lighttpd.conf ./lighttpd.conf

COPY entrypoint.sh /app/entrypoint.sh
COPY backend/doxygen_entrypoint.sh /app/backend/doxygen_entrypoint.sh
COPY backend/sqlite_entrypoint.sh /app/backend/sqlite_entrypoint.sh

RUN chmod +x /app/entrypoint.sh /app/backend/sqlite_entrypoint.sh

EXPOSE 8080

CMD ["/app/entrypoint.sh"]
