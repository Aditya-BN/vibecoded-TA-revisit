//=========================== SEND: MQTT ===========================
// The broker sits behind an ngrok TCP tunnel, and ngrok hands out
// a new port every restart. Node-RED knows today's port, so we ask it first.
#if SEND_TO_MQTT

#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

#define MQTT_HOST "0.tcp.ap.ngrok.io" // host only, no "tcp://"
#define MQTT_FALLBACK_PORT 12115      // used when Node-RED doesn't answer
#define MQTT_PORT_URL "https://nodered.agrisoil.my.id/getngrokport"
#define MQTT_CLIENT_ID "Agrisoil_1"
#define MQTT_CONNECT_ATTEMPTS 3

WiFiClient espClient;
PubSubClient client(espClient);

void beginMqtt()
{
  int port = fetchMqttPort();
  client.setServer(MQTT_HOST, port);

  Serial.print("=> MQTT broker: ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.println(port);
}

void sendToMqtt()
{
  Serial.println("=> Kirim ke MQTT");
  if (!connectMqtt()) return;

  //*********************** calibrated values
  client.publish("agrisoil/val/moist", String(nilaiMoist).c_str());
  client.publish("agrisoil/val/ph", String(nilaiPH).c_str());
  client.publish("agrisoil/val/nit", String(nilaiN).c_str());
  client.publish("agrisoil/val/phos", String(nilaiP).c_str());
  client.publish("agrisoil/val/kal", String(nilaiK).c_str());

  //*********************** raw values, for calibration
  client.publish("agrisoil/raw/moist", String(moistADC).c_str());
  client.publish("agrisoil/raw/ph", String(phADC).c_str());
  client.publish("agrisoil/raw/npk1", String(npk1ADC).c_str());
  client.publish("agrisoil/raw/npk2", String(npk2ADC).c_str());

  //*********************** location
  if (gpsValid)
  {
    client.publish("agrisoil/loc/lat", String(latitude, 6).c_str());
    client.publish("agrisoil/loc/lng", String(longitude, 6).c_str());
  }

  // Let the last packets leave before we hang up (or fall asleep).
  client.loop();
  delay(100);
  client.disconnect();
}

bool connectMqtt()
{
  for (int attempt = 1; attempt <= MQTT_CONNECT_ATTEMPTS; attempt++)
  {
    if (client.connect(MQTT_CLIENT_ID)) return true;

    Serial.print("   MQTT gagal, rc=");
    Serial.print(client.state());
    Serial.println(" coba lagi 2 detik");
    delay(2000);
  }

  Serial.println("   MQTT nyerah, skip dulu");
  return false;
}

//=========================== NGROK PORT ===========================
int fetchMqttPort()
{
  Serial.println("=> Nanya port MQTT ke Node-RED...");

  WiFiClientSecure secureClient;
  secureClient.setInsecure(); // no certificate check, it's a soil sensor not a bank

  HTTPClient http;
  http.begin(secureClient, MQTT_PORT_URL);
  http.setAuthorization(httpsUname, httpsPassword);

  int port = MQTT_FALLBACK_PORT;
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    int answer = http.getString().toInt();
    if (answer > 0 && answer <= 65535)
    {
      port = answer;
    }
  }
  else
  {
    Serial.print("   Gagal, HTTP ");
    Serial.print(httpCode);
    Serial.println(", pakai port cadangan");
  }

  http.end();
  return port;
}

#endif
