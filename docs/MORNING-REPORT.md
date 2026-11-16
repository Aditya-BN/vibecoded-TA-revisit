# Morning report — 16 Nov 2026 (Node-RED Dashboard)

The dashboard is now **Node-RED Dashboard 2.0**, inside Node-RED. The old Node.js dashboard is retired.
Mosquitto, Postgres, the ingest flow, the dummy sensors (now Alat1–5) and `/getngrokport` are unchanged.
The PR is still **not** opened (no GitHub token on the bastion). Link at the bottom.

## What's live
| What | URL | Login (Secret `nodered-auth` in `agrisoil-dev`) |
|---|---|---|
| **Dashboard** | **https://new-nodered.adityabn.my.id/dashboard** | `viewer-user` / `viewer-password` (the admin login works too) |
| Node-RED editor | https://new-nodered.adityabn.my.id | `admin-user` / `admin-password` |
| Port lookup | https://new-nodered.adityabn.my.id/getngrokport | `http-user` / `http-password` (device login) |
| Internal (office LAN) | http://nodered-agrisoil.10.12.135.205.nip.io (+ `/dashboard`) | same as above |
| MQTT | `mosquitto:1883`, inside the cluster only | `mosquitto-auth` (users `nodered`, `device`) |
| Postgres | `postgres:5432`, db `agrisoil` | `postgres-auth` |

Read a password: `kubectl -n agrisoil-dev get secret nodered-auth -o jsonpath='{.data.viewer-password}' | base64 -d`

## Checklist (handoff section 6)
- ✅ Dashboard 2.0 (`@flowfuse/node-red-dashboard@1.36.1`, pinned, installed by the init container). Flow in `nodered/flows/dashboard.json`.
- ✅ **Overview:** a card per sensor from `devices` (N/P/K/pH/moisture with units, last seen, online/offline after `OFFLINE_AFTER_SECONDS`), plus a Leaflet map of every sensor.
- ✅ **Sensor page:** the device dropdown fills from `devices` by itself; 1 jam / 24 jam / 7 hari range; a chart per value (bucketed in SQL); raw ADC in a collapsible panel.
- ✅ **Scalability:** with a browser already open, I published Alat5's first reading as the `device` user, the way an ESP32 would. Its card and dropdown entry appeared within 10 s, with no flow edit and no redeploy. The open viewer's selection didn't move. Alat5 is now a dummy sensor too.
- ✅ **Client constraints:** two browsers; B switching to 7 hari and A switching to Alat2 didn't affect each other, including across the 10 s refresh.
- ✅ **Login:** `/dashboard` pages, assets and socket.io all refuse anonymous users (401 / "Unauthorized"), and the device login is refused there too. `/getngrokport` still uses only the device login.
- ✅ **Old dashboard retired:** `dashboard/` code, Deployment, Service, Ingress, its ConfigMaps and the `dashboard-auth` Secret, deleted only inside `agrisoil-dev` and only after the checks above. The old nip.io URL now gives 404.
- ✅ Screenshots: `docs/screenshots/nodered-overview.png`, `docs/screenshots/nodered-sensor-Alat2.png`.
- ✅ All pods Running: mosquitto, postgres-0, nodered-0.

## Still open: you decide
- **MQTT for real ESP32s** is still inside the cluster only. Options (NodePort recommended) are in `docs/DECISIONS.md`. Port lookup over HTTPS works already.
- **Firmware:** add `mqttUser` / `mqttPassword` to `secret.h`. CI compiles it; check the run.
- **Security:** the cloudflared tunnel token is a plain container argument in `workload01-n2lws-pj5hl`. Move it to a Secret and rotate it.
- **Leftovers you may drop:** the Postgres role `dashboard` and the `dashboard-password` key (unused now).
- **Dashboard edits made in the Node-RED editor are lost on restart.** Change `nodered/flows/dashboard.json` in git.
- **Dummy sensors** write about 43k rows/day with 5 sensors. Trim `AGRI_DEVICES` when you don't need them.
- **PR:** https://github.com/Aditya-BN/vibecoded-TA-revisit/compare/main...claude/npk-vibecoded-rewrite-20ognj

The 12 Nov stack report is in git history (this file, commit `8dfe904`). Reasons for every choice: `docs/DECISIONS.md`.
