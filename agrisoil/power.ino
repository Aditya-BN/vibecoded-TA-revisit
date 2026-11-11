//=========================== POWER ===========================
// Battery relays, boot counter and deep sleep.
// "Hidup tak segan, mati tak mau." -> mending turu aja.

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define RELAY_BATTERY_1_PIN 18
#define RELAY_BATTERY_2_PIN 19

// Lives in RTC memory, so it survives deep sleep (but not a power cut).
RTC_DATA_ATTR int bootCount = 0;

//=========================== WAKE UP ===========================
void sayHello()
{
  // The relays + WiFi kick make the voltage dip at boot,
  // the brownout detector panics and resets us. Shush.
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  ++bootCount;
  Serial.println();
  Serial.println("Boot number: " + String(bootCount));
  printWakeupReason();
}

void printWakeupReason()
{
  switch (esp_sleep_get_wakeup_cause())
  {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
    default: Serial.println("Wakeup was not caused by deep sleep, fresh boot"); break;
  }
}

//=========================== RELAY ===========================
void powerOnSensors()
{
  Serial.println("=> Menyalakan relay baterai 1");
  pinMode(RELAY_BATTERY_1_PIN, OUTPUT);
  pinMode(RELAY_BATTERY_2_PIN, OUTPUT);
  digitalWrite(RELAY_BATTERY_1_PIN, HIGH);
  digitalWrite(RELAY_BATTERY_2_PIN, LOW); // battery 2 is the spare, stay off
}

void powerOffSensors()
{
  digitalWrite(RELAY_BATTERY_1_PIN, LOW);
  digitalWrite(RELAY_BATTERY_2_PIN, LOW);
}

//=========================== TURU ===========================
void goToSleep()
{
  powerOffSensors();

  esp_sleep_enable_timer_wakeup(SLEEP_SECONDS * 1000000ULL);
  Serial.println("=> Hibernasi " + String(SLEEP_SECONDS) + " detik. Good night.");
  Serial.flush();

  esp_deep_sleep_start();
}
