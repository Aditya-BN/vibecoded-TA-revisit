//=========================== DISPLAY ===========================
// Everything a human gets to see: the OLED and the serial monitor.
// No OLED plugged in? Fine, every function here just quietly skips.

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1); // -1 = no reset pin
bool oledAvailable = false;

const uint16_t PAGE_FLIP_MS = 3000; // small screen only

void beginDisplay()
{
  oledAvailable = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  if (!oledAvailable)
  {
    Serial.println(F("=> OLED nggak ketemu, lanjut tanpa layar"));
    return;
  }

  oled.setTextColor(SSD1306_WHITE);
  oled.clearDisplay();
  oled.display();
}

//=========================== OLED ===========================
void showMessage(const char *title, const char *detail)
{
  if (!oledAvailable) return;

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println(title);
  oled.setCursor(0, 16);
  oled.println(detail);
  oled.display();
}

// Big screen: both pages stacked.
// Small screen: NPK first, then pH & moisture (which stays while we sleep).
void showReading(const SoilReading &soil)
{
  if (!oledAvailable) return;

  oled.clearDisplay();
  oled.setTextSize(2);

  if (OLED_HEIGHT >= 64)
  {
    drawNpkPage(soil, 0);
    drawPhMoisturePage(soil, 32);
    oled.display();
    return;
  }

  drawNpkPage(soil, 0);
  oled.display();
  delay(PAGE_FLIP_MS);

  oled.clearDisplay();
  drawPhMoisturePage(soil, 0);
  oled.display();
}

//  N     P     K
//  350   12    150
void drawNpkPage(const SoilReading &soil, int16_t top)
{
  drawColumn(0, top, "N", soil.nitrogen, 0, "");
  drawColumn(45, top, "P", soil.phosphorus, 0, "");
  drawColumn(90, top, "K", soil.potassium, 0, "");
}

//  pH    Moist
//  6.8   54%
void drawPhMoisturePage(const SoilReading &soil, int16_t top)
{
  drawColumn(0, top, "pH", soil.ph, 1, "");
  drawColumn(60, top, "Moist", soil.moisture, 0, "%");
}

// Text size 2 = 16 px tall, so one column is exactly 32 px.
void drawColumn(int16_t left, int16_t top, const char *label, float value, uint8_t decimals, const char *unit)
{
  oled.setCursor(left, top);
  oled.print(label);
  oled.setCursor(left, top + 16);
  oled.print(value, decimals);
  oled.print(unit);
}

//=========================== SERIAL ===========================
void printReading(const SoilReading &soil, const GpsFix &where, const String &when)
{
  Serial.println("----------------------------------------");
  Serial.println("Waktu    : " + when);

  Serial.printf("N        : %.2f ppm\n", soil.nitrogen);
  Serial.printf("P        : %.2f ppm\n", soil.phosphorus);
  Serial.printf("K        : %.2f ppm\n", soil.potassium);
  Serial.printf("pH       : %.2f\n", soil.ph);
  Serial.printf("Moisture : %.2f %%\n", soil.moisture);

  Serial.printf("Raw      : npk=%ld npk2=%ld moist=%ld ph=%ld\n",
                (long)soil.npkRaw, (long)soil.npk2Raw, (long)soil.moistureRaw, (long)soil.phRaw);

  if (where.valid)
  {
    Serial.printf("Lokasi   : %.6f, %.6f (%lu satelit)\n",
                  where.latitude, where.longitude, (unsigned long)where.satellites);
  }
  else
  {
    Serial.println("Lokasi   : belum dapet sinyal GPS");
  }
  Serial.println("----------------------------------------");
}
