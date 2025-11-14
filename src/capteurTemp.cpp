#include <Wire.h>
#include <Arduino.h>
#define RTD_ADDR 0x66

float readTemp()
{
    Wire.beginTransmission(RTD_ADDR);
    Wire.write('r');
    Wire.endTransmission();
    delay(300);

    Wire.requestFrom(RTD_ADDR, 20);
    String r = "";
    while (Wire.available())
        r += char(Wire.read());

    return r.toFloat();   // °C
}
