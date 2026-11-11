// Sensor page /devices/<id>: latest values, history charts, raw ADC, map.
'use strict';

const id = decodeURIComponent(location.pathname.split('/')[2] || '');
let range = new URLSearchParams(location.search).get('range') || '24h';
if (!['1h', '24h', '7d'].includes(range)) range = '24h';

const charts = {};
let rawChart = null;
let map = null;
let marker = null;

Chart.defaults.font.family = getComputedStyle(document.body).fontFamily;
Chart.defaults.color = '#6b766d';

const TIME_FMT = {
  '1h': { hour: '2-digit', minute: '2-digit' },
  '24h': { hour: '2-digit', minute: '2-digit' },
  '7d': { day: '2-digit', month: 'short', hour: '2-digit' },
};
function tickLabel(ms) {
  return new Intl.DateTimeFormat('id-ID', { timeZone: 'Asia/Jakarta', hour12: false, ...TIME_FMT[range] }).format(new Date(ms));
}
function lineChart(canvas, datasets, unit) {
  return new Chart(canvas, {
    type: 'line',
    data: { datasets },
    options: {
      animation: false, maintainAspectRatio: false, parsing: false, normalized: true,
      interaction: { mode: 'nearest', axis: 'x', intersect: false },
      elements: { point: { radius: 0, hitRadius: 6 }, line: { borderWidth: 2, tension: 0.25 } },
      plugins: {
        legend: { display: datasets.length > 1, labels: { boxWidth: 12 } },
        tooltip: { callbacks: {
          title: (items) => items.length ? wib(items[0].parsed.x) : '',
          label: (c) => `${c.dataset.label}: ${c.parsed.y.toFixed(2)} ${unit}`,
        } },
      },
      scales: {
        x: { type: 'linear', ticks: { maxTicksLimit: 6, callback: (v) => tickLabel(v) }, grid: { display: false } },
        y: { title: { display: true, text: unit }, grace: '5%' },
      },
    },
  });
}

function buildCharts() {
  const box = document.getElementById('charts');
  box.innerHTML = FIELDS.map((f) => `
    <div class="chart-card"><h3>${esc(f.label)} <small>(${f.unit})</small></h3>
      <div class="chart-box"><canvas id="c-${f.key}"></canvas></div></div>`).join('');
  for (const f of FIELDS) {
    charts[f.key] = lineChart(document.getElementById('c-' + f.key),
      [{ label: f.label, borderColor: f.color, backgroundColor: f.color + '22', fill: true, data: [] }], f.unit);
  }
  rawChart = lineChart(document.getElementById('raw-chart'),
    RAW_FIELDS.map((f) => ({ label: f.label, borderColor: f.color, data: [] })), 'ADC');
}

function renderLatest(d) {
  document.title = `${d.name} · Agrisoil`;
  document.getElementById('title').textContent = d.name;
  document.getElementById('state').innerHTML = status(d) + (d.in_soil === false ? ' <span class="pill warn">tidak di tanah</span>' : '');
  document.getElementById('latest').innerHTML = FIELDS.map((f) => `
    <div class="value"><div class="lbl">${esc(f.label)}</div><div class="num">${withUnit(f, d[f.key])}</div></div>`).join('');
  document.getElementById('meta').innerHTML = [
    `ID: <b>${esc(d.id)}</b>`,
    `Bacaan terakhir: <b>${esc(wib(d.ts))}</b>`,
    `Terakhir terlihat: <b>${ago(d.last_seen)}</b>`,
    `Pertama terlihat: ${esc(wib(d.first_seen))}`,
    `Total bacaan: ${Number(d.readings).toLocaleString('id-ID')}`,
  ].map((s) => `<span>${s}</span>`).join('');
  document.getElementById('raw-latest').innerHTML = RAW_FIELDS.map((f) =>
    `<tr><td>${esc(f.label)}</td><td>${d[f.key] ?? '–'} <small>counts</small></td></tr>`).join('');

  if (d.last_lat !== null && d.last_lng !== null) {
    if (!map) map = makeMap(document.getElementById('map'));
    const ll = [d.last_lat, d.last_lng];
    if (!marker) { marker = L.marker(ll).addTo(map); map.setView(ll, 16); } else marker.setLatLng(ll);
    marker.bindPopup(`${esc(d.name)}<br>${ll[0].toFixed(6)}, ${ll[1].toFixed(6)}`);
    document.getElementById('map-note').textContent = `${ll[0].toFixed(6)}, ${ll[1].toFixed(6)} (lokasi GPS terakhir yang valid)`;
  } else {
    document.getElementById('map').hidden = true;
    document.getElementById('map-note').textContent = 'Belum ada lokasi GPS dari sensor ini.';
  }
}

async function loadHistory() {
  const h = await getJSON(`/api/devices/${encodeURIComponent(id)}/readings?range=${range}`);
  const t = h.points.map((p) => new Date(p.t).getTime());
  for (const f of FIELDS) {
    charts[f.key].data.datasets[0].data = h.points.map((p, i) => ({ x: t[i], y: p[f.key] })).filter((p) => p.y !== null);
    charts[f.key].update();
  }
  RAW_FIELDS.forEach((f, j) => {
    rawChart.data.datasets[j].data = h.points.map((p, i) => ({ x: t[i], y: p[f.key] })).filter((p) => p.y !== null);
  });
  rawChart.update();
  document.getElementById('bucket').textContent = `${h.points.length} titik, rata-rata per ${h.bucket.replace('seconds', 'detik').replace('minutes', 'menit')}`;
}

async function refresh() {
  const updated = document.getElementById('updated');
  try {
    const r = await fetch(`/api/devices/${encodeURIComponent(id)}`, { cache: 'no-store' });
    if (r.status === 404 || r.status === 400) {
      document.getElementById('content').hidden = true;
      document.getElementById('missing').hidden = false;
      document.getElementById('title').textContent = id;
      return;
    }
    const data = await r.json();
    clockSkew = new Date(data.now) - Date.now();
    document.getElementById('content').hidden = false;
    document.getElementById('missing').hidden = true;
    renderLatest(data.device);
    await loadHistory();
    setStatusLine(updated, true, 'diperbarui ' + new Date().toLocaleTimeString('id-ID'));
  } catch (e) {
    setStatusLine(updated, false, 'gagal memuat: ' + e.message);
  }
}

function selectRange(r) {
  range = r;
  for (const b of document.querySelectorAll('#ranges button')) b.classList.toggle('active', b.dataset.range === r);
  const u = new URL(location.href); u.searchParams.set('range', r); history.replaceState(null, '', u);
}
document.getElementById('ranges').addEventListener('click', (e) => {
  if (!e.target.dataset.range) return;
  selectRange(e.target.dataset.range);
  loadHistory().catch((err) => setStatusLine(document.getElementById('updated'), false, err.message));
});

document.getElementById('rename').addEventListener('click', async () => {
  const name = prompt('Nama tampilan baru (kosongkan untuk pakai ID):', document.getElementById('title').textContent);
  if (name === null) return;
  const r = await fetch(`/api/devices/${encodeURIComponent(id)}/name`, {
    method: 'PUT', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ name }),
  });
  if (r.ok) refresh(); else alert('Gagal ganti nama: HTTP ' + r.status);
});

buildCharts();
selectRange(range);
refresh();
setInterval(refresh, POLL_MS);
