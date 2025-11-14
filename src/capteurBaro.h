#pragma once
#include "waterDataPayload.h"
float readDepthBar30();
float readPressureBar30();
void updatePressureSensor();
void depthManagerInit();
bool depthManagerUpdate(WaterDataStruct* payload);
extern float nextTargetDepth;

