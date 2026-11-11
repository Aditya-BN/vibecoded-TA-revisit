-- Runs once, when the Postgres volume is empty.
CREATE TABLE devices (
  id          text PRIMARY KEY,           -- DEVICE_ID
  name        text,                       -- editable display name, defaults to id
  first_seen  timestamptz NOT NULL DEFAULT now(),
  last_seen   timestamptz NOT NULL DEFAULT now(),
  last_lat    double precision,
  last_lng    double precision
);
CREATE TABLE readings (
  id bigserial PRIMARY KEY,
  device_id text NOT NULL REFERENCES devices(id),
  ts timestamptz NOT NULL,                -- from waktu (WIB), or receive time
  n real, p real, k real, ph real, moist real,
  npk1_raw int, npk2_raw int, moist_raw int, ph_raw int,
  in_soil boolean, lat double precision, lng double precision
);
-- UNIQUE so a retained message re-delivered after a Node-RED restart is not stored twice.
CREATE UNIQUE INDEX readings_device_ts ON readings (device_id, ts DESC);
