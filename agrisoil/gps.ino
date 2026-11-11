//=========================== GPS ===========================
// GPS module on Serial2 (GPIO16 = RX2, GPIO17 = TX2), speaking NMEA at 9600.
// With USE_GPS 0 the functions still exist, they just say "no fix".
#if USE_GPS

#include <TinyGPSPlus.h>

#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define GPS_BAUD 9600
#define GPS_MAX_AGE_MS 5000 // older than this = we lost the sky

TinyGPSPlus gps;

void beginGps()
{
  // Bigger buffer so NMEA sentences survive while we're busy uploading.
  Serial2.setRxBufferSize(1024);
  Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
}

// Call this as often as you can, TinyGPS only knows what we feed it.
void feedGps()
{
  while (Serial2.available() > 0)
  {
    gps.encode(Serial2.read());
  }
}

void waitForGpsFix()
{
  Serial.println("=> Nyari satelit...");

  unsigned long startedAt = millis();
  while (millis() - startedAt < GPS_FIX_TIMEOUT_MS)
  {
    feedGps();
    readGps();
    if (gpsValid) return;
    delay(10);
  }

  if (gps.charsProcessed() < 10)
  {
    Serial.println(F("=> GPS diem aja, cek kabelnya"));
  }
}

void readGps()
{
  gpsValid = gps.location.isValid() && gps.location.age() < GPS_MAX_AGE_MS;
  latitude = gps.location.lat();
  longitude = gps.location.lng();
  satellites = gps.satellites.value();
}

#else

void beginGps() {}
void feedGps() {}
void waitForGpsFix() {}
void readGps() { gpsValid = false; }

#endif
