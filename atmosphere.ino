/**
 * aqDisplay displays particulate matter readings.
 * Board: Adafruit PyBadge M4 Express (SAMD51)
 * Sensor: Adafruit PM2.5 sensor via I2C
 * Library: Adafruit PM25 AQI
 */
#include <Adafruit_Arcada.h>
#include <Adafruit_PM25AQI.h>

Adafruit_Arcada arcada;
const int numMenuItems = 2;
const char *selection[numMenuItems] = {"Display AQI", "Display PSI"};

Adafruit_PM25AQI aqSensor = Adafruit_PM25AQI();
int previousAQI = 501;

const long interval = 1000;
unsigned long previousMillis = 0;
bool shouldRedraw = true;

void setup() {
  if (!arcada.arcadaBegin()) {
    //Serial.print("Arcada: Failed to begin");
    while (1);
  }
  arcada.displayBegin();
  arcada.setBacklight(64);
  arcada.display->fillScreen(ARCADA_BLUE); //28,113,187  //#1C71BB
  arcada.display->setTextColor(ARCADA_WHITE);
  arcada.display->setCursor(2, 2);
  arcada.display->setTextWrap(true);
  arcada.display->setTextSize(1);
  arcada.display->println("PMSA003I");
	// Wait two seconds for sensor to boot up!
  delay(2000);
  if (! aqSensor.begin_I2C()) { // connect to the sensor over I2C
    arcada.haltBox("Air Quality sensor not found!");
    //while (1) delay(10);
  }
  arcada.display->println("Air Quality sensor found!");
}

void loop() {
  unsigned long currentMillis = millis();
  PM25_AQI_Data data;

  arcada.readButtons();
  uint8_t buttons = arcada.justPressedButtons();
  if (buttons & ARCADA_BUTTONMASK_SELECT){
    uint8_t selected = arcada.menu(selection, numMenuItems, ARCADA_WHITE, ARCADA_BLACK);
    char message[80];
    sprintf(message, "Selected '%s'", selection[selected]);
    arcada.display->fillScreen(ARCADA_BLUE);
    arcada.infoBox(message);
    shouldRedraw = true;
  }
  if (currentMillis - previousMillis >= interval) {
    if (! aqSensor.read(&data)) {
      //Serial.println("Could not read from AQI");
      arcada.pixels.setPixelColor(2, arcada.pixels.Color(255,0, 0));
      arcada.pixels.show();
      delay(500);  // try again in a bit!
      return;
    }
    //Serial.println("AQI reading success");
    int aqi = calculateAQI(data);
    // Don't redraw screen if the aqi hasn't changed
    if (aqi != previousAQI) {
      previousAQI = aqi;
      shouldRedraw = true;
    }
    previousMillis = currentMillis;
  } // interval check
  if (shouldRedraw == true) {
    //arcada.pixels.clear();
    arcada.display->fillScreen(ARCADA_BLUE);
    arcada.pixels.show();
    displayAQI(previousAQI);
    shouldRedraw = false;
  }
} // end loop

// Returns a calculated air quality index (AQI)
// https://learn.adafruit.com/air-quality-sensor-silo-house/code-usage
int calculateAQI(PM25_AQI_Data data) {
  int aqiVal;
  uint16_t pm25env = data.pm25_env;
  // Check sensor reading using EPA breakpoint (Clow-Chigh)
  if ((0.0 <= pm25env) && (pm25env <= 12.0)) {
    // Good
    aqiVal = map(pm25env, 0, 12, 0, 50);
    // https://www.arduino.cc/reference/en/language/functions/math/map/
  } else if ((12.1 <= pm25env) && (pm25env <= 35.4)) {
    // Moderate
    aqiVal = map(pm25env, 12, 35, 51, 100);
  } else if ((35.5 <= pm25env) && (pm25env <= 55.4)) {
    // Unhealthy for Sensitive Groups
    aqiVal = map(pm25env, 36, 55, 101, 150);
  } else if ((55.5 <= pm25env) && (pm25env <= 150.4)) {
    // Unhealthy
    aqiVal = map(pm25env, 56, 150, 151, 200);
  } else if ((150.5 <= pm25env) && (pm25env <= 250.4)) {
    // Very Unhealthy
    aqiVal = map(pm25env, 151, 250, 201, 300);
  } else if ((250.5 <= pm25env) && (pm25env <= 350.4)) {
    // Hazardous
    aqiVal = map(pm25env, 251, 350, 301, 400);
  } else if ((350.5 <= pm25env) && (pm25env <= 500.4)) {
    // Hazardous
    aqiVal = map(pm25env, 351, 500, 401, 500);
  }    
  return aqiVal;
}

// https://en.wikipedia.org/wiki/Air_quality_index#United_States
void displayAQI(int aqi) {
  if (aqi < 51) {
    drawReading(ARCADA_GREEN, ARCADA_BLACK, aqi, "Good");
  } else if (aqi < 101) {
    drawReading(ARCADA_YELLOW, ARCADA_BLACK, aqi, "Moderate");
  } else if (aqi < 151) {
    drawReading(ARCADA_ORANGE, ARCADA_WHITE, aqi, "Unhealthy for Sensitive Groups");
  } else if (aqi < 201) {
    drawReading(ARCADA_RED, ARCADA_WHITE, aqi, "Unhealthy");
  } else if (aqi < 301) {
    drawReading(ARCADA_PURPLE, ARCADA_WHITE, aqi, "Very Unhealthy");
  } else if (aqi < 501) {
    drawReading(ARCADA_MAROON, ARCADA_WHITE, aqi, "Hazardous");
  }
}

// Redraw 160x128 screen 
void drawReading(uint16_t catColor, uint16_t valColor, int aqi, String catDesc) {
  int startX = 80;
  arcada.display->fillScreen(ARCADA_BLUE); //28,113,187  //#1C71BB
  
  // x = (width-(3x6x4)+buffer)/2
  arcada.display->fillRoundRect(39,16,82,70,15, ARCADA_WHITE);
  arcada.display->fillRoundRect(42,19,76,64,12,catColor);

  arcada.display->setTextColor(valColor);
  arcada.display->setTextSize(4);
  if (aqi < 10) {
    startX = 70; // 80 - ((6x4)/2)
  } else if (aqi < 100) {
    startX = 58; // 80 - ((6x2x4)/2)
  } else {
    startX = 46; // 80 - ((6x3x4)/2)
  }
  arcada.display->setCursor(startX, 36);
  arcada.display->print(aqi);
  arcada.display->setTextSize(1);
  arcada.display->setCursor(72, 68);
  arcada.display->print("AQI");

  arcada.display->setTextColor(ARCADA_WHITE);
  //handle long descriptions
  if (catDesc.length() > 26) { //screen width / char width or 160/6
    arcada.display->setTextSize(2);
    unsigned int i = catDesc.indexOf(" ");
    String sub = catDesc.substring(0,i);
    startX = 80 - (sub.length() * 6);
    arcada.display->setCursor(startX, 90);
    arcada.display->print(sub);
    //second part of string
    arcada.display->setTextSize(1);
    sub = catDesc.substring(i+1);
    startX = 80 - (sub.length() * 3);
    arcada.display->setCursor(startX, 108);
    arcada.display->print(sub);
  } else { // short description
    arcada.display->setTextSize(2);
    startX = 80 - (catDesc.length() * 6);
    arcada.display->setCursor(startX, 90);
    arcada.display->print(catDesc);
  }
  //arcada.display->setTextSize(1);
  //arcada.display->setCursor(4, 118);
  //arcada.display->print("AQI reading success");
  //TODO: print("pm25env=")
  //TODO: print(pm25env)
}
