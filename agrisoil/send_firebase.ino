//=========================== SEND: FIREBASE ===========================
// Two places in the realtime database:
//   /<DEVICE_ID>_rt -> latest reading, overwritten every time (for the live view)
//   /<DEVICE_ID>    -> history, a new child is pushed every time
// With DEVICE_ID "Alat3" that's /Alat3_rt and /Alat3, same as the old dashboard.
// Key names are kept exactly as the dashboard expects them.
#if SEND_TO_FIREBASE

#include <FirebaseESP32.h>

#define FIREBASE_LATEST_PATH "/" DEVICE_ID "_rt"
#define FIREBASE_HISTORY_PATH "/" DEVICE_ID

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void beginFirebase()
{
  config.database_url = firebaseDatabaseUrl;
  config.signer.tokens.legacy_token = firebaseDatabaseSecret;

  Firebase.reconnectNetwork(true);

  // Rx 4096, Tx 1024 bytes. Bigger payloads need a bigger Rx buffer.
  fbdo.setBSSLBufferSize(4096, 1024);

  Firebase.begin(&config, &auth);
}

void sendToFirebase()
{
  Serial.println("=> Kirim ke FIREEEEbase");

  //*********************** latest reading, as numbers
  FirebaseJson latest;
  latest.set("waktu", waktu);
  latest.set("LAT", latitude);
  latest.set("LONG", longitude);
  latest.set("N", nilaiN);
  latest.set("P", nilaiP);
  latest.set("K", nilaiK);
  latest.set("PH", nilaiPH);
  latest.set("MOIST", nilaiMoist);

  if (!Firebase.setJSON(fbdo, FIREBASE_LATEST_PATH, latest))
  {
    Serial.println("   Gagal update latest: " + fbdo.errorReason());
  }

  //*********************** history, as text (that's what the dashboard reads)
  FirebaseJson history;
  history.set("waktu", waktu);
  history.set("LAT", String(latitude, 6));
  history.set("LONG", String(longitude, 6));
  history.set("n", String(nilaiN));
  history.set("p", String(nilaiP));
  history.set("k", String(nilaiK));
  history.set("ph", String(nilaiPH));
  history.set("moisture", String(nilaiMoist));

  if (!Firebase.pushJSON(fbdo, FIREBASE_HISTORY_PATH, history))
  {
    Serial.println("   Gagal push history: " + fbdo.errorReason());
  }
}

#endif
