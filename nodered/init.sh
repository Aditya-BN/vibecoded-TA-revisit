#!/bin/sh
# Runs in the Node-RED init container before every start.
# 1. Install extra nodes into the PVC (only when the version changes).
# 2. Merge every flows/*.json into one flow file in memory.
# 3. Write the MQTT broker logins into /data/flows_cred.json, encrypted the way Node-RED
#    does it (Node-RED always reads the cred file from userDir, which is the PVC).
set -eu

cp /config/settings.js /data/settings.js

if [ ! -f /data/package.json ]; then
  echo '{"name":"node-red-project","private":true,"dependencies":{}}' > /data/package.json
fi
cd /data
for pkg in $EXTRA_NODES; do
  name=${pkg%@*}
  want=${pkg##*@}
  have=$(node -p "try{require('/data/node_modules/$name/package.json').version}catch(e){''}")
  if [ "$have" != "$want" ]; then
    echo "installing $pkg (have '$have')"
    npm install --no-audit --no-fund --omit=dev --save "$pkg"
  fi
done

node -e '
const fs = require("fs");
const dir = "/flows";
const files = fs.readdirSync(dir).filter(f => f.endsWith(".json")).sort();
const all = [];
for (const f of files) {
  const nodes = JSON.parse(fs.readFileSync(dir + "/" + f, "utf8"));
  all.push(...nodes);
  console.log("flow", f, nodes.length, "nodes");
}
const ids = new Set();
for (const n of all) { if (ids.has(n.id)) throw new Error("duplicate node id " + n.id); ids.add(n.id); }
fs.writeFileSync("/run/nodered/flows.json", JSON.stringify(all, null, 2));
// every mqtt-broker node logs in as the "nodered" user
const cred = {};
for (const n of all) if (n.type === "mqtt-broker") cred[n.id] = { user: process.env.MQTT_USER, password: process.env.MQTT_PASSWORD };
const crypto = require("crypto");
const key = crypto.createHash("sha256").update(process.env.NR_CREDENTIAL_SECRET).digest();
const iv = crypto.randomBytes(16);
const c = crypto.createCipheriv("aes-256-ctr", key, iv);
const enc = c.update(JSON.stringify(cred), "utf8", "base64") + c.final("base64");
fs.writeFileSync("/data/flows_cred.json", JSON.stringify({ $: iv.toString("hex") + enc }), { mode: 0o600 });
console.log("credentials for", Object.keys(cred).join(", "));
'
