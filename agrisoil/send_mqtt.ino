//=========================== SEND: MQTT ===========================
// The broker sits behind an ngrok TCP tunnel, and ngrok hands out
// a new port every restart. Node-RED knows today's port, so we ask it first.
#if SEND_TO_MQTT

#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

const char *MQTT_HOST = "0.tcp.ap.ngrok.io"; // host only, no "tcp://"
const uint16_t MQTT_FALLBACK_PORT = 12115;   // used when Node-RED doesn't answer
const char *MQTT_PORT_LOOKUP_URL = "https://nodered.agrisoil.my.id/getngrokport";
const char *MQTT_CLIENT_ID = "Agrisoil_1";
const uint8_t MQTT_CONNECT_ATTEMPTS = 3;

WiFiClient mqttNetwork;
PubSubClient mqtt(mqttNetwork);

void beginMqtt()
{
  uint16_t port = fetchMqttPort();
  mqtt.setServer(MQTT_HOST, port);
  Serial.printf("=> MQTT broker: %s:%u\n", MQTT_HOST, port);
}

void sendToMqtt(const SoilReading &soil, const GpsFix &where)
{
  Serial.println("=> Kirim ke MQTT");
  if (!connectMqtt()) return;

  //*********************** calibrated values
  publishNumber("agrisoil/val/moist", soil.moisture);
  publishNumber("agrisoil/val/ph", soil.ph);
  publishNumber("agrisoil/val/nit", soil.nitrogen);
  publishNumber("agrisoil/val/phos", soil.phosphorus);
  publishNumber("agrisoil/val/kal", soil.potassium);

  //*********************** raw values, for calibration
  publishNumber("agrisoil/raw/moist", soil.moistureRaw);
  publishNumber("agrisoil/raw/ph", soil.phRaw);
  publishNumber("agrisoil/raw/npk1", soil.npkRaw);
  publishNumber("agrisoil/raw/npk2", soil.npk2Raw);

  //*********************** location
  if (where.valid)
  {
    publishNumber("agrisoil/loc/lat", where.latitude, 6);
    publishNumber("agrisoil/loc/lng", where.longitude, 6);
  }

  // Let the last packets leave before we hang up (or fall asleep).
  mqtt.loop();
  delay(100);
  mqtt.disconnect();
}

bool connectMqtt()
{
  for (uint8_t attempt = 1; attempt <= MQTT_CONNECT_ATTEMPTS; attempt++)
  {
    if (mqtt.connect(MQTT_CLIENT_ID)) return true;

    Serial.printf("   MQTT gagal (rc=%d), coba lagi %u/%u\n", mqtt.state(), attempt, MQTT_CONNECT_ATTEMPTS);
    delay(2000);
  }

  Serial.println("   MQTT nyerah, skip dulu");
  return false;
}

void publishNumber(const char *topic, double value, uint8_t decimals)
{
  char text[20];
  dtostrf(value, 1, decimals, text);
  mqtt.publish(topic, text);
}

void publishNumber(const char *topic, double value)
{
  publishNumber(topic, value, 2);
}

//=========================== NGROK PORT ===========================
uint16_t fetchMqttPort()
{
  Serial.println("=> Nanya port MQTT ke Node-RED...");

  WiFiClientSecure tls;
  tls.setInsecure(); // no certificate pinning, it's a soil sensor not a bank

  HTTPClient http;
  http.begin(tls, MQTT_PORT_LOOKUP_URL);
  http.setAuthorization(httpsUname, httpsPassword);

  uint16_t port = MQTT_FALLBACK_PORT;
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    long answer = http.getString().toInt();
    if (answer > 0 && answer <= 65535)
    {
      port = answer;
    }
  }
  else
  {
    Serial.printf("   Gagal (HTTP %d), pakai port cadangan\n", httpCode);
  }

  http.end();
  return port;
}

#endif
