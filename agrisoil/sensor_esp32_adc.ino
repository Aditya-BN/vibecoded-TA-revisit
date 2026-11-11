//=========================== SENSOR: ESP32 ADC ===========================
// Reading method #2. Straight into the ESP32's own 12-bit ADC.
// Use ADC1 pins only (32-39), ADC2 stops working once WiFi is on.
//
// The old prototype read everything off pin 35, so that's the default.
// Got separate wires for each probe? Just change the pins below.
#if READ_METHOD == READ_WITH_ESP32_ADC

#define NPK_PIN 35
#define MOIST_PIN 35
#define PH_PIN 35

#define SAMPLES 10
#define SAMPLE_GAP_MS 20

// Above this (on the 0-1023 scale) the probe is hanging in the air.
#define OUT_OF_SOIL_LEVEL 1010

void beginSensors()
{
  analogReadResolution(12); // 0 - 4095
}

void readSoil()
{
  npk1ADC = readAverage(NPK_PIN);
  npk2ADC = 0; // only one NPK probe on this board
  moistADC = readAverage(MOIST_PIN);
  phADC = readAverage(PH_PIN);

  // The calibration was made on a 10-bit scale (Arduino Uno days), keep it that way.
  int npk = map(npk1ADC, 0, 4095, 0, 1023);
  int moist = map(moistADC, 0, 4095, 0, 1023);
  int ph = map(phADC, 0, 4095, 0, 1023);

  probeInSoil = npk <= OUT_OF_SOIL_LEVEL;
  if (!probeInSoil)
  {
    Serial.println("=> Probe nggak nancep di tanah, semua nilai 0");
    nilaiN = 0;
    nilaiP = 0;
    nilaiK = 0;
    nilaiPH = 0;
    nilaiMoist = 0;
    return;
  }

  //*********************** NPK
  nilaiN = map(npk, 0, 1023, 350, 60);
  nilaiN = constrain(nilaiN, 0, 900);

  nilaiP = map(npk, 100, 1023, 32, 5);
  nilaiP = constrain(nilaiP, 0, 100);

  nilaiK = map(npk, 30, 1023, 210, 70);
  nilaiK = constrain(nilaiK, 0, 700);

  //*********************** moisture
  nilaiMoist = map(moist, 0, 1023, 100, 20);

  //*********************** pH
  float phScale = map(ph, 0, 950, 10, 35);
  nilaiPH = (-0.0693 * phScale) + 7.3855;
  nilaiPH = constrain(nilaiPH, 1, 14);
}

long readAverage(int pin)
{
  long total = 0;
  for (int i = 0; i < SAMPLES; i++)
  {
    total += analogRead(pin);
    delay(SAMPLE_GAP_MS);
  }
  return total / SAMPLES;
}

#endif
