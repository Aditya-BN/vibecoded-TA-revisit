//=========================== DISPLAY ===========================
// Everything a human gets to see: the OLED and the serial monitor.
// No OLED plugged in? Fine, every function here just quietly skips.

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PAGE_FLIP_MS 3000 // small screen only

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1); // -1 = no reset pin
bool oledAvailable = false;

void beginDisplay()
{
  oledAvailable = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  if (!oledAvailable)
  {
    Serial.println(F("=> OLED nggak ketemu, lanjut tanpa layar"));
    return;
  }

  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.display();
}

//=========================== OLED ===========================
void showMessage(String title, String detail)
{
  if (!oledAvailable) return;

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(title);
  display.setCursor(0, 16);
  display.println(detail);
  display.display();
}

// Big screen: both pages stacked.
// Small screen: NPK first, then pH & moisture (which stays while we sleep).
void showReading()
{
  if (!oledAvailable) return;

  display.clearDisplay();
  display.setTextSize(2);

  if (OLED_HEIGHT >= 64)
  {
    drawNPK(0);
    drawPHMoist(32);
    display.display();
    return;
  }

  drawNPK(0);
  display.display();
  delay(PAGE_FLIP_MS);

  display.clearDisplay();
  drawPHMoist(0);
  display.display();
}

// Text size 2 = 16 px tall, so one page is exactly 32 px.
//  N     P     K
//  350   12    150
void drawNPK(int top)
{
  display.setCursor(0, top);
  display.print("N");
  display.setCursor(0, top + 16);
  display.print(nilaiN, 0);

  display.setCursor(45, top);
  display.print("P");
  display.setCursor(45, top + 16);
  display.print(nilaiP, 0);

  display.setCursor(90, top);
  display.print("K");
  display.setCursor(90, top + 16);
  display.print(nilaiK, 0);
}

//  pH    Moist
//  6.8   54%
void drawPHMoist(int top)
{
  display.setCursor(0, top);
  display.print("pH");
  display.setCursor(0, top + 16);
  display.print(nilaiPH, 1);

  display.setCursor(60, top);
  display.print("Moist");
  display.setCursor(60, top + 16);
  display.print(nilaiMoist, 0);
  display.print("%");
}

//=========================== SERIAL ===========================
void printReading()
{
  Serial.println("----------------------------------------");
  Serial.println("Waktu    : " + waktu);

  Serial.print("N        : ");
  Serial.print(nilaiN);
  Serial.println(" ppm");

  Serial.print("P        : ");
  Serial.print(nilaiP);
  Serial.println(" ppm");

  Serial.print("K        : ");
  Serial.print(nilaiK);
  Serial.println(" ppm");

  Serial.print("pH       : ");
  Serial.println(nilaiPH);

  Serial.print("Moisture : ");
  Serial.print(nilaiMoist);
  Serial.println(" %");

  Serial.print("Raw      : npk1=");
  Serial.print(npk1ADC);
  Serial.print(" npk2=");
  Serial.print(npk2ADC);
  Serial.print(" moist=");
  Serial.print(moistADC);
  Serial.print(" ph=");
  Serial.println(phADC);

  if (gpsValid)
  {
    Serial.print("Lokasi   : ");
    Serial.print(latitude, 6);
    Serial.print(", ");
    Serial.print(longitude, 6);
    Serial.print(" (");
    Serial.print(satellites);
    Serial.println(" satelit)");
  }
  else
  {
    Serial.println("Lokasi   : belum dapet sinyal GPS");
  }
  Serial.println("----------------------------------------");
}
