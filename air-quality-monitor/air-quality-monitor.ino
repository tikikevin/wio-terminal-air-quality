/*****************************************************************************************************************
   
   Air Quality Terminal 
   Original Author: Salman Faris
   Original Date: 01/09/2020
   Last Updates: 04/25/2025

   Required Libraries:
   - <Multichannel_Gas_GMXXX.h>: https://github.com/Seeed-Studio/Seeed_Multichannel_Gas_Sensor/archive/master.zip
   - <DHT.h>: https://github.com/Seeed-Studio/Grove_Temperature_And_Humidity_Sensor/archive/refs/heads/master.zip

*****************************************************************************************************************/

#include <TFT_eSPI.h>
#include <Multichannel_Gas_GMXXX.h> 
#include <Wire.h>
#include <DHT.h>

// Gas sensor object (I2C communication)
GAS_GMXXX<TwoWire> gas;

// Display objects
TFT_eSPI tft;
TFT_eSprite spr = TFT_eSprite(&tft);  // Sprite for off-screen rendering

// Sensor data variables
unsigned int no2, c2h5ch, voc, co;

// DHT sensor configuration
#define DHTPIN 0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize TFT display
  tft.begin();
  tft.setRotation(3);

  // Initialize gas sensor
  gas.begin(Wire, 0x08);

  // Initialize DHT sensor
  dht.begin();

  // Setup display layout
  setupDisplayLayout();
}

void loop() {
  // Read and display VOC data
  voc = gas.getGM502B();
  voc = constrain(voc, 0, 999);  // Limit value to 999 ppm
  displayData("VOC", voc, 15, 100);

  // Read and display CO data
  co = gas.getGM702B();
  co = constrain(co, 0, 999);  // Limit value to 999 ppm
  displayData("CO", co, 15, 185);

  // Read and display temperature
  float temperature = dht.readTemperature();
  int tempInt = static_cast<int>(temperature);  // Convert to integer
  displayData("Temp", tempInt, (tft.width() / 2) - 1, 100);

  // Read and display NO2 data
  no2 = gas.getGM102B();
  no2 = constrain(no2, 0, 999);  // Limit value to 999 ppm
  displayData("NO2", no2, ((tft.width() / 2) + (tft.width() / 2) / 2), 97);

  // Read and display humidity
  float humidity = dht.readHumidity();
  humidity = constrain(humidity, 0, 99);  // Limit value to 99%
  displayData("Humi", static_cast<int>(humidity), (tft.width() / 2) - 1, (tft.height() / 2) + 67);

  // Read and display Ethyl (C2H5CH) data
  c2h5ch = gas.getGM302B();
  c2h5ch = constrain(c2h5ch, 0, 999);  // Limit value to 999 ppm
  displayData("Ethyl", c2h5ch, ((tft.width() / 2) + (tft.width() / 2) / 2), (tft.height() / 2) + 67);

  // Delay for 5 seconds before the next update
  delay(5000);
}

// Function to set up the display layout
void setupDisplayLayout() {
  // Clear the screen
  tft.fillScreen(TFT_BLACK);

  // Display header
  tft.setFreeFont(&FreeSansBoldOblique18pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Air Quality", 70, 10, 1);

  // Draw header line
  for (int8_t i = 0; i < 5; i++) {
    tft.drawLine(0, 50 + i, tft.width(), 50 + i, TFT_GREEN);
  }

  // Draw rectangles and labels for each sensor
  drawSensorBox(5, 60, (tft.width() / 2) - 20, tft.height() - 65, TFT_WHITE, "VOC", "ppm", 7, 65, 55, 108);
  drawSensorBox(5, 150, (tft.width() / 2) - 20, tft.height() - 65, TFT_WHITE, "CO", "ppm", 7, 150, 55, 193);
  drawSensorBox((tft.width() / 2) - 10, 60, (tft.width() / 2) / 2, (tft.height() - 65) / 2, TFT_BLUE, "Temp", "C", (tft.width() / 2) - 1, 70, (tft.width() / 2) + 40, 100);
  drawSensorBox(((tft.width() / 2) + (tft.width() / 2) / 2) - 5, 60, (tft.width() / 2) / 2, (tft.height() - 65) / 2, TFT_BLUE, "NO2", "ppm", ((tft.width() / 2) + (tft.width() / 2) / 2), 70, ((tft.width() / 2) + (tft.width() / 2) / 2) + 30, 120);
  drawSensorBox((tft.width() / 2) - 10, (tft.height() / 2) + 30, (tft.width() / 2) / 2, (tft.height() - 65) / 2, TFT_BLUE, "Humi", "%", (tft.width() / 2) - 1, (tft.height() / 2) + 40, (tft.width() / 2) + 30, (tft.height() / 2) + 70);
  drawSensorBox(((tft.width() / 2) + (tft.width() / 2) / 2) - 5, (tft.height() / 2) + 30, (tft.width() / 2) / 2, (tft.height() - 65) / 2, TFT_BLUE, "Ethyl", "ppm", ((tft.width() / 2) + (tft.width() / 2) / 2), (tft.height() / 2) + 40, ((tft.width() / 2) + (tft.width() / 2) / 2) + 30, (tft.height() / 2) + 90);
}

// Function to draw a sensor box with labels
void drawSensorBox(int x, int y, int w, int h, uint16_t color, const char* label, const char* unit, int labelX, int labelY, int unitX, int unitY) {
  tft.drawRoundRect(x, y, w, h, 10, color);
  tft.setFreeFont(&FreeSansBoldOblique9pt7b);
  tft.setTextColor(TFT_RED);
  tft.drawString(label, labelX, labelY, 1);
  tft.setTextColor(TFT_GREEN);
  tft.drawString(unit, unitX, unitY, 1);
}

// Function to display sensor data on the screen
void displayData(const char* label, int value, int x, int y) {
  Serial.print(label);
  Serial.print(": ");
  Serial.print(value);
  Serial.println(" ppm");

  spr.createSprite(45, 30);
  spr.fillSprite(TFT_BLACK);
  spr.setFreeFont(&FreeSansBoldOblique12pt7b);
  spr.setTextColor(TFT_WHITE);
  spr.drawNumber(value, 0, 0, 1);
  spr.pushSprite(x, y);
  spr.deleteSprite();
}