//=========================== SENSOR: ESP32 ADC ===========================
// Reading method #2. Straight into the ESP32's own 12-bit ADC.
// Use ADC1 pins only (32-39), ADC2 stops working once WiFi is on.
//
// The old prototype read everything off pin 35, so that's the default.
// Got separate wires for each probe? Just change the pins below.
#if READ_METHOD == READ_WITH_ESP32_ADC

const uint8_t NPK_PIN = 35;
const uint8_t MOISTURE_PIN = 35;
const uint8_t PH_PIN = 35;

const uint8_t SAMPLES = 10;
const uint16_t SAMPLE_GAP_MS = 20;

// Above this (on the 0-1023 scale) the probe is hanging in the air.
const long OUT_OF_SOIL_LEVEL = 1010;

void beginSensors()
{
  analogReadResolution(12); // 0 - 4095
}

SoilReading readSoil()
{
  SoilReading soil = {}; // everything starts at 0

  soil.npkRaw = readAverage(NPK_PIN);
  soil.moistureRaw = readAverage(MOISTURE_PIN);
  soil.phRaw = readAverage(PH_PIN);

  // The calibration was made on a 10-bit scale (Arduino Uno days), keep it that way.
  long npk = toTenBit(soil.npkRaw);
  long moisture = toTenBit(soil.moistureRaw);
  long ph = toTenBit(soil.phRaw);

  soil.probeInSoil = npk <= OUT_OF_SOIL_LEVEL;
  if (!soil.probeInSoil)
  {
    Serial.println("=> Probe nggak nancep di tanah, semua nilai 0");
    return soil;
  }

  //*********************** NPK
  soil.nitrogen = constrain(map(npk, 0, 1023, 350, 60), 0L, 900L);
  soil.phosphorus = constrain(map(npk, 100, 1023, 32, 5), 0L, 100L);
  soil.potassium = constrain(map(npk, 30, 1023, 210, 70), 0L, 700L);

  //*********************** moisture
  soil.moisture = map(moisture, 0, 1023, 100, 20);

  //*********************** pH
  float phScale = map(ph, 0, 950, 10, 35);
  soil.ph = constrain(-0.0693f * phScale + 7.3855f, 1.0f, 14.0f);

  return soil;
}

int32_t readAverage(uint8_t pin)
{
  int32_t sum = 0;
  for (uint8_t i = 0; i < SAMPLES; i++)
  {
    sum += analogRead(pin);
    delay(SAMPLE_GAP_MS);
  }
  return sum / SAMPLES;
}

long toTenBit(int32_t raw)
{
  return map(raw, 0, 4095, 0, 1023);
}

#endif
