#include <Arduino.h>
#include "DHT.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>

enum CoolingModes
{
  Cooling = 1,
  Heating = 0
};
enum ZoneBarrelPosition
{
  Open = 1,
  Closed = 0,
};

const int DHTPIN = 2;
const uint8_t DHTTYPE = DHT11;
DHT dht(DHTPIN, DHTTYPE);
const int blue = 13;
const int white = 6;
const int TEMP_UP_BUTTON_PIN = 3;
const int TEMP_DOWN_BUTTON_PIN = 4;
const int MODE_BUTTON_PIN = 5;
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
int setTempAddress = 0;
int coolingModeAddress = setTempAddress += sizeof(float);

double setTemp = 22;

// Temperature Regulator
double highRange;
double lowRange;
double tempInc = 0.5;
CoolingModes coolingMode;
ZoneBarrelPosition barrelPosition = Open;

void setup()
{
  Serial.begin(9600);
  pinMode(blue, OUTPUT);
  pinMode(white, OUTPUT);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TEMP_UP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TEMP_DOWN_BUTTON_PIN, INPUT_PULLUP);
  dht.begin();
  lcd.begin(16, 2);
  EEPROM.get(coolingModeAddress, coolingMode);
  coolingMode == Cooling ? coolingMode = Heating : coolingMode = Cooling; // buttonPress bug on start up
  EEPROM.get(setTempAddress, setTemp);
  lowRange = setTemp - 1;
  highRange = setTemp + 1;
}

void printToLcd(double t, double setTemp)
{
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(t);
  lcd.print("c");
  lcd.setCursor(15, 0);
  coolingMode == Cooling ? lcd.print("C") : lcd.print("H");
  lcd.setCursor(0, 1);
  lcd.print("SetT: ");
  lcd.print(setTemp);
  lcd.print("c");
}

void changeSetTemp(double tempOffset)
{
  setTemp += tempOffset;
  highRange += tempOffset;
  lowRange += tempOffset;
  EEPROM.put(setTempAddress, setTemp);
}

ZoneBarrelPosition getBarrelPostition(double temp)
{
  if (!(temp < lowRange) && (temp < lowRange))
  {
    return barrelPosition;
  }
  if (coolingMode == Cooling)
  {
    if (temp > highRange)
      return Open;
    if (temp < lowRange)
      return Closed;
  }
  else
  {
    if (temp > highRange)
      return Closed;
    if (temp < lowRange)
      return Open;
  }
  return barrelPosition;
}

// Generic function to check if a button is pressed
int buttonPressed(uint8_t button)
{
  static uint16_t lastStates = 0;
  uint8_t state = digitalRead(button);
  if (state != ((lastStates >> button) & 1))
  {
    lastStates ^= 1 << button;
    return state == HIGH;
  }
  return false;
}

void loop()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (buttonPressed(MODE_BUTTON_PIN))
  {
    // Toggle Cooling mode
    coolingMode == Cooling ? coolingMode = Heating : coolingMode = Cooling;
    EEPROM.put(coolingModeAddress, coolingMode);
  };
  barrelPosition = getBarrelPostition(t);
  digitalWrite(white, barrelPosition);

  if (buttonPressed(TEMP_UP_BUTTON_PIN))
  {
    changeSetTemp(tempInc);
  };
  if (buttonPressed(TEMP_DOWN_BUTTON_PIN))
  {
    changeSetTemp(-tempInc);
  };

  digitalWrite(blue, coolingMode);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  printToLcd(t, setTemp);
}