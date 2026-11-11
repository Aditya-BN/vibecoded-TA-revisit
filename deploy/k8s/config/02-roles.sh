#!/bin/sh
# Runs once, after 01-schema.sql. Read-mostly login for the dashboard:
# it can read everything and only change a device's display name.
set -eu
psql -v ON_ERROR_STOP=1 -U "$POSTGRES_USER" -d "$POSTGRES_DB" -v pw="$DASHBOARD_DB_PASSWORD" <<'SQL'
CREATE ROLE dashboard LOGIN PASSWORD :'pw';
GRANT CONNECT ON DATABASE agrisoil TO dashboard;
GRANT USAGE ON SCHEMA public TO dashboard;
GRANT SELECT ON devices, readings TO dashboard;
GRANT UPDATE (name) ON devices TO dashboard;
SQL
