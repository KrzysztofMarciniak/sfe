#!/bin/sh

DB_PATH="/data/sfe.db"

mkdir -p /data

sqlite3 "$DB_PATH" <<EOF
-- Users table
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    password TEXT NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
EOF
