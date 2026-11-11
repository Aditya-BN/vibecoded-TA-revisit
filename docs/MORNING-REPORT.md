# Morning report — 12 Nov 2026

The stack is running in `agrisoil-dev` on cluster `workload01`: 5 pods, 4 dummy sensors, and the dashboard works.
The PR is **not** opened (no GitHub token on the bastion), so use the link at the bottom.

## What's live (internal network, plain HTTP)
| What | URL | Login (Secret in `agrisoil-dev`) |
|---|---|---|
| Dashboard | http://agrisoil.10.12.135.205.nip.io | none to view; renaming a sensor uses `dashboard-auth` |
| Node-RED editor (public) | **https://new-nodered.adityabn.my.id** (Cloudflare Tunnel) | `nodered-auth` (`admin-user` / `admin-password`) |
| Node-RED editor (internal) | http://nodered-agrisoil.10.12.135.205.nip.io | same |
| Port lookup | https://new-nodered.adityabn.my.id/getngrokport | `nodered-auth` (`http-user` / `http-password`) |
| MQTT | `mosquitto:1883`, inside the cluster only | `mosquitto-auth` (users `nodered`, `device`) |
| Postgres | `postgres:5432`, db `agrisoil` | `postgres-auth` (`agrisoil-password`, `dashboard-password`) |

To read a password: `kubectl -n agrisoil-dev get secret nodered-auth -o jsonpath='{.data.admin-password}' | base64 -d`.
The passwords are random, were made by `deploy/k8s/make-secrets.sh`, and are not in git.

## Definition of done
1. ✅ Pods: dashboard ×2, mosquitto, nodered-0 and postgres-0 are all 1/1 Running.
2. ✅ `mosquitto_sub` (from the mosquitto pod) shows Alat1–3 every 10 s. Alat4 is there too now.
3. ✅ `devices` and `readings` fill up (150+ readings at 01:37 WIB). WIB→UTC conversion checked.
4. ✅ `/getngrokport`: no auth or wrong password → 401; with auth → `1883` as text/plain.
5. ✅ The dashboard showed 3 sensors with charts. It shows 4 now, after test 6.
6. ✅ I added `Alat4` to `AGRI_DEVICES` (ConfigMap only). It was on the dashboard within seconds of the Node-RED restart.
7. ✅ Every pod was restarted once. DB rows were kept. The retained messages read right after the broker restart were stamped before it (01:33:09 vs restart at 01:33:12 WIB).
8. ✅ Screenshots: `docs/screenshots/overview.png`, `docs/screenshots/sensor-Alat2.png`.

Also tested: bad JSON, a non-object payload and a bad device ID are logged and dropped, and Node-RED keeps running.

## Add a real sensor
1. In `agrisoil/agrisoil.ino`, set a new `DEVICE_ID` (letters, digits and `_` only).
2. **New:** add `mqttUser` / `mqttPassword` to your `secret.h` (see `secret.example.h`). Use the `device` login from `mosquitto-auth`, because the broker refuses anonymous clients.
3. In `send_mqtt.ino`, set `MQTT_PORT_URL` to `https://new-nodered.adityabn.my.id/getngrokport` and point `MQTT_HOST` at the new broker. **Blocked:** the broker isn't reachable from outside the cluster yet (see below).
4. Power it on. It appears on the dashboard by itself.

## Left / blocked: you decide
- **Exposing MQTT to real ESP32s.** The cluster has no free TCP exposure, so I created nothing. Options:
  (a) a NodePort Service (free, port 30000+; set `AGRI_MQTT_PORT` to match);
  (b) a MetalLB LoadBalancer (on-prem, so no money, but it takes an IP from the shared pool);
  (c) a TCP entry in ingress-nginx (changes a shared controller);
  (d) keep ngrok in front of `mosquitto`.
  I'd pick (a).
- **Port lookup over HTTPS works now:** set `MQTT_PORT_URL` to `https://new-nodered.adityabn.my.id/getngrokport`. The tunnel only carries HTTP, so the broker itself still needs one of the options above.
- **Security:** the cloudflared Deployment in `workload01-n2lws-pj5hl` has the tunnel token as a plain container argument, so anyone who can read Deployments there can see it. Consider moving it to a Secret (`TUNNEL_TOKEN` env) and rotating it. I didn't change it.
- **PR:** open it yourself: https://github.com/Aditya-BN/vibecoded-TA-revisit/compare/main...claude/npk-vibecoded-rewrite-20ognj
- **Firmware change not compiled here.** CI (`compile.yml`) will check it on push. Please look at the run.
- Readings that arrive while Postgres is down are dropped (QoS 0, no buffer).
- Edits made in the Node-RED editor are lost on restart, because flows come from git (`nodered/flows/`). Export changes to the repo.
- The dummy sensors keep running and write about 35k rows a day. Set `AGRI_DEVICES` to empty, or scale Node-RED down, when you don't need them.
- To remove everything: `kubectl delete ns agrisoil-dev`. The PVCs are deleted too (reclaim policy Delete).

Details and the reasons behind each choice: `docs/DECISIONS.md`.
