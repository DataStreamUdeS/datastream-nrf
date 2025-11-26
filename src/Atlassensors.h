/* AtlasSensors.h
   Library for interfacing with Atlas Scientific EZO sensors via I2C
   Supports: pH, ORP (Oxidation-Reduction Potential), DO (Dissolved Oxygen), Temperature
*/

#ifndef ATLAS_SENSORS_H
#define ATLAS_SENSORS_H

#include <Arduino.h>
#include <Wire.h>

// Default I2C addresses for Atlas Scientific EZO circuits
#define ATLAS_PH_ADDR       0x63  // pH sensor
#define ATLAS_ORP_ADDR      0x62  // ORP sensor
#define ATLAS_DO_ADDR       0x61  // Dissolved Oxygen sensor
#define ATLAS_RTD_ADDR      0x66  // Temperature sensor (RTD)
#define ATLAS_EC_ADDR       0x64  // Conductivity (if you're using this as 4th sensor)

// Response codes from Atlas Scientific sensors
#define ATLAS_SUCCESS       1     // Command successful
#define ATLAS_FAIL          2     // Command failed
#define ATLAS_PENDING       254   // Processing, not ready
#define ATLAS_NO_DATA       255   // No data to send

// Timing constants (in milliseconds)
#define ATLAS_DELAY_READ    900   // Time to wait for reading
#define ATLAS_DELAY_SHORT   300   // Short delay for other commands

class AtlasSensors {
public:
    // Constructor
    AtlasSensors(int sda = 21, int scl = 22);
    
    // Initialization
    bool begin();
    
    // Check if sensors are connected
    bool checkSensor(uint8_t address);
    void scanI2C();
    
    // Reading functions
    float readPH();
    float readORP();
    float readDO();
    float readTemperature();
    float readDepth(); // Placeholder - implement based on your depth sensor
    
    // Sensor control
    bool sendCommand(uint8_t address, const char* command);
    String readResponse(uint8_t address);
    
    // Calibration functions (to be implemented as needed)
    bool calibratePH(float pH_value);
    bool calibrateORP(float orp_value);
    bool calibrateDO(bool atmospheric = true);
    
    // Sleep/Wake functions for power saving
    bool sleepSensor(uint8_t address);
    bool wakeSensor(uint8_t address);
    
    // Status functions
    bool getStatus(uint8_t address);
    String getDeviceInfo(uint8_t address);
    
    // Temperature compensation (for pH and DO)
    bool setTemperatureCompensation(uint8_t address, float temp);
    
private:
    int _sda;
    int _scl;
    
    // Helper functions
    float readSensorValue(uint8_t address, const char* command = "R");
    bool waitForResponse(uint8_t address, uint16_t timeout = ATLAS_DELAY_READ);
    uint8_t getResponseCode(uint8_t address);
    
    // Sensor availability flags
    bool _phAvailable;
    bool _orpAvailable;
    bool _doAvailable;
    bool _tempAvailable;
};

#endif // ATLAS_SENSORS_H