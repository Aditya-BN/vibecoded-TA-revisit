# vibecoded-TA-revisit
Now I have claude!!!! So I can vibecode all of my TA lol

Rewrite of [codingTA](https://github.com/Aditya-BN/codingTA) + [npkgis](https://github.com/Aditya-BN/npkgis)
into one ESP32 sketch: **`agrisoil/`**.

## What it does
Read N, P, K, pH and moisture → tag with GPS + time → show on OLED → send to MQTT, Firebase and MySQL → sleep.

## Getting started
1. Copy `agrisoil/secret.example.h` to `agrisoil/secret.h` and fill in WiFi, Node-RED and Firebase credentials.
2. Open `agrisoil/agrisoil.ino` and set the switches in the **PILIH-PILIH DULU** block.
3. Board: *ESP32 Dev Module*, Partition Scheme: **Huge APP (3MB No OTA/1MB SPIFFS)**. Upload.
   (With Firebase on, the sketch is ~1.4 MB, too big for the default 1.2 MB partition on ESP32 core 3.x.)

## The switches
| Switch | Options |
| --- | --- |
| `READ_METHOD` | `READ_WITH_ADS1115` (2x ADS1115, codingTA calibration) or `READ_WITH_ESP32_ADC` (`analogRead`, npkgis calibration) |
| `SEND_TO_MQTT` / `SEND_TO_FIREBASE` / `SEND_TO_MYSQL` | `1` on, `0` off |
| `USE_GPS` | `1` on, `0` off |
| `USE_DEEP_SLEEP` | `1` measure once then sleep `SLEEP_SECONDS`, `0` stay awake and measure every `MEASURE_INTERVAL_MS` |
| `OLED_HEIGHT` | `64` or `32` |

A turned-off feature isn't compiled at all, so you don't even need its library installed.

## Libraries
| Library | Needed for |
| --- | --- |
| ADS1X15 (Rob Tillaart) | `READ_WITH_ADS1115` |
| Adafruit SSD1306 + Adafruit GFX | OLED |
| TinyGPSPlus (Mikal Hart) | `USE_GPS` |
| PubSubClient (Nick O'Leary) | `SEND_TO_MQTT` |
| Firebase ESP32 Client (Mobizt) | `SEND_TO_FIREBASE` |

## Wiring
| Part | Pins |
| --- | --- |
| OLED, ADS1115 0x48 & 0x49 | I2C: SDA 21, SCL 22 |
| GPS | RX2 16, TX2 17 |
| Battery relay 1 / 2 | 18 / 19 |
| ESP32 ADC probes | 35 by default (change in `sensor_esp32_adc.ino`) |
