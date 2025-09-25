# --- Stage 1: Builder ---
# This stage is for all the build tools and dependencies.
FROM alpine:latest AS builder

RUN apk add --no-cache \
    tcc \
    sqlite-dev \
    musl-dev \
    ninja \
    tcc-libs-static \
    unzip \
    json-c-dev \
    doxygen \
    openssl-dev \
    git \
    curl \
    && rm -rf /var/cache/apk/*

WORKDIR /app

# Create the symbolic link for 'cc'.
RUN ln -s /usr/bin/tcc /usr/bin/cc

# Clone and build the external jwtc library.
RUN git clone https://github.com/KrzysztofMarciniak/jwtc.git /app/jwtc \
    && cd /app/jwtc \
    && chmod +x ./install.sh \
    && ./install.sh

# --- Stage 2: Frontend ---
# This stage handles all frontend assets, including Pico.css and Alpine.js.
FROM alpine:latest AS frontend

RUN apk add --no-cache curl unzip && rm -rf /var/cache/apk/*

WORKDIR /app/web/assets
RUN mkdir -p . \
    && curl -fsSL https://github.com/picocss/pico/archive/refs/heads/main.zip -o pico.zip \
    && unzip pico.zip \
    && cp pico-main/css/pico.min.css ./pico.min.css \
    && rm -rf pico.zip pico-main \
    && curl -fsSL https://cdn.jsdelivr.net/npm/alpinejs@3.x.x/dist/cdn.min.js -o alpine.min.js

# Copy web files after frontend dependencies are handled.
COPY web/ ./web/

# --- Stage 3: Compiler ---
# This stage compiles the backend using the environment from the 'builder' stage.
FROM builder AS compiler

# Copy the backend source code into this stage.
COPY backend/ ./backend/
COPY backend/generate_build.sh /app/backend/generate_build.sh

WORKDIR /app/backend

RUN chmod +x generate_build.sh \
    && ./generate_build.sh \
    && ninja -C .

# --- Stage 4: Runtime ---
# This is the final, minimal image with all the required components.
FROM alpine:latest

# Install only the necessary runtime dependencies.
RUN apk add --no-cache \
    lighttpd \
    tcc-libs-static \
    sqlite \
    && rm -rf /var/cache/apk/*

WORKDIR /app

# Copy the backend binary from the 'compiler' stage.
COPY --from=compiler /app/backend /app/backend

# Copy the compiled jwtc library.
COPY --from=compiler /app/jwtc/libjwtc.so /usr/lib/

# Copy the frontend assets from the 'frontend' stage.
COPY --from=frontend /app/web/ /app/web/

# Set up the lighttpd directory.
RUN mkdir -p /tmp/lighttpd/deflate && chmod 1777 /tmp/lighttpd/deflate

# Copy configuration and entrypoint scripts.
COPY .secrets .secrets
COPY web/lighttpd.conf /app/web/lighttpd.conf
WORKDIR /app
COPY entrypoint.sh backend/doxygen_entrypoint.sh backend/sqlite_entrypoint.sh ./
RUN chmod +x entrypoint.sh backend/sqlite_entrypoint.sh

EXPOSE 8080

CMD ["/app/entrypoint.sh"]
