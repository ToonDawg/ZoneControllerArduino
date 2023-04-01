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
    Closed = 0
};
enum Buttons
{
    ModeButton = 5,
    TempUpButton = 3,
    TempDownButton = 4
};

const int DHTPIN = 2;
const uint8_t DHTTYPE = DHT11;
DHT dht(DHTPIN, DHTTYPE);
const int blue = 13;
const int white = 6;

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

int setTempAddress = 0;
int coolingModeAddress = setTempAddress + sizeof(float);

double setTemp = 22;
double highRange;
double lowRange;
const double tempInc = 0.5;

void setup()
{
    Serial.begin(9600);
    pinMode(blue, OUTPUT);
    pinMode(white, OUTPUT);
    pinMode(ModeButton, INPUT_PULLUP);
    pinMode(TempUpButton, INPUT_PULLUP);
    pinMode(TempDownButton, INPUT_PULLUP);
    dht.begin();
    lcd.begin(16, 2);
    EEPROM.get(coolingModeAddress, coolingMode);
    EEPROM.get(setTempAddress, setTemp);
    lowRange = setTemp - 1;
    highRange = setTemp + 1;
}

void printToLcd(double t, double setTemp, CoolingModes coolingMode)
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

void changeSetTemp(double tempOffset, int setTempAddress)
{
    setTemp += tempOffset;
    highRange += tempOffset;
    lowRange += tempOffset;
    EEPROM.put(setTempAddress, setTemp);
}

ZoneBarrelPosition getBarrelPosition(double temp, CoolingModes coolingMode)
{
    ZoneBarrelPosition newPosition = barrelPosition;

    if (temp < lowRange || temp > highRange)
    {
        if ((coolingMode == Cooling && temp > highRange) || (coolingMode == Heating && temp < lowRange))
        {
            newPosition = Open;
        }
        else
        {
            newPosition = Closed;
        }
    }

    return newPosition;
}


bool buttonPressed(uint8_t button, uint32_t &lastButtonPress)
{
    const uint32_t debounceTime = 200;
    uint32_t currentTime = millis();
    bool state = digitalRead(button);

    if (state == LOW && currentTime - lastButtonPress > debounceTime)
    {
        lastButtonPress = currentTime;
        return true;
    }
    return false;
}

   void loop()
{
    static uint32_t lastButtonPresses[3] = {0, 0, 0};
    static CoolingModes coolingMode = Cooling;
    static ZoneBarrelPosition barrelPosition = Open;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (buttonPressed(ModeButton, lastButtonPresses[0]))
    {
        // Toggle Cooling mode
        coolingMode = (coolingMode == Cooling) ? Heating : Cooling;
        EEPROM.put(coolingModeAddress, coolingMode);
    }
    barrelPosition = getBarrelPosition(t, coolingMode, barrelPosition);
    digitalWrite(white, barrelPosition);

    if (buttonPressed(TempUpButton, lastButtonPresses[1]))
    {
        changeSetTemp(tempInc, setTempAddress);
    }
    if (buttonPressed(TempDownButton, lastButtonPresses[2]))
    {
        changeSetTemp(-tempInc, setTempAddress);
    }

    digitalWrite(blue, coolingMode);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t))
    {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
    }
    printToLcd(t, setTemp, coolingMode);
}
