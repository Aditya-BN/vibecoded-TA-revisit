# Agrisoil: Node-RED dummy data brief

Context for a new session: deploy Node-RED on my infra, make it generate fake agrisoil
readings, then remake the dashboard on top of that data.

Firmware source of truth: `Aditya-BN/vibecoded-TA-revisit`, branch `claude/npk-vibecoded-rewrite-20ognj`,
folder `agrisoil/` (`send_mqtt.ino`, `send_firebase.ino`, `send_mysql.ino`).
Ready-to-import flow: `docs/nodered-dummy-flow.json` (same branch).

## 1. What the real device sends (the data contract)
The device wakes up, measures, sends everything once, then deep sleeps (`SLEEP_SECONDS`, 10 s for testing).

### MQTT (QoS 0, no retain, client id `Agrisoil_1`)
Payload is plain text, not JSON.

| Topic | Payload example | Meaning |
|---|---|---|
| `agrisoil/val/nit` | `201.59` | Nitrogen, ppm |
| `agrisoil/val/phos` | `13.99` | Phosphorus, ppm |
| `agrisoil/val/kal` | `125.94` | Potassium, ppm |
| `agrisoil/val/ph` | `6.57` | pH |
| `agrisoil/val/moist` | `48.01` | Moisture, % |
| `agrisoil/raw/npk1` | `5666` | Raw ADC, NPK probe 1 |
| `agrisoil/raw/npk2` | `5659` | Raw ADC, NPK probe 2 (0 on ESP32-ADC builds) |
| `agrisoil/raw/moist` | `8454` | Raw ADC, moisture |
| `agrisoil/raw/ph` | `309` | Raw ADC, pH |
| `agrisoil/loc/lat` | `-6.199969` | Latitude, only sent when GPS has a fix |
| `agrisoil/loc/lng` | `106.816642` | Longitude, only sent when GPS has a fix |

The 11 messages of one reading arrive within ~100 ms. To rebuild one reading, group them by arrival time.

### MQTT port lookup (Node-RED must serve this)
- The broker sits behind an ngrok TCP tunnel at `0.tcp.ap.ngrok.io:<port>`, and the port changes on every ngrok restart.
- On boot the device calls `GET https://nodered.agrisoil.my.id/getngrokport` with HTTP basic auth (`httpsUname` / `httpsPassword` in `secret.h`).
- Reply: plain text, just the port number, e.g. `12115`. Anything else → device falls back to `12115`.

### Firebase Realtime Database (device writes directly, not through Node-RED)
- `PUT /Alat3_rt`: latest reading, **numbers**: `waktu, LAT, LONG, N, P, K, PH, MOIST`
- `POST /Alat3`: history, **strings**: `waktu, LAT, LONG, n, p, k, ph, moisture`
- `waktu` = `"2023-11-02 12:23:08"` (WIB, UTC+7) or `"unknown"`.

### MySQL (device POSTs directly to the PHP dashboard)
`POST https://sistempemetaan2023.com/dashboard/saveData`, form-urlencoded:
`kelembaban, ph, nilai_n, nilai_p, nilai_k, latitude, longitude`

## 2. Realistic value ranges for dummy data
| Value | Dummy range | Firmware clamp (ADS1115 / ESP32 ADC) |
|---|---|---|
| N | 60 – 350 ppm | 0–700 / 0–900 |
| P | 5 – 30 ppm | 0–30 / 0–100 |
| K | 70 – 170 ppm | 0–170 / 0–700 |
| pH | 5.0 – 8.0 | 1–14 |
| Moisture | 20 – 90 % | 0–100 |
| raw npk1 | `(46510 - N) / 8.1731` → ~5650–5690 | ADS1115 counts |
| raw moist | `11940 - moist% × 72.6` (4680 = wet, 11940 = dry) | ADS1115 counts |

Use a random walk (small step from the previous value), not pure random, so charts look like real soil.
Send ~10% of readings without lat/lng to mimic "no GPS fix".
Edge cases worth faking too: probe out of soil (all values 0), pH stuck at ~7.5 (known calibration issue).

## 3. The dummy flow (`docs/nodered-dummy-flow.json`)
Import it in Node-RED (Menu → Import). It contains:
- **inject, every 10 s** → **function "fake agrisoil reading"** → **mqtt out** (the exact topics above). Random walk state lives in the function's `context`.
- **http in `GET /getngrokport`** → **function** → **http response**. Answers `global.ngrokPort`, or env `AGRI_MQTT_PORT`, or `1883`.
- **mqtt-broker config** pointing at `localhost:1883`. Change it to the real broker.

Optional env vars: `AGRI_LAT`, `AGRI_LNG` (field location), `AGRI_MQTT_PORT`.

## 4. To do in the new session
1. Deploy Node-RED + an MQTT broker (e.g. Mosquitto) on the infra.
2. Protect HTTP endpoints with basic auth (`httpNodeAuth` in `settings.js`), so `/getngrokport` behaves like production.
3. Import the dummy flow, point the broker node at the real broker, deploy, and watch the topics arrive (`mosquitto_sub -t 'agrisoil/#' -v`).
4. Optional: also write dummy readings to a **dev** Firebase database and a dev MySQL table in the same shapes as section 1, so the new dashboard can be built against any of the three.
5. Remake the dashboard on top of this data.

⚠️ The Firebase database secret in the old `npkgis` repo is public. Rotate it, and use a separate dev database for dummy data.
