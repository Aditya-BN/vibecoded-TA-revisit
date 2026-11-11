//=========================== GPS ===========================
// GPS module on Serial2 (GPIO16 = RX2, GPIO17 = TX2), speaking NMEA at 9600.
// With USE_GPS 0 the functions still exist, they just say "no fix".
#if USE_GPS

#include <TinyGPSPlus.h>

const uint8_t GPS_RX_PIN = 16;
const uint8_t GPS_TX_PIN = 17;
const uint32_t GPS_BAUD = 9600;

const uint32_t GPS_MAX_AGE_MS = 5000; // older than this = we lost the sky

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

void waitForGpsFix(uint32_t timeoutMs)
{
  Serial.println("=> Nyari satelit...");

  uint32_t startedAt = millis();
  while (!currentGpsFix().valid && millis() - startedAt < timeoutMs)
  {
    feedGps();
    delay(10);
  }

  if (gps.charsProcessed() < 10)
  {
    Serial.println(F("=> GPS diem aja, cek kabelnya"));
  }
}

GpsFix currentGpsFix()
{
  GpsFix fix;
  fix.valid = gps.location.isValid() && gps.location.age() < GPS_MAX_AGE_MS;
  fix.latitude = gps.location.lat();
  fix.longitude = gps.location.lng();
  fix.satellites = gps.satellites.value();
  return fix;
}

#else

void beginGps() {}
void feedGps() {}
void waitForGpsFix(uint32_t timeoutMs) {}

GpsFix currentGpsFix()
{
  GpsFix noFix = {};
  return noFix;
}

#endif
