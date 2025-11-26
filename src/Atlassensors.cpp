/* AtlasSensors.cpp
   Implementation of Atlas Scientific EZO sensor interface
*/

#include "AtlasSensors.h"

AtlasSensors::AtlasSensors(int sda, int scl) {
    _sda = sda;
    _scl = scl;
    _phAvailable = false;
    _orpAvailable = false;
    _doAvailable = false;
    _tempAvailable = false;
}

bool AtlasSensors::begin() {
    Serial.println("[AtlasSensors] Initializing sensors...");
    
    // Scan for available sensors
    scanI2C();
    
    // Check each sensor
    _phAvailable = checkSensor(ATLAS_PH_ADDR);
    _orpAvailable = checkSensor(ATLAS_ORP_ADDR);
    _doAvailable = checkSensor(ATLAS_DO_ADDR);
    _tempAvailable = checkSensor(ATLAS_RTD_ADDR);
    
    Serial.print("[AtlasSensors] pH: ");
    Serial.println(_phAvailable ? "OK" : "NOT FOUND");
    Serial.print("[AtlasSensors] ORP: ");
    Serial.println(_orpAvailable ? "OK" : "NOT FOUND");
    Serial.print("[AtlasSensors] DO: ");
    Serial.println(_doAvailable ? "OK" : "NOT FOUND");
    Serial.print("[AtlasSensors] Temp: ");
    Serial.println(_tempAvailable ? "OK" : "NOT FOUND");
    
    // Return true if at least one sensor is available
    return (_phAvailable || _orpAvailable || _doAvailable || _tempAvailable);
}

bool AtlasSensors::checkSensor(uint8_t address) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    return (error == 0);
}

void AtlasSensors::scanI2C() {
    Serial.println("[I2C] Scanning for devices...");
    byte count = 0;
    
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.print("[I2C] Device found at 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
            count++;
        }
    }
    
    if (count == 0) {
        Serial.println("[I2C] No I2C devices found");
    } else {
        Serial.print("[I2C] Found ");
        Serial.print(count);
        Serial.println(" device(s)");
    }
}

bool AtlasSensors::sendCommand(uint8_t address, const char* command) {
    Wire.beginTransmission(address);
    Wire.write(command);
    byte error = Wire.endTransmission();
    return (error == 0);
}

String AtlasSensors::readResponse(uint8_t address) {
    String response = "";
    
    Wire.requestFrom((int)address, 20, 1); // Request up to 20 bytes
    
    byte code = Wire.read(); // First byte is response code
    
    if (code == ATLAS_SUCCESS) {
        while (Wire.available()) {
            char c = Wire.read();
            if (c != '\0') {
                response += c;
            }
        }
    }
    
    return response;
}

float AtlasSensors::readSensorValue(uint8_t address, const char* command) {
    // Send read command
    if (!sendCommand(address, command)) {
        Serial.println("[AtlasSensors] Failed to send command");
        return -999.0; // Error value
    }
    
    // Wait for sensor to process
    delay(ATLAS_DELAY_READ);
    
    // Read response
    String response = readResponse(address);
    
    if (response.length() > 0) {
        return response.toFloat();
    }
    
    return -999.0; // Error value
}

bool AtlasSensors::waitForResponse(uint8_t address, uint16_t timeout) {
    unsigned long startTime = millis();
    
    while (millis() - startTime < timeout) {
        byte code = getResponseCode(address);
        if (code == ATLAS_SUCCESS) {
            return true;
        } else if (code == ATLAS_FAIL) {
            return false;
        }
        delay(100);
    }
    
    return false; // Timeout
}

uint8_t AtlasSensors::getResponseCode(uint8_t address) {
    Wire.requestFrom((int)address, 1, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return ATLAS_NO_DATA;
}

float AtlasSensors::readPH() {
    if (!_phAvailable) {
        Serial.println("[AtlasSensors] pH sensor not available - using dummy data");
        return 7.0 + random(-10, 10) / 10.0; // Dummy data 6.0-8.0
    }
    
    Serial.print("[AtlasSensors] Reading pH... ");
    float value = readSensorValue(ATLAS_PH_ADDR, "R");
    Serial.println(value);
    return value;
}

float AtlasSensors::readORP() {
    if (!_orpAvailable) {
        Serial.println("[AtlasSensors] ORP sensor not available - using dummy data");
        return 250.0 + random(-50, 50); // Dummy data 200-300 mV
    }
    
    Serial.print("[AtlasSensors] Reading ORP... ");
    float value = readSensorValue(ATLAS_ORP_ADDR, "R");
    Serial.println(value);
    return value;
}

float AtlasSensors::readDO() {
    if (!_doAvailable) {
        Serial.println("[AtlasSensors] DO sensor not available - using dummy data");
        return 8.0 + random(-20, 20) / 10.0; // Dummy data 6.0-10.0 mg/L
    }
    
    Serial.print("[AtlasSensors] Reading DO... ");
    float value = readSensorValue(ATLAS_DO_ADDR, "R");
    Serial.println(value);
    return value;
}

float AtlasSensors::readTemperature() {
    if (!_tempAvailable) {
        Serial.println("[AtlasSensors] Temp sensor not available - using dummy data");
        return 20.0 + random(-5, 15); // Dummy data 15-35°C
    }
    
    Serial.print("[AtlasSensors] Reading Temperature... ");
    float value = readSensorValue(ATLAS_RTD_ADDR, "R");
    Serial.println(value);
    return value;
}

float AtlasSensors::readDepth() {
    // TODO: Implement based on your depth sensor type
    // Options:
    // 1. Pressure sensor (convert pressure to depth)
    // 2. Ultrasonic sensor (if surface distance known)
    // 3. Time-based estimate (if descent rate is constant)
    
    Serial.println("[AtlasSensors] Depth sensor not implemented - using dummy data");
    static float depth = 0.0;
    depth += 10.0; // Simulate descending 10 units per reading
    return depth;
}

bool AtlasSensors::calibratePH(float pH_value) {
    if (!_phAvailable) return false;
    
    char cmd[20];
    snprintf(cmd, sizeof(cmd), "Cal,%.2f", pH_value);
    
    if (sendCommand(ATLAS_PH_ADDR, cmd)) {
        delay(ATLAS_DELAY_SHORT);
        return true;
    }
    return false;
}

bool AtlasSensors::calibrateORP(float orp_value) {
    if (!_orpAvailable) return false;
    
    char cmd[20];
    snprintf(cmd, sizeof(cmd), "Cal,%.0f", orp_value);
    
    if (sendCommand(ATLAS_ORP_ADDR, cmd)) {
        delay(ATLAS_DELAY_SHORT);
        return true;
    }
    return false;
}

bool AtlasSensors::calibrateDO(bool atmospheric) {
    if (!_doAvailable) return false;
    
    const char* cmd = atmospheric ? "Cal" : "Cal,0";
    
    if (sendCommand(ATLAS_DO_ADDR, cmd)) {
        delay(ATLAS_DELAY_READ);
        return true;
    }
    return false;
}

bool AtlasSensors::sleepSensor(uint8_t address) {
    return sendCommand(address, "Sleep");
}

bool AtlasSensors::wakeSensor(uint8_t address) {
    Wire.beginTransmission(address);
    Wire.write("");
    return (Wire.endTransmission() == 0);
}

bool AtlasSensors::getStatus(uint8_t address) {
    if (sendCommand(address, "Status")) {
        delay(ATLAS_DELAY_SHORT);
        String response = readResponse(address);
        Serial.print("[AtlasSensors] Status: ");
        Serial.println(response);
        return true;
    }
    return false;
}

String AtlasSensors::getDeviceInfo(uint8_t address) {
    if (sendCommand(address, "i")) {
        delay(ATLAS_DELAY_SHORT);
        return readResponse(address);
    }
    return "";
}

bool AtlasSensors::setTemperatureCompensation(uint8_t address, float temp) {
    char cmd[20];
    snprintf(cmd, sizeof(cmd), "T,%.2f", temp);
    
    if (sendCommand(address, cmd)) {
        delay(ATLAS_DELAY_SHORT);
        return true;
    }
    return false;
}