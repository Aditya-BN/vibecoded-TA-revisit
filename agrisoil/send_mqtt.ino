//=========================== SEND: MQTT ===========================
// The broker sits behind an ngrok TCP tunnel, and ngrok hands out
// a new port every restart. Node-RED knows today's port, so we ask it first.
//
// Every topic starts with agrisoil/<DEVICE_ID>/ so many sensors can share one broker:
//   agrisoil/Alat3/reading    -> the whole reading as JSON (retained, dashboards use this)
//   agrisoil/Alat3/val/nit    -> single values, plain text (handy for quick gauges)
//   agrisoil/Alat3/raw/npk1   -> raw ADC counts, plain text
//   agrisoil/Alat3/loc/lat    -> location, only when GPS has a fix
#if SEND_TO_MQTT

#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

#define MQTT_HOST "0.tcp.ap.ngrok.io" // host only, no "tcp://"
#define MQTT_FALLBACK_PORT 12115      // used when Node-RED doesn't answer
#define MQTT_PORT_URL "https://nodered.agrisoil.my.id/getngrokport"
#define MQTT_CLIENT_ID "Agrisoil_" DEVICE_ID // must be unique per sensor
#define MQTT_CONNECT_ATTEMPTS 3

WiFiClient espClient;
PubSubClient client(espClient);

void beginMqtt()
{
  int port = fetchMqttPort();
  client.setServer(MQTT_HOST, port);
  client.setBufferSize(512); // default 256 is too small for the JSON reading

  Serial.print("=> MQTT broker: ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.println(port);
}

void sendToMqtt()
{
  Serial.println("=> Kirim ke MQTT");
  if (!connectMqtt()) return;

  //*********************** the whole reading, for dashboards
  // retained = the broker remembers the last one, so a dashboard that
  // opens later instantly sees every sensor that ever reported.
  publishReading();

  //*********************** calibrated values
  publishValue("val/moist", String(nilaiMoist));
  publishValue("val/ph", String(nilaiPH));
  publishValue("val/nit", String(nilaiN));
  publishValue("val/phos", String(nilaiP));
  publishValue("val/kal", String(nilaiK));

  //*********************** raw values, for calibration
  publishValue("raw/moist", String(moistADC));
  publishValue("raw/ph", String(phADC));
  publishValue("raw/npk1", String(npk1ADC));
  publishValue("raw/npk2", String(npk2ADC));

  //*********************** location
  if (gpsValid)
  {
    publishValue("loc/lat", String(latitude, 6));
    publishValue("loc/lng", String(longitude, 6));
  }

  // Let the last packets leave before we hang up (or fall asleep).
  client.loop();
  delay(100);
  client.disconnect();
}

// agrisoil/<DEVICE_ID>/<name>
void publishValue(String name, String value)
{
  String topic = "agrisoil/" + String(DEVICE_ID) + "/" + name;
  client.publish(topic.c_str(), value.c_str());
}

// {"device":"Alat3","waktu":"2026-11-11 12:23:08","n":201.59,...,"lat":-6.2,"lng":106.8}
void publishReading()
{
  String json = "{";
  json += "\"device\":\"" + String(DEVICE_ID) + "\",";
  json += "\"waktu\":\"" + waktu + "\",";
  json += "\"n\":" + String(nilaiN) + ",";
  json += "\"p\":" + String(nilaiP) + ",";
  json += "\"k\":" + String(nilaiK) + ",";
  json += "\"ph\":" + String(nilaiPH) + ",";
  json += "\"moist\":" + String(nilaiMoist) + ",";
  json += "\"npk1_raw\":" + String(npk1ADC) + ",";
  json += "\"npk2_raw\":" + String(npk2ADC) + ",";
  json += "\"moist_raw\":" + String(moistADC) + ",";
  json += "\"ph_raw\":" + String(phADC) + ",";
  json += "\"in_soil\":" + String(probeInSoil ? "true" : "false") + ",";

  if (gpsValid)
  {
    json += "\"lat\":" + String(latitude, 6) + ",";
    json += "\"lng\":" + String(longitude, 6);
  }
  else
  {
    json += "\"lat\":null,\"lng\":null";
  }
  json += "}";

  String topic = "agrisoil/" + String(DEVICE_ID) + "/reading";
  client.publish(topic.c_str(), json.c_str(), true); // true = retained
}

bool connectMqtt()
{
  for (int attempt = 1; attempt <= MQTT_CONNECT_ATTEMPTS; attempt++)
  {
    if (client.connect(MQTT_CLIENT_ID, mqttUser, mqttPassword)) return true;

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
