// Overview: one card per sensor + a map. Sensors come from the database,
// so a new DEVICE_ID shows up here on its own after its first reading.
'use strict';

const map = makeMap(document.getElementById('map'));
const markers = {};
let fitted = false;

function card(d) {
  const values = FIELDS.map((f) => `
    <div class="value" title="${esc(f.label)}"><div class="lbl">${esc(f.short)}</div><div class="num">${withUnit(f, d[f.key])}</div></div>`).join('');
  const soil = d.in_soil === false ? ' <span class="pill warn">tidak di tanah</span>' : '';
  return `
    <a class="card ${d.online ? '' : 'offline'}" href="/devices/${encodeURIComponent(d.id)}">
      <div class="card-head">
        <span class="name">${esc(d.name)}</span>
        ${d.name !== d.id ? `<span class="id">${esc(d.id)}</span>` : ''}
        ${status(d)}${soil}
      </div>
      <div class="values">${values}</div>
      <div class="card-foot">
        <span title="${esc(wib(d.last_seen))}">Terakhir: ${ago(d.last_seen)}</span>
        <span>${esc(wib(d.ts))}</span>
      </div>
    </a>`;
}

function updateMap(devices) {
  const pts = [];
  for (const d of devices) {
    if (d.last_lat === null || d.last_lng === null) continue;
    const ll = [d.last_lat, d.last_lng];
    pts.push(ll);
    const html = `<b><a href="/devices/${encodeURIComponent(d.id)}">${esc(d.name)}</a></b><br>` +
      `${d.online ? 'online' : 'offline'} · ${ago(d.last_seen)}<br>` +
      `N ${fmt(d.n, 1)} · P ${fmt(d.p, 1)} · K ${fmt(d.k, 1)} ppm<br>pH ${fmt(d.ph, 2)} · ${fmt(d.moist, 1)} %`;
    if (!markers[d.id]) {
      markers[d.id] = L.marker(ll, { title: d.name }).addTo(map)
        .bindTooltip(d.name, { permanent: true, direction: 'top', offset: [-15, -12] });
      markers[d.id].bindPopup(html);
      fitted = false; // a new sensor appeared, re-fit once
    } else {
      markers[d.id].setLatLng(ll).setPopupContent(html);
    }
  }
  if (!fitted && pts.length) {
    map.fitBounds(pts, { padding: [40, 40], maxZoom: 16 });
    fitted = true;
  }
}

async function refresh() {
  const updated = document.getElementById('updated');
  try {
    const { devices } = await getJSON('/api/devices');
    const online = devices.filter((d) => d.online).length;
    document.getElementById('summary').textContent = `${devices.length} sensor · ${online} online`;
    document.getElementById('cards').innerHTML = devices.length
      ? devices.map(card).join('')
      : '<div class="empty">Belum ada sensor. Sensor baru muncul otomatis setelah kirim data pertama ke <code>agrisoil/&lt;DEVICE_ID&gt;/reading</code>.</div>';
    updateMap(devices);
    setStatusLine(updated, true, 'diperbarui ' + new Date().toLocaleTimeString('id-ID'));
  } catch (e) {
    setStatusLine(updated, false, 'gagal memuat: ' + e.message);
  }
}

refresh();
setInterval(refresh, POLL_MS);
