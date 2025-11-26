/* water_ble_sequence_esp32.cpp
   Adapted for ESP32 WROVER-E from NRF52840 Bluefruit version
   Implements BLE peripheral with handshake protocol and Atlas Scientific sensor support
*/

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include "waterDataPayload.h"
#include "AtlasSensors.h"

// I2C Configuration
#define I2C_SDA 21
#define I2C_SCL 22
#define I2C_FREQ 100000

// Forward declarations
void MEASURE_COLLECTION_SENSOR_CONTROL();

// Global data storage
WaterDataStruct waterDataPayload[12];

// BLE UUIDs - Using full 128-bit UUIDs based on your 16-bit ones
// #define SERVICE_UUID        "0000180A-0000-1000-8000-00805f9b34fb"
// #define DATA_CHAR_UUID      "0000181A-0000-1000-8000-00805f9b34fb"
// #define CONTROL_CHAR_UUID   "0000182A-0000-1000-8000-00805f9b34fb"
// #define CONFIG_CHAR_UUID    "0000183A-0000-1000-8000-00805f9b34fb"
// #define STATUS_CHAR_UUID    "0000184A-0000-1000-8000-00805f9b34fb"

#define SERVICE_UUID        "180A"
#define DATA_CHAR_UUID      "181A"
#define CONTROL_CHAR_UUID   "182A"
#define CONFIG_CHAR_UUID    "183A"
#define STATUS_CHAR_UUID    "184A"

// BLE Objects
BLEServer* pServer = nullptr;
BLEService* pService = nullptr;
BLECharacteristic* waterDataChar = nullptr;
BLECharacteristic* waterControlChar = nullptr;
BLECharacteristic* waterConfigChar = nullptr;
BLECharacteristic* waterStatusChar = nullptr;

// State machine
enum DeviceState { IDLE, INIT_RECEIVED, READY, MOVING, TX_READY, TX};
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
      
      
      // Restart advertising after disconnect
      // delay(500); // Small delay before restarting advertising
      // BLEDevice::startAdvertising();
      // logSerial("BLE", "Advertising restarted");
    }
};

// Control characteristic callback
class ControlCharCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      
      if (value.length() > 0) {
        String cmd = String(value.c_str());
        String logmsg = "Control WRITE received: " + cmd;
        logSerial("BLE", logmsg);

        if (cmd == "Init") {
          devState = INIT_RECEIVED;
          // Notify status "Ready"
          String statusCmd = "Ready";
          waterStatusChar->setValue(statusCmd.c_str());
          waterStatusChar->notify();
          devState = READY;
          logSerial("BLE", "Sent status 'Ready' (notify)");
        }
        else if ((cmd == "start_retriving") && (devState == READY)) {
          devState = MOVING;
          // Update and notify status as well
          String statusCmd = "Moving";
          waterStatusChar->setValue(statusCmd.c_str());
          waterStatusChar->notify();
          logSerial("BLE", "Control 'Moving' received - status set & notified to 'Moving'");
          // TODO ÉMILE : code des capteurs
          MEASURE_COLLECTION_SENSOR_CONTROL();
          devState = TX_READY;
          // Notify status "Ready_to_tx"
          statusCmd = "ready_to_tx";
          waterStatusChar->setValue(statusCmd.c_str());
          waterStatusChar->notify();
          logSerial("BLE", "Sensor measurements done - sent status 'ready_to_tx' (notify)");
          // At this point the Pi is still connected; the Pi intends to disconnect
        }
        else if ((cmd == "ready_to_tx") && (devState == TX_READY)) {
          devState = TX;
          // Notify status "Idle"
          String statusCmd = "tx";
          waterStatusChar->setValue(statusCmd.c_str());
          waterStatusChar->notify();
          waterDataChar->setValue((uint8_t*)waterDataPayload, sizeof(WaterDataStruct) * 12);
          waterDataChar->notify();
          logSerial("BLE", "Sent status 'tx' and notified data");
        }
        else if (cmd == "done_tx") {
          devState = IDLE;
          // Notify status "Idle"
          String statusCmd = "Idle";
          waterStatusChar->setValue(statusCmd.c_str());
          waterStatusChar->notify();
          logSerial("BLE", "Sent status 'Idle' (notify)");
        }
        else if (cmd.startsWith("Calibration")) { //might fuck up, pas sur que la fonction est reconnu
          logSerial("BLE", "Calibration command received - not implemented yet");
          // TODO : Implement calibration handling
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
        // Device could react if RPi writes status; for now we only log it.
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
    waterDataPayload[i].depth = sensors.readDepth(); // You'll need to implement depth sensor
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
    
    // Delay between measurements (adjust as needed)
    if (i < 11) delay(5000); // 5 seconds between readings
  }
  
  logSerial("SENSOR", "MEASURE_COLLECTION_SENSOR_CONTROL() completed");
  
  // Update BLE characteristic with collected data
  // Note: You might want to send this data back when RPi reconnects
  if (waterDataChar != nullptr) {
    waterDataChar->setValue((uint8_t*)waterDataPayload, sizeof(WaterDataStruct) * 12);
    logSerial("SENSOR", "Data updated in BLE characteristic");
  }
}

// Initialize BLE
void initBLE() {
  logSerial("BLE", "Initializing BLE");
  
  // Create BLE Device
  BLEDevice::init("DataStream Capsule");
  
  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Create BLE Service
  pService = pServer->createService(SERVICE_UUID);
  
  // Create Water Data Characteristic (Read, Notify)
  waterDataChar = pService->createCharacteristic(
                      DATA_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
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
  waterControlChar->setCallbacks(new ControlCharCallbacks());
  waterControlChar->setValue("Idle");
  
  // Create Config Characteristic (Read, Write)
  waterConfigChar = pService->createCharacteristic(
                      CONFIG_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );
  waterConfigChar->setValue("Default");
  
  // Create Status Characteristic (Read, Write, Notify)
  waterStatusChar = pService->createCharacteristic(
                      STATUS_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  waterStatusChar->addDescriptor(new BLE2902());
  waterStatusChar->setCallbacks(new StatusCharCallbacks());
  waterStatusChar->setValue("Sleeping");
  
  // Start the service
  pService->start();
  
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  logSerial("BLE", "BLE initialized and advertising started");
}

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize
  
  logSerial("SYS", "ESP32 Water Quality Monitor Starting...");
  
  // Initialize LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  // Initialize I2C for Atlas Scientific sensors
  logSerial("I2C", "Initializing I2C bus");
  Wire.begin(I2C_SDA, I2C_SCL, I2C_FREQ);
  
  // Initialize Atlas Scientific sensors
  logSerial("SENSOR", "Initializing Atlas Scientific sensors");
  if (sensors.begin()) {
    logSerial("SENSOR", "Sensors initialized successfully");
  } else {
    logSerial("SENSOR", "WARNING: Some sensors may not be responding");
  }
  
  // Initialize BLE
  initBLE();
  
  logSerial("SYS", "Setup complete - waiting for connection");
}

void loop() {
  // Handle connection state changes
  if (deviceConnected && !oldDeviceConnected) {
    // Connected
    oldDeviceConnected = deviceConnected;
  }
  
  if (!deviceConnected && oldDeviceConnected) {
    // Disconnected
    oldDeviceConnected = deviceConnected;
  }
  
  // Periodic heartbeat
  static unsigned long last = 0;
  if (millis() - last > 10000) {
    last = millis();
    logSerial("SYS", "Heartbeat - state: " + String(devState) + 
              ", connected: " + String(deviceConnected));
  }
  
  delay(100); // Small delay to prevent watchdog issues
}