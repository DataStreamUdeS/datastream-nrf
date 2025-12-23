/* water_ble_sequence_esp32.cpp
   Adapted for ESP32-S3 WROOM-1
   Implements BLE peripheral with handshake protocol and Atlas Scientific sensor support
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include "waterDataPayload.h"
#include "AtlasSensors.h"

// I2C Configuration
#define I2C_SDA 19
#define I2C_SCL 20
#define I2C_FREQ 100000

// Forward declarations
void MEASURE_COLLECTION_SENSOR_CONTROL();

// Global data storage
WaterDataStruct waterDataPayload[12];

// BLE UUIDs
#define SERVICE_UUID "7e3a0001-2f4a-4c1e-9b0f-8c1e12345678"
#define CONTROL_CHAR_UUID "7e3a0002-2f4a-4c1e-9b0f-8c1e12345678"
#define STATUS_CHAR_UUID "7e3a0003-2f4a-4c1e-9b0f-8c1e12345678"
#define DATA_CHAR_UUID "7e3a0004-2f4a-4c1e-9b0f-8c1e12345678"
#define CONFIG_CHAR_UUID "7e3a0005-2f4a-4c1e-9b0f-8c1e12345678"
 

// BLE Objects
BLEServer* pServer = nullptr;
BLEService* pService = nullptr;
BLECharacteristic* waterDataChar = nullptr;
BLECharacteristic* waterControlChar = nullptr;
BLECharacteristic* waterConfigChar = nullptr;
BLECharacteristic* waterStatusChar = nullptr;

// State machine - FIXED: Added TRANSMITTING state (TX conflicts with Arduino pin)
enum DeviceState { IDLE, INIT_RECEIVED, READY, MOVING, TX_READY, TRANSMITTING };
DeviceState devState = IDLE;

// Connection status
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Atlas Sensors
AtlasSensors sensors(I2C_SDA, I2C_SCL);

// Logging function
void logSerial(const char* tag, const String &msg) {
  Serial.print("[");
  Serial.print(tag);
  Serial.print("] ");
  Serial.println(msg);
}

// Server callbacks for connection management
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      logSerial("BLE", "Central connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      logSerial("BLE", "Central disconnected - calling MEASURE_COLLECTION_SENSOR_CONTROL()");
      
      // Start sensor collection because RPi is underwater / gone
      MEASURE_COLLECTION_SENSOR_CONTROL();
      
      // Restart advertising after disconnect
      delay(500);
      BLEDevice::startAdvertising();
      logSerial("BLE", "Advertising restarted");
    }
};

class ControlCharCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      
      if (value.length() > 0) {
        String cmd = String(value.c_str());
        String logmsg = "Control WRITE received: " + cmd;
        logSerial("BLE", logmsg);

        if (cmd == "Init") {
          devState = INIT_RECEIVED;
          String statusCmd = "Ready";
          waterStatusChar->setValue(statusCmd.c_str());
          waterStatusChar->notify();
          devState = READY;
          logSerial("BLE", "Sent status 'Ready' (notify)");
        }
        else if ((cmd == "start_retriving") && (devState == READY)) {
          devState = MOVING;
          String statusCmd = "Moving";
          waterStatusChar->setValue(statusCmd.c_str());
          waterStatusChar->notify();
          logSerial("BLE", "Control 'Moving' received - status set & notified to 'Moving'");
          
          MEASURE_COLLECTION_SENSOR_CONTROL();
          
          devState = TX_READY;
          statusCmd = "ready_to_tx";
          waterStatusChar->setValue(statusCmd.c_str());
          waterStatusChar->notify();
          logSerial("BLE", "Sensor measurements done - sent status 'ready_to_tx' (notify)");
        }
        else if ((cmd == "ready_to_tx") && (devState == TX_READY)) {
          devState = TRANSMITTING;
          String statusCmd = "tx";
          waterStatusChar->setValue(statusCmd.c_str());
          waterStatusChar->notify();

          for (int i = 0; i < 12; i++) {
            waterDataChar->setValue((uint8_t*)&waterDataPayload[i], sizeof(waterDataPayload[i]));
            waterDataChar->notify();
            delay(50);
          }
          // waterDataChar->setValue((uint8_t*)waterDataPayload, sizeof(WaterDataStruct) * 12);
          // waterDataChar->notify();
          logSerial("BLE", "Sent status 'tx' and notified data");
        }
        else if (cmd == "done_tx") {
          devState = IDLE;
          String statusCmd = "Idle";
          waterStatusChar->setValue(statusCmd.c_str());
          waterStatusChar->notify();
          logSerial("BLE", "Sent status 'Idle' (notify)");
        }
        else if (cmd.startsWith("Calibration")) {
          logSerial("BLE", "Calibration command received - not implemented yet");
          // TODO: Implement calibration handling
        }
        else {
          logSerial("BLE", ("Unknown control: " + cmd));
        }
      }
    }
};

// Status characteristic callback
class StatusCharCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      
      if (value.length() > 0) {
        String val = String(value.c_str());
        logSerial("BLE", ("Status WRITE received: " + val));
      }
    }
};

// Sensor measurement and data collection
void MEASURE_COLLECTION_SENSOR_CONTROL() {
  logSerial("SENSOR", "MEASURE_COLLECTION_SENSOR_CONTROL() started");
  
  // Visual indicator - blink LED
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
  }
  
  // Take 12 measurements at different depths/times
  for (int i = 0; i < 12; i++) {
    logSerial("SENSOR", "Taking measurement " + String(i + 1) + "/12");
    
    // Read from Atlas Scientific sensors
    waterDataPayload[i].depth = sensors.readDepth();
    waterDataPayload[i].temperature = sensors.readTemperature();
    waterDataPayload[i].pH = sensors.readPH();
    waterDataPayload[i].orp = sensors.readORP();
    waterDataPayload[i].o2 = sensors.readDO();
    
    // Log the reading
    logSerial("SENSOR", "Depth: " + String(waterDataPayload[i].depth) + 
              " Temp: " + String(waterDataPayload[i].temperature) +
              " pH: " + String(waterDataPayload[i].pH) +
              " ORP: " + String(waterDataPayload[i].orp) +
              " O2: " + String(waterDataPayload[i].o2));
    
    // Delay between measurements
    if (i < 11) delay(200); // 5 seconds between readings
  }
  
  logSerial("SENSOR", "MEASURE_COLLECTION_SENSOR_CONTROL() completed");
  
  // Update BLE characteristic with collected data
  if (waterDataChar != nullptr) {
    waterDataChar->setValue((uint8_t*)waterDataPayload, sizeof(WaterDataStruct) * 12);
    logSerial("SENSOR", "Data updated in BLE characteristic");
  }
}

// Initialize BLE
void initBLE() {
  logSerial("BLE", "Initializing BLE");
  
  BLEDevice::init("DataStream Capsule");
  
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  pService = pServer->createService(SERVICE_UUID);
  
  // Create Water Data Characteristic (Read, Notify)
  waterDataChar = pService->createCharacteristic(
                      DATA_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  
waterDataChar->setAccessPermissions(ESP_GATT_PERM_READ);
  waterDataChar->addDescriptor(new BLE2902());
  
  // Initialize with dummy data
  fillPayloadWithDummyData(waterDataPayload);
  waterDataChar->setValue((uint8_t*)waterDataPayload, sizeof(WaterDataStruct) * 12);
  
  // Create Control Characteristic (Write)
  waterControlChar = pService->createCharacteristic(
                      CONTROL_CHAR_UUID,
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_WRITE_NR
                    );
  
  waterControlChar->setAccessPermissions(ESP_GATT_PERM_WRITE);
  waterControlChar->setCallbacks(new ControlCharCallbacks());
  waterControlChar->setValue("Idle");
  
  // Create Config Characteristic (Read, Write)
  waterConfigChar = pService->createCharacteristic(
                      CONFIG_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );
  waterConfigChar->setAccessPermissions(
    ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE
  );
  waterConfigChar->setValue("Default");
  
  // Create Status Characteristic (Read, Write, Notify)
  waterStatusChar = pService->createCharacteristic(
                      STATUS_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  waterStatusChar->setAccessPermissions(ESP_GATT_PERM_READ);                
  waterStatusChar->addDescriptor(new BLE2902());
  waterStatusChar->setCallbacks(new StatusCharCallbacks());
  waterStatusChar->setValue("Sleeping");
  
  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  logSerial("BLE", "BLE initialized and advertising started");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  logSerial("SYS", "ESP32 Water Quality Monitor Starting...");
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  logSerial("I2C", "Initializing I2C bus");
  Wire.begin(I2C_SDA, I2C_SCL, I2C_FREQ);
  
  logSerial("SENSOR", "Initializing Atlas Scientific sensors");
  if (sensors.begin()) {
    logSerial("SENSOR", "Sensors initialized successfully");
  } else {
    logSerial("SENSOR", "WARNING: Some sensors may not be responding");
  }
  
  initBLE();
  
  logSerial("SYS", "Setup complete - waiting for connection");
}

void loop() {
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  if (!deviceConnected && oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  static unsigned long last = 0;
  if (millis() - last > 10000) {
    last = millis();
    logSerial("SYS", "Heartbeat - state: " + String(devState) + 
              ", connected: " + String(deviceConnected));
  }
  
  delay(100);
}
