// Shared bits for both pages: value metadata, formatting, fetch, map.
'use strict';

const FIELDS = [
  { key: 'n', label: 'Nitrogen (N)', unit: 'ppm', digits: 1, color: '#2f7d32' },
  { key: 'p', label: 'Fosfor (P)', unit: 'ppm', digits: 1, color: '#c26a00' },
  { key: 'k', label: 'Kalium (K)', unit: 'ppm', digits: 1, color: '#7b3fa0' },
  { key: 'ph', label: 'pH tanah', unit: 'pH', digits: 2, color: '#c0392b' },
  { key: 'moist', label: 'Kelembapan', unit: '%', digits: 1, color: '#1f6fb2' },
];
const RAW_FIELDS = [
  { key: 'npk1_raw', label: 'NPK 1', color: '#2f7d32' },
  { key: 'npk2_raw', label: 'NPK 2', color: '#8bc34a' },
  { key: 'moist_raw', label: 'Moisture', color: '#1f6fb2' },
  { key: 'ph_raw', label: 'pH', color: '#c0392b' },
];
const POLL_MS = 10000;
const WIB = new Intl.DateTimeFormat('id-ID', {
  timeZone: 'Asia/Jakarta', year: 'numeric', month: 'short', day: '2-digit',
  hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false,
});

let clockSkew = 0; // server time - browser time, so "x ago" is right even if the laptop clock is off

function esc(s) {
  return String(s ?? '').replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
}
function fmt(v, digits) {
  return v === null || v === undefined || Number.isNaN(Number(v)) ? '–' : Number(v).toFixed(digits);
}
function withUnit(f, v) {
  const s = fmt(v, f.digits);
  if (s === '–') return s;
  return f.unit === 'pH' ? `${s} <small>pH</small>` : `${s} <small>${f.unit}</small>`;
}
function wib(t) {
  return t ? WIB.format(new Date(t)) + ' WIB' : '–';
}
function ago(t) {
  if (!t) return 'belum pernah';
  const s = Math.max(0, Math.round((Date.now() + clockSkew - new Date(t)) / 1000));
  if (s < 60) return `${s} dtk lalu`;
  if (s < 3600) return `${Math.floor(s / 60)} mnt lalu`;
  if (s < 86400) return `${Math.floor(s / 3600)} jam lalu`;
  return `${Math.floor(s / 86400)} hari lalu`;
}
function status(d) {
  return d.online
    ? '<span class="pill on">● online</span>'
    : '<span class="pill off">● offline</span>';
}
async function getJSON(url) {
  const r = await fetch(url, { cache: 'no-store' });
  if (!r.ok) throw new Error(`${url}: HTTP ${r.status}`);
  const data = await r.json();
  if (data.now) clockSkew = new Date(data.now) - Date.now();
  return data;
}
function setStatusLine(el, ok, text) {
  el.textContent = text;
  el.className = ok ? 'updated' : 'updated error';
}

function makeMap(el) {
  const map = L.map(el, { scrollWheelZoom: false }).setView([-6.2, 106.8166], 13);
  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    maxZoom: 19, attribution: '&copy; OpenStreetMap contributors',
  }).addTo(map);
  return map;
}
