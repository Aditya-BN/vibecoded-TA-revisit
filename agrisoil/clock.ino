//=========================== CLOCK ===========================
// Real time from the internet, so every reading gets a timestamp.
// Uses the ESP32's built-in SNTP, no extra library needed.

#define GMT_OFFSET_SECONDS (7 * 3600) // WIB, UTC+7
#define DAYLIGHT_OFFSET_SECONDS 0     // Indonesia doesn't do DST

void syncClock()
{
  configTime(GMT_OFFSET_SECONDS, DAYLIGHT_OFFSET_SECONDS, "pool.ntp.org", "time.google.com");
}

// Puts "2023-11-02 12:23:08" into waktu, or "unknown" when we're offline.
void readClock()
{
  struct tm now;
  if (!getLocalTime(&now, 5000))
  {
    Serial.println("=> Gagal ambil waktu dari NTP");
    waktu = "unknown";
    return;
  }

  char text[20];
  strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", &now);
  waktu = String(text);
}
