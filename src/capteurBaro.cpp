#include <Wire.h>
#include <MS5837.h>
#include "capteurBaro.h"
#include "gestionnaireCapteur.h"

MS5837 bar30;
float lastDepth = 0;
float lastPressure = 0;
float nextTargetDepth = 0.5;
int sampleIndex = 0;

void updatePressureSensor()
{
    bar30.read();
    lastDepth = bar30.depth();
    lastPressure = bar30.pressure();
}

float readDepthBar30()     { return lastDepth; }
float readPressureBar30()  { return lastPressure; }


float increment(float d)
{
    if (d < 2.0) return 0.5;
    return 1.0;
}

void depthManagerInit()
{
    nextTargetDepth = 0.5;
    sampleIndex = 0;
}

void depthManagerUpdate(WaterDataStruct* payload)
{
    updatePressureSensor();
    float depth = readDepthBar30();

    if (depth >= nextTargetDepth && sampleIndex < 12)
    {
        runAllSensors(payload, sampleIndex);

        nextTargetDepth += increment(depth);
        sampleIndex++;
    }
}