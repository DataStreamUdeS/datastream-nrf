#include <Wire.h>
#include <MS5837.h>
#include "capteurBaro.h"
#include "gestionnaireCapteur.h"

MS5837 bar30;

#define PRESSION_ATM_MBAR 1013.0f
#define THRESOLD_PROFONDEUR 0.05f

static float nextTargetDepth = 0.5;
static int sampleIndex = 0;

void depthManagerInit()
{
    Wire.begin();

    bar30.init();
    bar30.setModel(MS5837::MS5837_30BA);
    bar30.setFluidDensity(997); 

    nextTargetDepth = 0.5f;
    sampleIndex = 0;
}

float increment(float d)
{
    if (d < 2.0f) return 0.5f;
    return 1.0f;
}

bool depthManagerUpdate(WaterDataStruct* payload)
{
    bar30.read();

    float profondeur = bar30.depth();
    float pressure = bar30.pressure();

    // === 1. Arrêt automatique si revenu à l'air ===
    if (profondeur < THRESOLD_PROFONDEUR && sampleIndex > 0) {
        return true;  // terminé
    }

    // === 2. Acquisition selon profondeur ===
    if (profondeur >= nextTargetDepth && sampleIndex < 12)
    {
        runAllSensors(payload, sampleIndex);

        nextTargetDepth += increment(profondeur);
        sampleIndex++;
    }

    return false; 
}

