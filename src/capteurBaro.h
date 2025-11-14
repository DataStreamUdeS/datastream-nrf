#pragma once
#include "waterDataPayload.h"
float readDepthBar30();
float readPressureBar30();
void updatePressureSensor();
void depthManagerInit();
void depthManagerUpdate(WaterDataStruct* payload);
extern float nextTargetDepth;

