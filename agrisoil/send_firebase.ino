//=========================== SEND: FIREBASE ===========================
// Two places in the realtime database:
//   /Alat3_rt -> latest reading, overwritten every time (for the live view)
//   /Alat3    -> history, a new child is pushed every time
// Key names are kept exactly as the dashboard expects them.
#if SEND_TO_FIREBASE

#include <FirebaseESP32.h>

const char *FIREBASE_LATEST_PATH = "/Alat3_rt";
const char *FIREBASE_HISTORY_PATH = "/Alat3";

FirebaseData fbdo;
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig;

void beginFirebase()
{
  firebaseConfig.database_url = firebaseDatabaseUrl;
  firebaseConfig.signer.tokens.legacy_token = firebaseDatabaseSecret;

  Firebase.reconnectNetwork(true);

  // Rx 4096, Tx 1024 bytes. Bigger payloads need a bigger Rx buffer.
  fbdo.setBSSLBufferSize(4096, 1024);

  Firebase.begin(&firebaseConfig, &firebaseAuth);
}

void sendToFirebase(const SoilReading &soil, const GpsFix &where, const String &when)
{
  Serial.println("=> Kirim ke FIREEEEbase");

  //*********************** latest reading, as numbers
  FirebaseJson latest;
  latest.set("waktu", when);
  latest.set("LAT", where.latitude);
  latest.set("LONG", where.longitude);
  latest.set("N", soil.nitrogen);
  latest.set("P", soil.phosphorus);
  latest.set("K", soil.potassium);
  latest.set("PH", soil.ph);
  latest.set("MOIST", soil.moisture);

  if (!Firebase.setJSON(fbdo, FIREBASE_LATEST_PATH, latest))
  {
    Serial.println("   Gagal update latest: " + fbdo.errorReason());
  }

  //*********************** history, as text (that's what the dashboard reads)
  FirebaseJson history;
  history.set("waktu", when);
  history.set("LAT", String(where.latitude, 6));
  history.set("LONG", String(where.longitude, 6));
  history.set("n", String(soil.nitrogen));
  history.set("p", String(soil.phosphorus));
  history.set("k", String(soil.potassium));
  history.set("ph", String(soil.ph));
  history.set("moisture", String(soil.moisture));

  if (!Firebase.pushJSON(fbdo, FIREBASE_HISTORY_PATH, history))
  {
    Serial.println("   Gagal push history: " + fbdo.errorReason());
  }
}

#endif
