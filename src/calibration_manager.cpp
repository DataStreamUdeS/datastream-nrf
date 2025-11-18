#include "calibration_manager.h"
#include <Wire.h>
#include "capteurPH.h"
#include "capteurORP.h"
#include "capteurO2.h"
#include "capteurTemp.h"

void sendEZOCommand(uint8_t addr, const char* cmd)
{
    Wire.beginTransmission(addr);
    Wire.write(cmd);
    Wire.endTransmission();
    delay(600);
}

// commande du type "Calibrate-pH-mid"
void startCalibration(String command)
{
    if (command == "Calibrate-pH-mid") {
        sendEZOCommand(0x63, "Cal,mid,7.0"); 
    }
    else if (command == "Calibrate-pH-low") {
        sendEZOCommand(0x63, "Cal,low,4.0");
    }
    else if (command == "Calibrate-pH-high") {
        sendEZOCommand(0x63, "Cal,high,10.0");
    }
    else if (command == "Calibrate-pH-clear") {
        sendEZOCommand(0x63, "Cal,clear");
    }

    else if (command == "Calibrate-ORP") {
        sendEZOCommand(0x62, "Cal,0");
    }

    else if (command == "Calibrate-ORP-clear") {
        sendEZOCommand(0x62, "Cal,clear");
    }

    else if (command == "Calibrate-DO-air") {
        sendEZOCommand(0x61, "Cal");
    }

    else if (command == "Calibrate-DO-zero") {
        sendEZOCommand(0x61, "Cal,0");
    }

    else if (command == "Calibrate-DO-clear") {
        sendEZOCommand(0x61, "Cal,clear");
    }

    else if (command.startsWith("Calibrate-Temp-")) {
        float t = command.substring(16).toFloat();  // ex: "Calibrate-Temp-25.0"
        String msg = "Cal," + String(t);
        sendEZOCommand(0x66, msg.c_str());
    }

    else if (command == "Calibrate-Temp-clear") {
        sendEZOCommand(0x66, "Cal,clear");
    }
}
