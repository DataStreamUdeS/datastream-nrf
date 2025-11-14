#include <Wire.h>
#include <Arduino.h>
#define PH_ADDR 0x63

float readPH()
{
    Wire.beginTransmission(PH_ADDR);
    Wire.write('r');      // commande READ
    Wire.endTransmission();
    delay(300);           // temps EZO-pH

    Wire.requestFrom(PH_ADDR, 20);
    String resp = "";
    while (Wire.available())
        resp += char(Wire.read());

    return resp.toFloat();
}
