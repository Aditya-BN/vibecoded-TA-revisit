//=========================== AGRISOIL ===========================
// Soil checker: reads N, P, K, pH and moisture, tags it with GPS + time,
// shows it on the OLED, then ships it to MQTT, Firebase and MySQL.
//
// Every other tab is one "part" of the device:
//   clock            -> NTP time, for the timestamp
//   display          -> OLED + serial printout
//   gps              -> where am I
//   power            -> sensor relay, boot counter, deep sleep
//   send_firebase    -> Firebase realtime database
//   send_mqtt        -> MQTT broker (behind ngrok)
//   send_mysql       -> PHP/MySQL dashboard
//   sensor_ads1115   -> reading method #1, external 16-bit ADC
//   sensor_esp32_adc -> reading method #2, the ESP32's own ADC
//   wifi             -> getting online
//
// Read this file top to bottom and you know what the device does.
// Open a tab only if you want to know *how*.

//=========================== PILIH-PILIH DULU ===========================
// Everything you'd want to flip lives here. 1 = on, 0 = off.

// Who am I? Every sensor needs its own name.
// It goes into the MQTT topics, the MQTT client id and the Firebase paths,
// so a new sensor = new name here = its own dashboard. Letters, numbers, _ only.
#define DEVICE_ID "Alat3"

// How should we read the sensors?
//   READ_WITH_ADS1115   -> 2x ADS1115 at 0x48 & 0x49 (calibration from codingTA)
//   READ_WITH_ESP32_ADC -> analogRead() on the ESP32 (calibration from npkgis)
#define READ_WITH_ADS1115 1
#define READ_WITH_ESP32_ADC 2

#define READ_METHOD READ_WITH_ADS1115

// Where should the data go?
#define SEND_TO_MQTT 1
#define SEND_TO_FIREBASE 1
#define SEND_TO_MYSQL 1

// Do we have a GPS module plugged in?
#define USE_GPS 1
#define GPS_FIX_TIMEOUT_MS 30000 // how long to wait for satellites after waking up

// 1 -> measure once, send, then deep sleep (battery mode)
// 0 -> stay awake and measure every MEASURE_INTERVAL_MS (bench mode)
#define USE_DEEP_SLEEP 1
#define SLEEP_SECONDS 10
#define MEASURE_INTERVAL_MS 5000

// OLED size. 64 shows everything at once, 32 flips between two pages.
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDRESS 0x3C

#if READ_METHOD != READ_WITH_ADS1115 && READ_METHOD != READ_WITH_ESP32_ADC
#error "READ_METHOD must be READ_WITH_ADS1115 or READ_WITH_ESP32_ADC"
#endif

//=========================== LIBRARIES ===========================
#include <Wire.h>
#include "secret.h" // no secret.h yet? copy secret.example.h and fill it in

//=========================== HASIL BACAAN ===========================
// Filled by readSoil(), readGps() and readClock().
// Every other tab just reads these.

// soil, already calibrated
float nilaiN;     // ppm
float nilaiP;     // ppm
float nilaiK;     // ppm
float nilaiPH;    // 1 - 14
float nilaiMoist; // %

// soil, raw ADC counts (you'll want these when re-calibrating)
long npk1ADC;
long npk2ADC;
long moistADC;
long phADC;

bool probeInSoil;

// where
bool gpsValid;
double latitude;
double longitude;
int satellites;

// when
String waktu;

unsigned long previousMillis = 0;

//=========================== SETUP ===========================
void setup()
{
  Serial.begin(115200);
  sayHello();

  powerOnSensors();
  Wire.begin();

  beginDisplay();
  beginSensors();
  beginGps();

  showMessage("Connecting wifi...", "");
  beginWifi();
  syncClock();
  beginUploads();

#if USE_DEEP_SLEEP
  //******************** battery mode: one shot, then turu
  showMessage("Nyari satelit...", "");
  waitForGpsFix();

  measureAndSend();
  goToSleep();
  // seriously, don't put any code below this line,
  // anything here will be sent to oblivion just like my.. nvm.
#endif
}

//=========================== LOOP ===========================
// Only reached when USE_DEEP_SLEEP is 0.
void loop()
{
  feedGps();

  if (millis() - previousMillis >= MEASURE_INTERVAL_MS)
  {
    previousMillis = millis();
    measureAndSend();
  }
}

//=========================== ONE CYCLE ===========================
void measureAndSend()
{
  showMessage("Membaca", "sensor...");

  readSoil();
  readGps();
  readClock();

  printReading();
  showReading();

  sendEverywhere();
}

//=========================== UPLOADS ===========================
void beginUploads()
{
#if SEND_TO_MQTT
  beginMqtt();
#endif
#if SEND_TO_FIREBASE
  beginFirebase();
#endif
}

void sendEverywhere()
{
  if (!ensureWifi())
  {
    Serial.println("=> Nggak ada wifi, data cuma ditampilin aja");
    return;
  }

#if SEND_TO_MQTT
  sendToMqtt();
#endif
#if SEND_TO_FIREBASE
  sendToFirebase();
#endif
#if SEND_TO_MYSQL
  sendToMysql();
#endif
}
