//=========================== SEND: MYSQL ===========================
// A plain form POST to the PHP dashboard, which saves it into MySQL.
#if SEND_TO_MYSQL

#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#define MYSQL_URL "https://sistempemetaan2023.com/dashboard/saveData"

void sendToMysql()
{
  Serial.println("=> Kirim ke MySQL");

  String postData = "kelembaban=" + String(nilaiMoist) +
                    "&ph=" + String(nilaiPH) +
                    "&nilai_n=" + String(nilaiN) +
                    "&nilai_p=" + String(nilaiP) +
                    "&nilai_k=" + String(nilaiK) +
                    "&latitude=" + String(latitude, 6) +
                    "&longitude=" + String(longitude, 6);

  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  HTTPClient http;
  http.begin(secureClient, MYSQL_URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST(postData);

  if (httpCode == HTTP_CODE_OK)
  {
    Serial.println("   Server bilang: " + http.getString());
  }
  else if (httpCode > 0)
  {
    Serial.print("   [HTTP] POST code: ");
    Serial.println(httpCode);
  }
  else
  {
    Serial.print("   [HTTP] POST failed: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}

#endif
