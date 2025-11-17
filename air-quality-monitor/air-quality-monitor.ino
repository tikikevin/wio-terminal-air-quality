/**
 * @file air-quality-monitor.ino
 * @brief Air Quality Monitoring System for Wio Terminal
 * 
 * This program monitors air quality using various sensors and displays the data on a TFT screen.
 * It measures VOC, CO, NO2, Ethyl (C2H5CH), temperature, and humidity levels.
 * 
 * @author Salman Faris
 * @date Original: 01/09/2020, Last Updated: 04/25/2025
 * 
 * @details
 * - The program uses the Seeed Multichannel Gas Sensor library for gas measurements.
 * - The DHT library is used for temperature and humidity readings.
 * - Data is displayed on a TFT screen using the TFT_eSPI library.
 * - Sensor values are constrained to reasonable ranges for display purposes.
 * - The display layout is organized into labeled sections for each sensor.
 * 
 * @dependencies
 * - <Multichannel_Gas_GMXXX.h>: https://github.com/Seeed-Studio/Seeed_Multichannel_Gas_Sensor/archive/master.zip
 * - <DHT.h>: https://github.com/Seeed-Studio/Grove_Temperature_And_Humidity_Sensor/archive/refs/heads/master.zip
 * - <TFT_eSPI.h>: https://github.com/Bodmer/TFT_eSPI
 * 
 * @hardware
 * - Wio Terminal
 * - Multichannel Gas Sensor
 * - DHT11 Temperature and Humidity Sensor
 * 
 * @functions
 * - setup(): Initializes the sensors, display, and layout.
 * - loop(): Reads sensor data, processes it, and updates the display.
 * - setupDisplayLayout(): Configures the layout of the TFT screen.
 * - drawSensorBox(): Draws labeled boxes for each sensor on the display.
 * - displayData(): Displays sensor readings on the screen and logs them to the serial monitor.
 * 
 * @license
 * This code is provided as-is without any warranty. Use at your own risk.
 */

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
  // Configuration constants
  constexpr uint32_t SERIAL_BAUD = 115200UL;
  constexpr uint8_t GAS_I2C_ADDR = 0x08;
  constexpr unsigned long SERIAL_TIMEOUT_MS = 2000;

  // Initialize serial for debugging (wait a short time for USB-serial to enumerate)
  Serial.begin(SERIAL_BAUD);
  unsigned long start = millis();
  while (!Serial && (millis() - start) < SERIAL_TIMEOUT_MS) {
    delay(10);
  }
  Serial.println("Initializing...");

  // Initialize I2C and set a safe clock frequency
  Wire.begin();
  Wire.setClock(100000); // 100 kHz

  // Initialize TFT display and clear screen
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  // Initialize gas sensor (I2C). Allow time for power-up and take a few
  // initial/discarded readings to let the sensor stabilize.
  gas.begin(Wire, GAS_I2C_ADDR);
  delay(200); // sensor power-up
  for (int i = 0; i < 3; ++i) {
    (void)gas.getGM502B();
    (void)gas.getGM702B();
    (void)gas.getGM102B();
    (void)gas.getGM302B();
    delay(100);
  }
  Serial.println("Gas sensor initialized (warm-up complete)");

  // Initialize DHT sensor
  dht.begin();
  Serial.println("DHT sensor initialized");

  // Prepare display layout (static UI elements)
  setupDisplayLayout();

  Serial.println("Setup complete");
}

/**
 * loop()
 * - Non-blocking periodic sensor read + display update.
 * - Uses millis() to avoid blocking delay(), validates DHT reads,
 *   constrains gas sensor values to a safe display range (0-999 ppm),
 *   and logs sensor read failures to Serial.
 */
void loop() {
  constexpr unsigned long UPDATE_INTERVAL_MS = 5000UL; // update every 5 seconds
  static unsigned long lastUpdate = 0;
  unsigned long now = millis();

  // Return early if it's not time to update yet (non-blocking)
  if (now - lastUpdate < UPDATE_INTERVAL_MS) {
    return;
  }
  lastUpdate = now;

  // --- VOC (GM502B) ---
  {
    unsigned int raw = gas.getGM502B();
    unsigned int value = static_cast<unsigned int>(constrain(raw, 0u, 999u));
    displayData("VOC", value, 15, 100);
  }

  // --- CO (GM702B) ---
  {
    unsigned int raw = gas.getGM702B();
    unsigned int value = static_cast<unsigned int>(constrain(raw, 0u, 999u));
    displayData("CO", value, 15, 185);
  }

  // --- Temperature (DHT) ---
  {
    float temperature = dht.readTemperature();
    int tempInt;
    if (isnan(temperature)) {
      Serial.println("Warning: Temperature read failed (NaN)");
      tempInt = 0; // fallback display value
    } else {
      tempInt = static_cast<int>(round(temperature));
    }
    displayData("Temp", tempInt, (tft.width() / 2) - 1, 100);
  }

  // --- NO2 (GM102B) ---
  {
    unsigned int raw = gas.getGM102B();
    unsigned int value = static_cast<unsigned int>(constrain(raw, 0u, 999u));
    displayData("NO2", value, ((tft.width() / 2) + (tft.width() / 2) / 2), 97);
  }

  // --- Humidity (DHT) ---
  {
    float humidity = dht.readHumidity();
    int humInt;
    if (isnan(humidity)) {
      Serial.println("Warning: Humidity read failed (NaN)");
      humInt = 0; // fallback display value
    } else {
      humInt = static_cast<int>(round(constrain(humidity, 0.0f, 99.0f)));
    }
    displayData("Humi", humInt, (tft.width() / 2) - 1, (tft.height() / 2) + 67);
  }

  // --- Ethyl (GM302B) ---
  {
    unsigned int raw = gas.getGM302B();
    unsigned int value = static_cast<unsigned int>(constrain(raw, 0u, 999u));
    displayData("Ethyl", value, ((tft.width() / 2) + (tft.width() / 2) / 2), (tft.height() / 2) + 67);
  }

  // Yield to background tasks (WiFi/USB/etc.) if available
  yield();
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
  drawSensorBox((tft.width() / 2) - 10, 60, (tft.width() / 2) / 2, (tft.height() - 65) / 2, TFT_BLUE, "Temp", "C", (tft.width() / 2) - 1, 70, (tft.width() / 2) + 45, 100);
  drawSensorBox(((tft.width() / 2) + (tft.width() / 2) / 2) - 5, 60, (tft.width() / 2) / 2, (tft.height() - 65) / 2, TFT_BLUE, "NO2", "ppm", ((tft.width() / 2) + (tft.width() / 2) / 2), 70, ((tft.width() / 2) + (tft.width() / 2) / 2) + 30, 120);
  drawSensorBox((tft.width() / 2) - 10, (tft.height() / 2) + 30, (tft.width() / 2) / 2, (tft.height() - 65) / 2, TFT_BLUE, "Humi", "%", (tft.width() / 2) - 1, (tft.height() / 2) + 40, (tft.width() / 2) + 45, (tft.height() / 2) + 70);
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