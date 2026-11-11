# Handoff: Agrisoil on Kubernetes + new multi-sensor dashboard

You (Claude) are working **alone overnight**. The owner is asleep and has granted the access you need.
Read this whole file before acting.

Repo: `Aditya-BN/vibecoded-TA-revisit`, branch `claude/npk-vibecoded-rewrite-20ognj`.
Firmware (source of truth for the data): `agrisoil/send_mqtt.ino`, `send_firebase.ino`, `send_mysql.ino`.
Dummy sensors flow: `docs/nodered-dummy-flow.json`.

---

## 0. Ground rules (read twice)

**Work without asking**
- Don't stop to ask questions. Pick sensible defaults and keep going.
- Write every non-obvious choice in `docs/DECISIONS.md`: one line for what you chose, one for why.
- If something is really blocked (missing credential, no cluster access), skip it, write down exactly what's needed, and carry on with everything else.

**Stay in your own space**
- Everything goes in a new namespace, **`agrisoil-dev`**.
- Don't modify or delete anything outside it: no other namespaces, no cluster-wide settings, no existing ingress controller or cert-manager config.
- Only *add* Ingress objects in your own namespace.

**No costs**
- Don't create anything that adds cost: no new cloud LoadBalancer Services, no managed databases, no extra nodes.
- Reuse the cluster's existing ingress.
- Keep each PersistentVolumeClaim at 1 Gi or less.
- Before using the default StorageClass, check whether it's a paid cloud disk, and note it.

**No production data, no committed secrets**
- Don't touch production data: the existing Firebase database, the `sistempemetaan2023.com` MySQL, or the existing `nodered.agrisoil.my.id`.
- The old Firebase secret in the `npkgis` repo is public. Never use it.
- Never commit secrets. Use Kubernetes Secrets, generate random passwords, and record **where** each one is stored (not its value) in the morning report.

**Ship it on a branch**
- Everything as code in this repo: manifests in `deploy/k8s/` (Kustomize), the dashboard app in `dashboard/`.
- Commit often to the branch above and push. Open a PR at the end, but **don't merge it**.

---

## 1. The data contract (what a real sensor sends)

Each sensor has a `DEVICE_ID` (e.g. `Alat3`, letters/numbers/underscore). It wakes up, measures, publishes, then deep sleeps.

### MQTT, all under `agrisoil/<DEVICE_ID>/`

| Topic | Payload | Notes |
|---|---|---|
| `agrisoil/<id>/reading` | JSON, see below | **retained**, the main thing to consume |
| `agrisoil/<id>/val/nit` · `phos` · `kal` | `"201.59"` | N, P, K in ppm, plain text |
| `agrisoil/<id>/val/ph` · `moist` | `"6.57"` · `"48.01"` | pH, moisture % |
| `agrisoil/<id>/raw/npk1` · `npk2` · `moist` · `ph` | `"5666"` | raw ADC counts |
| `agrisoil/<id>/loc/lat` · `lng` | `"-6.199969"` | only when GPS has a fix |

```json
{"device":"Alat3","waktu":"2026-11-11 12:23:08","n":201.59,"p":13.99,"k":125.94,
 "ph":6.57,"moist":48.01,"npk1_raw":5666,"npk2_raw":5659,"moist_raw":8454,"ph_raw":309,
 "in_soil":true,"lat":-6.199969,"lng":106.816642}
```

- `waktu` is local time, WIB (UTC+7), or `"unknown"` when NTP failed. In that case, use the broker receive time.
- `lat` and `lng` are `null` when there's no GPS fix.
- MQTT client id is `Agrisoil_<id>`, QoS 0.

### MQTT port lookup
- The firmware calls `GET <node-red>/getngrokport` with HTTP basic auth.
- It expects plain text with just the broker port, e.g. `12115`.
- Keep serving this endpoint from Node-RED, returning the port the ESP32 should use to reach the broker. Make the value configurable by env or ConfigMap.

### Firebase and MySQL (legacy)
The device also writes `/<id>_rt`, `/<id>` to Firebase and POSTs a form to the MySQL dashboard with an extra `device_id` field. **Out of scope tonight.** The new stack consumes MQTT only.

### Realistic ranges

| N ppm | P ppm | K ppm | pH | Moisture % |
|---|---|---|---|---|
| 60–350 | 5–30 | 70–170 | 5.0–8.0 | 20–90 |

---

## 2. Target architecture (all in `agrisoil-dev`)

```
dummy sensors (Node-RED flow) ─┐
real ESP32 sensors ────────────┼─► Mosquitto ─► Node-RED ingest ─► PostgreSQL ─► Dashboard (web)
                               │   (retained)   agrisoil/+/reading   readings      auto-lists sensors
                               └─ GET /getngrokport (Node-RED)       devices
```

| Piece | How |
|---|---|
| **Mosquitto** | Deployment, 1 replica, small PVC (keeps retained messages), ClusterIP Service `mosquitto:1883`, username/password from a Secret. For real ESP32s outside the cluster, reuse whatever TCP exposure the cluster already has. Otherwise document the options in the report and don't create a paid LoadBalancer. |
| **Node-RED** | StatefulSet or Deployment + small PVC, Service, Ingress. `adminAuth` and `httpNodeAuth` from a Secret. Import `docs/nodered-dummy-flow.json`; its broker node already points at host `mosquitto`. |
| **Ingest flow** (new, in Node-RED) | Subscribe `agrisoil/+/reading`. Parse the JSON. Upsert the device into `devices`, insert into `readings`. Handle bad JSON without crashing. If you ever run more than one Node-RED replica, use a shared subscription (`$share/ingest/agrisoil/+/reading`) so readings aren't stored twice. |
| **PostgreSQL** | StatefulSet, 1 replica, small PVC. Tables below. Credentials from a Secret. |
| **Dashboard** | New stateless web app in `dashboard/` with its own small API that reads Postgres. Deployment (2 replicas), Service, Ingress. Your choice of stack; keep it simple and nice. |

```sql
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
CREATE INDEX ON readings (device_id, ts DESC);
```

---

## 3. "Scalable": a new sensor gets its own dashboard automatically

This is the main requirement:
- **Zero per-sensor config.** A new `DEVICE_ID` that publishes once must appear on the dashboard by itself. No redeploy, no code change, no manual DB row.
- **Overview page:** a card per sensor with the latest N/P/K/pH/moisture, last seen (+ an "offline" state after a configurable time), and a map with every sensor's last location.
- **Sensor page** at `/devices/<id>`: latest values, history charts per value with a time-range picker (1 h / 24 h / 7 d), raw ADC values in a collapsible section (for calibration), and a location map.
- **Stateless dashboard and API,** so more replicas can be added. Postgres is the only state.
- **Live updates** are a nice-to-have, via polling or a WebSocket or SSE fed from MQTT.
- The UI language can mix Indonesian and English (the owner's style), but values always show units.

---

## 4. Definition of done (verify, don't assume)

1. `kubectl -n agrisoil-dev get pods`: everything Running and Ready.
2. `mosquitto_sub -t 'agrisoil/#' -v` (from a pod) shows the 3 dummy sensors `Alat1..3` every 10 s.
3. `readings` and `devices` fill up in Postgres.
4. `GET /getngrokport` with basic auth returns a plain port number; without auth it returns 401.
5. The dashboard shows 3 sensors with charts.
6. **Scalability test:** change the dummy flow's `AGRI_DEVICES` to add `Alat4`. It appears on the dashboard with no other change.
7. Restart each pod once. Data and retained messages survive.
8. A screenshot of the overview and of one sensor page, saved in `docs/screenshots/`.

---

## 5. Morning report

When finished (or when time runs out), write `docs/MORNING-REPORT.md`, push it, and make it your final message. Short, phone-readable:
- What's live: URLs, and which Secret holds each login (no values).
- Definition-of-done checklist: ✅ / ❌ with one line each.
- How to add a real sensor: set `DEVICE_ID` in `agrisoil/agrisoil.ino` and point `MQTT_HOST` / the port lookup at the new broker.
- What's left or blocked, and what the owner has to decide.
- The PR link.
