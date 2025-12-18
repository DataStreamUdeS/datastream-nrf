/* AtlasSensors.cpp
   Implementation of Atlas Scientific sensor interface
   pH, ORP, DO via I2C | Temperature via Analog
*/

#include "AtlasSensors.h"

AtlasSensors::AtlasSensors(int sda, int scl) {
    _sda = sda;
    _scl = scl;
    _phAvailable = false;
    _orpAvailable = false;
    _doAvailable = false;
    _tempAvailable = false;
    _barometerAvailable = false;
}

bool AtlasSensors::begin() {
    Serial.println("[AtlasSensors] Initializing sensors...");
    
    // Scan for available sensors
    scanI2C();
    
    // Check each I2C sensor
    _phAvailable = checkSensor(ATLAS_PH_ADDR);
    _orpAvailable = checkSensor(ATLAS_ORP_ADDR);
    _doAvailable = checkSensor(ATLAS_DO_ADDR);
    _tempAvailable = true; // Analog sensor always available
    
    // Initialize barometer
    _barometerAvailable = initBarometer();
    
    Serial.print("[AtlasSensors] pH: ");
    Serial.println(_phAvailable ? "OK" : "NOT FOUND");
    Serial.print("[AtlasSensors] ORP: ");
    Serial.println(_orpAvailable ? "OK" : "NOT FOUND");
    Serial.print("[AtlasSensors] DO: ");
    Serial.println(_doAvailable ? "OK" : "NOT FOUND");
    Serial.println("[AtlasSensors] Temp: OK (analog)");
    Serial.print("[AtlasSensors] Barometer: ");
    Serial.println(_barometerAvailable ? "OK" : "NOT FOUND");
    
    // Setup analog pin
    analogReadResolution(12);
    
    return (_phAvailable || _orpAvailable || _doAvailable || _barometerAvailable);
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
    
    Wire.requestFrom((int)address, 20, 1);
    
    byte code = Wire.read();
    
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
    if (!sendCommand(address, command)) {
        Serial.println("[AtlasSensors] Failed to send command");
        return -999.0;
    }
    
    delay(ATLAS_DELAY_READ);
    
    String response = readResponse(address);
    
    if (response.length() > 0) {
        return response.toFloat();
    }
    
    return -999.0;
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
    
    return false;
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
        return 7.0 + random(-10, 10) / 10.0;
    }
    
    Serial.print("[AtlasSensors] Reading pH... ");
    float value = readSensorValue(ATLAS_PH_ADDR, "R");
    Serial.println(value);
    return value;
}

float AtlasSensors::readORP() {
    if (!_orpAvailable) {
        Serial.println("[AtlasSensors] ORP sensor not available - using dummy data");
        return 250.0 + random(-50, 50);
    }
    
    Serial.print("[AtlasSensors] Reading ORP... ");
    float value = readSensorValue(ATLAS_ORP_ADDR, "R");
    Serial.println(value);
    return value;
}

float AtlasSensors::readDO() {
    if (!_doAvailable) {
        Serial.println("[AtlasSensors] DO sensor not available - using dummy data");
        return 8.0 + random(-20, 20) / 10.0;
    }
    
    Serial.print("[AtlasSensors] Reading DO... ");
    float value = readSensorValue(ATLAS_DO_ADDR, "R");
    Serial.println(value);
    return value;
}

float AtlasSensors::readTemperature() {
    // Read analog temperature sensor (RTD Meter v3)
    Serial.print("[AtlasSensors] Reading Temperature (analog)... ");
    
    int raw = analogRead(TEMP_ANALOG_PIN);
    float voltage = (raw / 4095.0) * 3.3;
    
    // Calibrated formula: Ice=1.09V/0°C, Boiling=1.67V/100°C
    float temp = (voltage * 172.4) - 187.9;
    
    Serial.println(temp);
    return temp;
}

float AtlasSensors::readDepth() {
    if (!_barometerAvailable) {
        Serial.println("[AtlasSensors] Barometer not available - using dummy data");
        static float depth = 0.0;
        depth += 10.0;
        return depth;
    }
    
    Serial.print("[AtlasSensors] Reading Depth (barometer)... ");
    
    // Read barometer - pressure first
    _baro_D1 = readBarometerADC(0x48); // Pressure conversion (OSR=4096)
    delay(50);  // Allow time between readings
    
    _baro_D2 = readBarometerADC(0x58); // Temperature conversion (OSR=4096)
    delay(50);
    
    calculateBarometer();
    
    // Convert pressure to depth
    // P = pressure in Pa, atmospheric = 101325 Pa
    // Depth (m) = (P - P_atm) / (ρ * g) where ρ=1000 kg/m³, g=9.81 m/s²
    // _baro_P is in Pa/100 (mbar units), so convert properly
    float pressure_mbar = _baro_P / 100.0;
    float depth_m = (pressure_mbar - 1013.25) / 100.0;  // Rough approximation
    
    Serial.print(pressure_mbar);
    Serial.print(" mbar, ");
    Serial.print(depth_m);
    Serial.println(" m");
    
    return depth_m;
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

// MS5837 Barometer Functions
bool AtlasSensors::initBarometer() {
    // Check if sensor exists
    if (!checkSensor(BAR30_ADDR)) {
        return false;
    }
    
    // Reset sensor
    Wire.beginTransmission(BAR30_ADDR);
    Wire.write(0x1E); // Reset command
    Wire.endTransmission();
    delay(100);  // MS5837 needs 2.8ms, use 100ms to be safe
    
    // Read calibration data
    readBarometerCalibration();
    
    return true;
}

void AtlasSensors::readBarometerCalibration() {
    for (uint8_t i = 0; i < 7; i++) {  // MS5837 has 7 coefficients (C0-C6)
        Wire.beginTransmission(BAR30_ADDR);
        Wire.write(0xA0 + (i * 2));
        byte error = Wire.endTransmission(false);  // Don't release bus
        
        if (error != 0) {
            Serial.print("[Barometer] Error reading C");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(error);
            continue;
        }
        
        delay(10);  // Give sensor time
        
        Wire.requestFrom((uint8_t)BAR30_ADDR, (uint8_t)2, (uint8_t)true);
        if (Wire.available() >= 2) {
            _baro_C[i] = (Wire.read() << 8) | Wire.read();
            Serial.print("[Barometer] C");
            Serial.print(i);
            Serial.print(" = ");
            Serial.println(_baro_C[i]);
        } else {
            Serial.print("[Barometer] No data for C");
            Serial.println(i);
        }
        delay(10);
    }
}

uint32_t AtlasSensors::readBarometerADC(uint8_t cmd) {
    // Send conversion command
    Wire.beginTransmission(BAR30_ADDR);
    Wire.write(cmd);
    Wire.endTransmission();
    
    delay(20); // Wait for ADC conversion (typical 10ms, use 20ms)
    
    // Read ADC result
    Wire.beginTransmission(BAR30_ADDR);
    Wire.write(0x00); // ADC read command
    Wire.endTransmission();
    
    Wire.requestFrom((uint8_t)BAR30_ADDR, (uint8_t)3);
    if (Wire.available() < 3) {
        Serial.println("[Barometer] ADC read failed");
        return 0;
    }
    
    uint32_t result = 0;
    result = Wire.read();
    result = (result << 8) | Wire.read();
    result = (result << 8) | Wire.read();
    
    return result;
}

void AtlasSensors::calculateBarometer() {
    // Calculate temperature
    int32_t dT = _baro_D2 - ((uint32_t)_baro_C[5] << 8);
    _baro_TEMP = 2000 + ((int64_t)dT * _baro_C[6]) / 8388608LL;
    
    // Calculate pressure
    int64_t OFF = ((int64_t)_baro_C[2] << 16) + (((int64_t)_baro_C[4] * dT) / 128);
    int64_t SENS = ((int64_t)_baro_C[1] << 15) + (((int64_t)_baro_C[3] * dT) / 256);
    
    _baro_P = ((_baro_D1 * SENS / 2097152LL) - OFF) / 8192LL;
}