//=========================== SENSOR: ADS1115 ===========================
// Reading method #1. Two external 16-bit ADCs on I2C:
//   ADS 0x48 -> ch0: NPK probe 1, ch1: NPK probe 2
//   ADS 0x49 -> ch0: moisture,    ch1: pH
//
// Calibration came from comparing raw values with a "real" NPK meter,
// so the numbers below look like magic, but they're not. Mostly.
#if READ_METHOD == READ_WITH_ADS1115

#include <ADS1X15.h>

#define SAMPLES 10
#define SAMPLE_GAP_MS 500

// Moisture probe: ADC says 4680 in water and 11940 in thin air.
#define MOIST_ADC_WET 4680.0
#define MOIST_ADC_DRY 11940.0

ADS1115 ADS0(0x48);
ADS1115 ADS1(0x49);

void beginSensors()
{
  if (!ADS0.begin())
  {
    Serial.println("=> ADS 0x48 (NPK) nggak ketemu!");
  }
  if (!ADS1.begin())
  {
    Serial.println("=> ADS 0x49 (moist & pH) nggak ketemu!");
  }

  // gain 0 -> +-6.144V, one count = 0.1875 mV
  ADS0.setGain(0);
  ADS1.setGain(0);
}

void readSoil()
{
  //*********************** average a few samples
  // long on purpose: 10 x 16-bit readings won't fit in an int16_t
  long npk1Total = 0;
  long npk2Total = 0;
  long moistTotal = 0;
  long phTotal = 0;

  for (int i = 0; i < SAMPLES; i++)
  {
    npk1Total += ADS0.readADC(0);
    npk2Total += ADS0.readADC(1);
    moistTotal += ADS1.readADC(0);
    phTotal += ADS1.readADC(1);
    delay(SAMPLE_GAP_MS);
  }

  npk1ADC = npk1Total / SAMPLES;
  npk2ADC = npk2Total / SAMPLES;
  moistADC = moistTotal / SAMPLES;
  phADC = phTotal / SAMPLES;

  probeInSoil = true; // no way to tell with this board, trust the user

  //*********************** NPK
  // All three were calibrated against probe 1. Probe 2 is only sent raw.
  nilaiN = (-8.1731 * npk1ADC) + 46510;
  nilaiN = constrain(nilaiN, 0, 700);

  nilaiP = (-0.8921 * npk1ADC) + 5112;
  nilaiP = constrain(nilaiP, 0, 30);

  nilaiK = (21.2 * npk1ADC) - 120331;
  nilaiK = constrain(nilaiK, 0, 170);

  //*********************** moisture
  // dry (11940) -> 0%, wet (4680) -> 100%
  nilaiMoist = (MOIST_ADC_DRY - moistADC) / (MOIST_ADC_DRY - MOIST_ADC_WET) * 100;
  nilaiMoist = constrain(nilaiMoist, 0, 100);

  //*********************** pH
  // idk, just idk. I just lost 3 braincell tried to understand this.
  // https://depoinovasi.com/produk-975-sensor-ph-tanah-support-arduino.html
  // TODO: on volts this barely moves (always ~7.5). Re-check with buffer solution
  //       whether the original calibration was done in volts, mV, or raw counts.
  float phVolt = ADS1.toVoltage(phADC);
  nilaiPH = (-0.0136 * phVolt) + 7.5773;
  nilaiPH = constrain(nilaiPH, 1, 14);
}

#endif
