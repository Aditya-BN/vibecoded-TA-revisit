//=========================== SEND: MYSQL ===========================
// A plain form POST to the PHP dashboard, which saves it into MySQL.
#if SEND_TO_MYSQL

#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char *MYSQL_SAVE_URL = "https://sistempemetaan2023.com/dashboard/saveData";

void sendToMysql(const SoilReading &soil, const GpsFix &where)
{
  Serial.println("=> Kirim ke MySQL");

  String form = "kelembaban=" + String(soil.moisture) +
                "&ph=" + String(soil.ph) +
                "&nilai_n=" + String(soil.nitrogen) +
                "&nilai_p=" + String(soil.phosphorus) +
                "&nilai_k=" + String(soil.potassium) +
                "&latitude=" + String(where.latitude, 6) +
                "&longitude=" + String(where.longitude, 6);

  WiFiClientSecure tls;
  tls.setInsecure();

  HTTPClient http;
  http.begin(tls, MYSQL_SAVE_URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST(form);

  if (httpCode == HTTP_CODE_OK)
  {
    Serial.println("   Server bilang: " + http.getString());
  }
  else if (httpCode > 0)
  {
    Serial.printf("   [HTTP] POST code: %d\n", httpCode);
  }
  else
  {
    Serial.printf("   [HTTP] POST failed: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

#endif
