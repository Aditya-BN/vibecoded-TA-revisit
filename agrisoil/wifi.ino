//=========================== WIFI ===========================
// Up to three access points (see secret.h), the ESP picks the strongest one.

#include <WiFi.h>
#include <WiFiMulti.h>

const uint32_t WIFI_TIMEOUT_MS = 10000;

WiFiMulti wifiMulti;

void beginWifi()
{
  wifiMulti.addAP(ssid_AP_1, password_AP_1);
  wifiMulti.addAP(ssid_AP_2, password_AP_2);
  wifiMulti.addAP(ssid_AP_3, password_AP_3);

  Serial.println("=> Connecting Wifi...");
  if (!ensureWifi())
  {
    Serial.println("=> WiFi gagal, lanjut offline");
    showMessage("WiFi gagal :(", "lanjut offline");
    return;
  }

  Serial.println("=> WiFi connected to " + WiFi.SSID());
  Serial.print("   IP address: ");
  Serial.println(WiFi.localIP());
  showMessage("WiFi connected!", WiFi.localIP().toString().c_str());
}

// Already online? Returns right away. Otherwise tries to (re)connect.
bool ensureWifi()
{
  return wifiMulti.run(WIFI_TIMEOUT_MS) == WL_CONNECTED;
}
