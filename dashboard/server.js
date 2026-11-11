// Agrisoil dashboard: static pages + a small JSON API over PostgreSQL.
// Stateless: run as many replicas as you like, Postgres is the only state.
'use strict';
const http = require('http');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { Pool } = require('pg');

const PORT = Number(process.env.PORT || 8080);
const OFFLINE_AFTER_SECONDS = Number(process.env.OFFLINE_AFTER_SECONDS || 120);
const ADMIN_USER = process.env.ADMIN_USER || '';
const ADMIN_PASSWORD = process.env.ADMIN_PASSWORD || '';

// PGHOST, PGPORT, PGDATABASE, PGUSER, PGPASSWORD come from the environment.
const pool = new Pool({ max: 5, idleTimeoutMillis: 30000, connectionTimeoutMillis: 5000, application_name: 'dashboard' });
pool.on('error', (err) => console.error('pg pool:', err.message));

// history range -> [window, bucket]; buckets keep every chart at a few hundred points
const RANGES = {
  '1h': ['1 hour', '10 seconds'],
  '24h': ['24 hours', '2 minutes'],
  '7d': ['7 days', '15 minutes'],
};
const ID_RE = /^[A-Za-z0-9_]{1,64}$/;

// ---------- static files, loaded once ----------
const PUBLIC = path.join(__dirname, 'public');
const MODULES = path.join(__dirname, 'node_modules');
const VENDOR = {
  '/vendor/chart.umd.min.js': path.join(MODULES, 'chart.js/dist/chart.umd.min.js'),
  '/vendor/leaflet/leaflet.js': require.resolve('leaflet/dist/leaflet.js'),
  '/vendor/leaflet/leaflet.css': require.resolve('leaflet/dist/leaflet.css'),
};
const leafletImages = path.join(path.dirname(require.resolve('leaflet/dist/leaflet.js')), 'images');
for (const f of fs.readdirSync(leafletImages)) VENDOR['/vendor/leaflet/images/' + f] = path.join(leafletImages, f);

const TYPES = { '.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8', '.css': 'text/css; charset=utf-8',
  '.png': 'image/png', '.svg': 'image/svg+xml', '.ico': 'image/x-icon' };
const files = {};
function load(urlPath, file) {
  files[urlPath] = { body: fs.readFileSync(file), type: TYPES[path.extname(file)] || 'application/octet-stream',
    etag: '"' + crypto.createHash('sha1').update(fs.readFileSync(file)).digest('hex').slice(0, 16) + '"' };
}
for (const f of fs.readdirSync(PUBLIC)) load('/' + f, path.join(PUBLIC, f));
for (const [u, f] of Object.entries(VENDOR)) load(u, f);

// ---------- helpers ----------
function send(res, status, body, headers = {}) {
  res.writeHead(status, { 'Cache-Control': 'no-store', ...headers });
  res.end(body);
}
function json(res, status, obj) {
  send(res, status, JSON.stringify(obj), { 'Content-Type': 'application/json; charset=utf-8' });
}
function same(a, b) {
  const x = Buffer.from(String(a)), y = Buffer.from(String(b));
  return x.length === y.length && x.length > 0 && crypto.timingSafeEqual(x, y);
}
function isAdmin(req) {
  const [scheme, token] = (req.headers.authorization || '').split(' ');
  if (scheme !== 'Basic' || !token || !ADMIN_USER) return false;
  const s = Buffer.from(token, 'base64').toString();
  const i = s.indexOf(':');
  return i > 0 && same(s.slice(0, i), ADMIN_USER) && same(s.slice(i + 1), ADMIN_PASSWORD);
}
function readBody(req, limit = 4096) {
  return new Promise((resolve, reject) => {
    let size = 0; const chunks = [];
    req.on('data', (c) => { size += c.length; if (size > limit) { reject(new Error('too large')); req.destroy(); } else chunks.push(c); });
    req.on('end', () => resolve(Buffer.concat(chunks).toString()));
    req.on('error', reject);
  });
}

// ---------- queries ----------
const LATEST = `
  SELECT d.id, COALESCE(NULLIF(d.name, ''), d.id) AS name, d.first_seen, d.last_seen, d.last_lat, d.last_lng,
         r.ts, r.n, r.p, r.k, r.ph, r.moist, r.npk1_raw, r.npk2_raw, r.moist_raw, r.ph_raw, r.in_soil,
         (now() - d.last_seen) < make_interval(secs => $1) AS online
  FROM devices d
  LEFT JOIN LATERAL (SELECT * FROM readings WHERE device_id = d.id ORDER BY ts DESC LIMIT 1) r ON true`;

async function listDevices() {
  const { rows } = await pool.query(LATEST + ' ORDER BY d.id', [OFFLINE_AFTER_SECONDS]);
  return rows;
}
async function getDevice(id) {
  const { rows } = await pool.query(LATEST + ' WHERE d.id = $2', [OFFLINE_AFTER_SECONDS, id]);
  if (!rows[0]) return null;
  const c = await pool.query('SELECT count(*)::int AS readings FROM readings WHERE device_id = $1', [id]);
  return { ...rows[0], readings: c.rows[0].readings };
}
async function history(id, range) {
  const [win, bucket] = RANGES[range];
  const { rows } = await pool.query(`
    SELECT date_bin($2::interval, ts, timestamptz '2000-01-01') AS t,
           avg(n)::real AS n, avg(p)::real AS p, avg(k)::real AS k, avg(ph)::real AS ph, avg(moist)::real AS moist,
           avg(npk1_raw)::real AS npk1_raw, avg(npk2_raw)::real AS npk2_raw,
           avg(moist_raw)::real AS moist_raw, avg(ph_raw)::real AS ph_raw
    FROM readings
    WHERE device_id = $1 AND ts >= now() - $3::interval
    GROUP BY 1 ORDER BY 1`, [id, bucket, win]);
  return { range, bucket, points: rows };
}

// ---------- routes ----------
async function handle(req, res) {
  const url = new URL(req.url, 'http://x');
  const p = url.pathname;

  if (p === '/livez') return send(res, 200, 'ok');
  if (p === '/healthz') {
    try { await pool.query('SELECT 1'); return send(res, 200, 'ok'); }
    catch (e) { return send(res, 503, 'db: ' + e.message); }
  }

  if (p === '/api/devices' && req.method === 'GET') {
    return json(res, 200, { now: new Date(), offlineAfterSeconds: OFFLINE_AFTER_SECONDS, devices: await listDevices() });
  }

  let m = /^\/api\/devices\/([^/]+)(\/readings|\/name)?$/.exec(p);
  if (m) {
    const id = decodeURIComponent(m[1]);
    if (!ID_RE.test(id)) return json(res, 400, { error: 'bad device id' });
    if (!m[2] && req.method === 'GET') {
      const d = await getDevice(id);
      return d ? json(res, 200, { now: new Date(), offlineAfterSeconds: OFFLINE_AFTER_SECONDS, device: d })
               : json(res, 404, { error: 'device not found' });
    }
    if (m[2] === '/readings' && req.method === 'GET') {
      const range = url.searchParams.get('range') || '24h';
      if (!RANGES[range]) return json(res, 400, { error: 'range must be 1h, 24h or 7d' });
      return json(res, 200, await history(id, range));
    }
    if (m[2] === '/name' && req.method === 'PUT') {
      if (!isAdmin(req)) return send(res, 401, 'login required', { 'WWW-Authenticate': 'Basic realm="agrisoil dashboard"' });
      let name;
      try { name = String(JSON.parse(await readBody(req)).name || '').trim(); } catch { return json(res, 400, { error: 'bad JSON' }); }
      if (name.length > 64) return json(res, 400, { error: 'name too long' });
      const r = await pool.query("UPDATE devices SET name = NULLIF($2, '') WHERE id = $1", [id, name]);
      return r.rowCount ? json(res, 200, { id, name: name || id }) : json(res, 404, { error: 'device not found' });
    }
    return json(res, 405, { error: 'method not allowed' });
  }
  if (p.startsWith('/api/')) return json(res, 404, { error: 'not found' });

  if (req.method !== 'GET' && req.method !== 'HEAD') return send(res, 405, 'method not allowed');
  const f = p === '/' ? files['/index.html'] : /^\/devices\/[^/]+\/?$/.test(p) ? files['/device.html'] : files[p];
  if (!f) return send(res, 404, 'not found', { 'Content-Type': 'text/plain' });
  if (req.headers['if-none-match'] === f.etag) return send(res, 304, '');
  res.writeHead(200, { 'Content-Type': f.type, ETag: f.etag, 'Cache-Control': 'no-cache',
    'X-Content-Type-Options': 'nosniff' });
  res.end(req.method === 'HEAD' ? undefined : f.body);
}

const server = http.createServer((req, res) => {
  handle(req, res).catch((err) => {
    console.error(req.method, req.url, err.message);
    if (!res.headersSent) json(res, 500, { error: 'internal error' });
  });
});
server.listen(PORT, () => console.log(`dashboard on :${PORT}, offline after ${OFFLINE_AFTER_SECONDS}s`));

function shutdown() {
  server.close(() => pool.end().then(() => process.exit(0)));
  setTimeout(() => process.exit(0), 5000).unref();
}
process.on('SIGTERM', shutdown);
process.on('SIGINT', shutdown);
