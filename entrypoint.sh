#!/bin/sh

chmod +x /app/backend/sqlite_entrypoint.sh
chmod +x /app/backend/doxygen_entrypoint.sh

/app/backend/sqlite_entrypoint.sh
cd /app/backend/
/app/backend/doxygen_entrypoint.sh
mv html docs
cd /app/

exec lighttpd -D -f /app/web/lighttpd.conf
