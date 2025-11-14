#include "gestionnaireCapteur.h"
#include "capteurPH.h"
#include "capteurORP.h"
#include "capteurO2.h"
#include "capteurTemp.h"
#include "capteurBaro.h"

void runAllSensors(WaterDataStruct* payload, int index)
{
    payload[index].depth      = readDepthBar30();
    payload[index].temperature= readTemp();
    payload[index].pH         = readPH();
    payload[index].orp        = readORP();
    payload[index].o2         = readDO2();
    payload[index].pression   = readPressureBar30();
}
