#include <Wire.h>
#include <Arduino.h>
#define ORP_ADDR 0x62

float readORP()
{
    Wire.beginTransmission(ORP_ADDR);
    Wire.write('r');
    Wire.endTransmission();
    delay(300);

    Wire.requestFrom(ORP_ADDR, 20);
    String r = "";
    while (Wire.available())
        r += char(Wire.read());

    return r.toFloat();  // mV
}
