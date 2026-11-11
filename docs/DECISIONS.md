# Decisions (overnight k8s + dashboard run, 2026-11-12)

Each entry: **what I chose** / *why*.

## Cluster and costs
- **Cluster: kube context `workload01-admin@workload01`** (current context; the other one is the NKP management cluster).
  *Workloads belong on the workload cluster, never on the management cluster.*
- **Default StorageClass `nutanix-volume` (csi.nutanix.com), 1 Gi per PVC, 3 PVCs.**
  *It's on-prem Nutanix Volumes, not a paid cloud disk; reclaimPolicy Delete, so deleting the namespace frees it.*
- **No new Service of type LoadBalancer or NodePort.** Everything is ClusterIP behind the existing ingress-nginx (`10.12.135.205`).
  *The ground rules forbid new LBs; MetalLB IPs are a shared pool too.*
- **Hostnames `agrisoil.10.12.135.205.nip.io` and `nodered-agrisoil.10.12.135.205.nip.io`, plain HTTP.**
  *nip.io needs no DNS change; adding TLS would mean touching cert-manager/issuers, which is off limits.*

## Images and build
- **No custom image registry. Stock public images (`node`, `nodered/node-red`, `eclipse-mosquitto`, `postgres`) + app code from Kustomize-generated ConfigMaps; `npm ci` runs in an init container.**
  *No registry credentials were available and pushing to the shared Harbor would change something outside `agrisoil-dev`. Hash-suffixed ConfigMaps make every code change roll the pods. A `dashboard/Dockerfile` is included for when a registry is chosen.*
- **Pinned image tags** (`node:22.21-alpine`, `nodered/node-red:4.1.1`, `eclipse-mosquitto:2.0.22`, `postgres:17.7-alpine`). *Reproducible restarts.*
- **Node-RED extra node `node-red-contrib-postgresql@0.16.2` installed into the Node-RED PVC by the init container (only when the version changes).** *Keeps restarts fast, no custom image.*

## Repo layout
- **Moved `docs/nodered-dummy-flow.json` → `nodered/flows/dummy-sensors.json` (content unchanged).**
  *Kustomize can't read files outside a kustomization's folder, so flows live next to `nodered/kustomization.yaml`.*
- **`dashboard/` and `nodered/` each have a small `kustomization.yaml` (ConfigMaps only); `deploy/k8s/` holds the workloads and includes both.**

## Node-RED
- **Flows are rebuilt from git on every start (`nodered/flows/*.json` merged into `/run/nodered/flows.json`, in memory).**
  *Git is the source of truth. Edits made in the editor are lost on restart: export them into the repo.*
- **MQTT logins are written by the init container to `/data/flows_cred.json`, encrypted exactly like Node-RED does (AES-256-CTR, key = sha256 of `credential-secret` in `nodered-auth`).**
  *Node-RED always reads the cred file from `userDir` (the PVC), and doesn't expand `${ENV}` in credentials. Encrypted at rest, nothing committed.*
- **`adminAuth` uses a custom `authenticate` and http-in auth uses `httpNodeMiddleware` (basic auth), both comparing against plain passwords from the `nodered-auth` Secret.**
  *Same effect as `adminAuth`/`httpNodeAuth` without pre-computing bcrypt hashes.*
- **`/getngrokport` returns `AGRI_MQTT_PORT` from the `nodered-env` ConfigMap (default 1883).** *Configurable without code change, as the brief asks.*
- **One Node-RED replica, plain subscription `agrisoil/+/reading` (not `$share/...`).**
  *Mosquitto doesn't send retained messages to shared subscriptions; with one replica a plain subscription also picks up each sensor's latest retained reading after a restart. Switch to `$share/ingest/...` before scaling.*
- **Separate MQTT broker config node for ingest (random client id).** *Ingest keeps working if the dummy tab is disabled.*

## Ingest and database
- **`CREATE UNIQUE INDEX readings_device_ts ON readings (device_id, ts DESC)` instead of the plain index from the brief.**
  *Same index, but unique: a retained reading re-delivered after a Node-RED restart is skipped (`ON CONFLICT DO NOTHING`) instead of stored twice.*
- **Device upsert + reading insert in one SQL statement (data-modifying CTE).** *Atomic, one round trip, no manual DB row ever.*
- **`last_seen` = time of the newest reading (capped at now), not the time Node-RED processed it.**
  *Otherwise a dead sensor's retained message would make it look online after every Node-RED restart.*
- **The topic decides the DEVICE_ID; a different `device` field in the JSON only logs a warning.** *The topic is what the broker ACL can control.*
- **`waktu` is used only when it parses, is ≥ 2020 and not more than 5 min in the future; otherwise receive time.** *Handles `"unknown"` and never-synced clocks.*
- **GPS `0,0` or out-of-range coordinates are treated as no fix.**
- **Dashboard uses a separate DB login `dashboard` (SELECT + UPDATE of `devices.name` only).** *Least privilege; Node-RED owns writes.*

## MQTT
- **Two broker users: `nodered` (read/write `agrisoil/#`) and `device` (write-only `agrisoil/#`, shared by the ESP32s).** *Sensors can't read each other or anything else.*
- **Firmware: `client.connect(MQTT_CLIENT_ID, mqttUser, mqttPassword)` with `mqttUser`/`mqttPassword` in `secret.h`.**
  *The new broker refuses anonymous clients and the old firmware connected anonymously. Existing `secret.h` files need these two lines added.*
- **Real ESP32s are NOT exposed to the broker tonight.** No existing TCP exposure is free to reuse (ingress-nginx has no TCP services configured, and the Traefik TCP entrypoints belong to Kommander). Options are in the morning report. *The ground rules say: don't create a paid LB, document instead.*

## Dashboard
- **Plain Node.js `http` server + `pg`, vanilla JS front end, Chart.js + Leaflet served from `node_modules` (no CDN, no build step).**
  *Fewest moving parts; only map tiles come from the internet (OpenStreetMap).*
- **Live updates by polling every 10 s.** *Keeps the API stateless (no MQTT connection per replica); readings arrive every 10 s anyway.*
- **History is averaged into buckets (1 h → 10 s, 24 h → 2 min, 7 d → 15 min).** *A few hundred points per chart however many readings exist.*
- **Offline after `OFFLINE_AFTER_SECONDS` (ConfigMap `dashboard-env`, default 120 s).**
  *Dummies publish every 10 s; raise this when real sensors sleep longer.*
- **Display names can be changed from the sensor page; the API asks for basic auth (`dashboard-auth` Secret). Reading is public on the internal network.**
- **Times are shown in WIB.**

## Secrets
- **`deploy/k8s/make-secrets.sh` generates random passwords straight into Kubernetes Secrets and never overwrites existing ones.** *Nothing secret is in git; reruns are safe.*
