#include <Wire.h>
#include <Arduino.h>
#define DO_ADDR 0x61

float readDO2()
{
    Wire.beginTransmission(DO_ADDR);
    Wire.write('r');
    Wire.endTransmission();
    delay(600); // DO prend plus de temps

    Wire.requestFrom(DO_ADDR, 20);
    String r = "";
    while (Wire.available())
        r += char(Wire.read());

    return r.toFloat();  // mg/L
}
