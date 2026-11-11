//=========================== SENSOR: ADS1115 ===========================
// Reading method #1. Two external 16-bit ADCs on I2C:
//   ADS 0x48 -> ch0: NPK probe 1, ch1: NPK probe 2
//   ADS 0x49 -> ch0: moisture,    ch1: pH
//
// Calibration came from comparing raw values with a "real" NPK meter,
// so the numbers below look like magic, but they're not. Mostly.
#if READ_METHOD == READ_WITH_ADS1115

#include <ADS1X15.h>

ADS1115 adsNpk(0x48);
ADS1115 adsSoil(0x49);

const uint8_t SAMPLES = 10;
const uint16_t SAMPLE_GAP_MS = 500;

// Moisture probe: ADC says 4680 in water and 11940 in thin air.
const float MOISTURE_RAW_WET = 4680;
const float MOISTURE_RAW_DRY = 11940;

void beginSensors()
{
  if (!adsNpk.begin())
  {
    Serial.println("=> ADS 0x48 (NPK) nggak ketemu!");
  }
  if (!adsSoil.begin())
  {
    Serial.println("=> ADS 0x49 (moist & pH) nggak ketemu!");
  }

  // gain 0 -> +-6.144V, one count = 0.1875 mV
  adsNpk.setGain(0);
  adsSoil.setGain(0);
}

SoilReading readSoil()
{
  SoilReading soil = {};

  //*********************** average a few samples
  // int32_t on purpose: 10 x 16-bit readings won't fit in an int16_t
  int32_t npkSum = 0, npk2Sum = 0, moistureSum = 0, phSum = 0;

  for (uint8_t i = 0; i < SAMPLES; i++)
  {
    npkSum += adsNpk.readADC(0);
    npk2Sum += adsNpk.readADC(1);
    moistureSum += adsSoil.readADC(0);
    phSum += adsSoil.readADC(1);
    delay(SAMPLE_GAP_MS);
  }

  soil.npkRaw = npkSum / SAMPLES;
  soil.npk2Raw = npk2Sum / SAMPLES;
  soil.moistureRaw = moistureSum / SAMPLES;
  soil.phRaw = phSum / SAMPLES;
  soil.probeInSoil = true; // no way to tell with this board, trust the user

  //*********************** NPK
  // All three were calibrated against probe 1. Probe 2 is only sent raw.
  soil.nitrogen = constrain(-8.1731f * soil.npkRaw + 46510.0f, 0.0f, 700.0f);
  soil.phosphorus = constrain(-0.8921f * soil.npkRaw + 5112.0f, 0.0f, 30.0f);
  soil.potassium = constrain(21.2f * soil.npkRaw - 120331.0f, 0.0f, 170.0f);

  //*********************** moisture
  // dry (11940) -> 0%, wet (4680) -> 100%
  float wetness = (MOISTURE_RAW_DRY - soil.moistureRaw) / (MOISTURE_RAW_DRY - MOISTURE_RAW_WET);
  soil.moisture = constrain(wetness * 100.0f, 0.0f, 100.0f);

  //*********************** pH
  // idk, just idk. I just lost 3 braincell tried to understand this.
  // https://depoinovasi.com/produk-975-sensor-ph-tanah-support-arduino.html
  // TODO: on volts this barely moves (always ~7.5). Re-check with buffer solution
  //       whether the original calibration was done in volts, mV, or raw counts.
  float phVolt = adsSoil.toVoltage(soil.phRaw);
  soil.ph = constrain(-0.0136f * phVolt + 7.5773f, 1.0f, 14.0f);

  return soil;
}

#endif
