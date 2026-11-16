// Node-RED settings for the agrisoil-dev namespace.
// Every login comes from env vars that Kubernetes fills from the nodered-auth Secret.
const crypto = require('crypto');

function same(a, b) {
    const x = Buffer.from(String(a || ''));
    const y = Buffer.from(String(b || ''));
    return x.length === y.length && x.length > 0 && crypto.timingSafeEqual(x, y);
}

const ADMIN_USER = process.env.NR_ADMIN_USER;
const ADMIN_PASS = process.env.NR_ADMIN_PASSWORD;
const HTTP_USER = process.env.NR_HTTP_USER;
const HTTP_PASS = process.env.NR_HTTP_PASSWORD;
const VIEW_USER = process.env.NR_VIEWER_USER;
const VIEW_PASS = process.env.NR_VIEWER_PASSWORD;

// true when the Authorization header carries one of the given user/password pairs
function basicOk(header, ...pairs) {
    const [scheme, token] = String(header || '').split(' ');
    if (scheme !== 'Basic' || !token) return false;
    const decoded = Buffer.from(token, 'base64').toString();
    const i = decoded.indexOf(':');
    if (i <= 0) return false;
    const u = decoded.slice(0, i), p = decoded.slice(i + 1);
    return pairs.some(([user, pass]) => same(u, user) && same(p, pass));
}
// the dashboard accepts the viewer login, and the admin login too
const DASHBOARD_LOGINS = [[VIEW_USER, VIEW_PASS], [ADMIN_USER, ADMIN_PASS]];

module.exports = {
    uiPort: 1880,
    userDir: '/data',
    // Flows are rebuilt from git (nodered/flows/*.json) on every start and kept in memory,
    // so the repo stays the source of truth. The image's FLOWS env var points here too.
    flowFile: '/run/nodered/flows.json',
    // flows_cred.json (in /data) is written encrypted by init.sh with this key.
    credentialSecret: process.env.NR_CREDENTIAL_SECRET,

    // adminAuth: editor + admin API
    adminAuth: {
        type: 'credentials',
        users: async (username) => (username === ADMIN_USER ? { username, permissions: '*' } : null),
        authenticate: async (username, password) =>
            same(username, ADMIN_USER) && same(password, ADMIN_PASS) ? { username, permissions: '*' } : null,
    },

    // httpNodeAuth equivalent: basic auth on every http-in route (e.g. /getngrokport for the ESP32).
    // Node-RED applies this to http-in nodes only, not to Dashboard 2.0 (see `dashboard` below).
    // Plain passwords from the Secret, so no bcrypt hash has to be pre-computed.
    httpNodeMiddleware: (req, res, next) => {
        if (basicOk(req.headers.authorization, [HTTP_USER, HTTP_PASS])) return next();
        res.set('WWW-Authenticate', 'Basic realm="agrisoil"');
        res.status(401).send('Unauthorized');
    },

    // Dashboard 2.0 at /dashboard: viewer (or admin) login on the pages and on the socket.io connection.
    dashboard: {
        middleware: (req, res, next) => {
            if (basicOk(req.headers.authorization, ...DASHBOARD_LOGINS)) return next();
            res.set('WWW-Authenticate', 'Basic realm="agrisoil dashboard"');
            res.status(401).send('Unauthorized');
        },
        ioMiddleware: (socket, next) =>
            basicOk(socket.request.headers.authorization, ...DASHBOARD_LOGINS) ? next() : next(new Error('Unauthorized')),
    },

    // Leaflet for the dashboard map, served by Node-RED itself (no CDN).
    httpStatic: [{ path: '/data/node_modules/leaflet/dist/', root: '/vendor/leaflet/' }],

    diagnostics: { enabled: true, ui: true },
    runtimeState: { enabled: false, ui: false },
    logging: { console: { level: 'info', metrics: false, audit: false } },
    exportGlobalContextKeys: false,
    externalModules: { autoInstall: false, palette: { allowInstall: false } },
    editorTheme: { projects: { enabled: false }, palette: {} },
    functionExternalModules: false,
    functionTimeout: 0,
    debugMaxLength: 1000,
    mqttReconnectTime: 5000,
};
