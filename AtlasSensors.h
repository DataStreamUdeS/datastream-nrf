/* AtlasSensors.h
   Library for interfacing with Atlas Scientific EZO sensors via I2C
   Supports: pH, ORP, DO (I2C), Temperature (Analog)
*/

#ifndef ATLAS_SENSORS_H
#define ATLAS_SENSORS_H

#include <Arduino.h>
#include <Wire.h>

// Analog temperature sensor pin
#define TEMP_ANALOG_PIN 4

// Blue Robotics Bar30 barometer address
#define BAR30_ADDR          0x76  // MS5837 sensor

// Default I2C addresses for Atlas Scientific EZO circuits
#define ATLAS_PH_ADDR       0x63  // pH sensor
#define ATLAS_ORP_ADDR      0x62  // ORP sensor
#define ATLAS_DO_ADDR       0x61  // Dissolved Oxygen sensor

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
    float readTemperature();  // Analog sensor
    float readDepth();
    
    // Sensor control
    bool sendCommand(uint8_t address, const char* command);
    String readResponse(uint8_t address);
    
    // Calibration functions
    bool calibratePH(float pH_value);
    bool calibrateORP(float orp_value);
    bool calibrateDO(bool atmospheric = true);
    
    // Sleep/Wake functions
    bool sleepSensor(uint8_t address);
    bool wakeSensor(uint8_t address);
    
    // Status functions
    bool getStatus(uint8_t address);
    String getDeviceInfo(uint8_t address);
    
    // Temperature compensation
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
    bool _barometerAvailable;
    
    // MS5837 barometer variables
    uint16_t _baro_C[8];  // Calibration coefficients
    uint32_t _baro_D1, _baro_D2;  // Raw pressure and temperature
    int32_t _baro_TEMP;
    int32_t _baro_P;
    
    // Barometer helper functions
    bool initBarometer();
    void readBarometerCalibration();
    uint32_t readBarometerADC(uint8_t cmd);
    void calculateBarometer();
};

#endif // ATLAS_SENSORS_H