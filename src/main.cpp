/* water_ble_sequence.ino
   Based on your original example, adapted for the handshake & disconnection behavior.
*/

#include <Arduino.h>
#include <bluefruit.h>
#include "waterDataPayload.h"   // keep your struct / fill function here

// Forward-declare control functions
void MEASURE_COLLECTION_SENSOR_CONTROL();

WaterDataStruct waterDataPayload[12];

// Using the 16-bit UUIDs you already used (they map to full 128-bit UUIDs automatically)
BLEService waterDataService = BLEService(0x180A); 
BLECharacteristic waterDataChar = BLECharacteristic(0x181A); 
BLECharacteristic waterControlChar = BLECharacteristic(0x182A);
BLECharacteristic waterConfigChar = BLECharacteristic(0x183A);
BLECharacteristic waterStatusChar = BLECharacteristic(0x184A);

// Keep simple state machine
enum DeviceState { IDLE, INIT_RECEIVED, READY, MOVING };
DeviceState devState = IDLE;

void logSerial(const char* tag, const String &msg) {
  // Serial.print("[");
  // Serial.print(tag);
  // Serial.print("] ");
  // Serial.println(msg);
}

void setBLECharacteristicDefaults() {
  waterDataChar.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  waterDataChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);

  waterControlChar.setProperties(CHR_PROPS_WRITE_WO_RESP | CHR_PROPS_WRITE);
  waterControlChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);

  waterConfigChar.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE);
  waterConfigChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);

  // Make status readable, writable and notifiable so RPi can write status and device can notify
  waterStatusChar.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_NOTIFY);
  waterStatusChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);
}

void onControlWritten(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  // data is raw bytes — interpret as C-string
  String cmd = String((char*)data).substring(0, len);
  String logmsg = "Control WRITE received: " + cmd;
  logSerial("BLE", logmsg);

  if (cmd == "Init") {
    devState = INIT_RECEIVED;
    // Notify status "Ready"
    String statusCmd = "Ready";
    // Update status characteristic value and notify
    waterStatusChar.write((uint8_t*)statusCmd.c_str(), statusCmd.length());
    waterStatusChar.notify((uint8_t*)statusCmd.c_str(), statusCmd.length());
    devState = READY;
    logSerial("BLE", "Sent status 'Ready' (notify)");
  }
  else if (cmd == "Moving") {
    devState = MOVING;
    // Update and notify status as well
    String statusCmd = "Moving";
    waterStatusChar.write((uint8_t*)statusCmd.c_str(), statusCmd.length());
    waterStatusChar.notify((uint8_t*)statusCmd.c_str(), statusCmd.length());
    logSerial("BLE", "Control 'Moving' received — status set & notified to 'Moving'");
    // At this point the Pi is still connected; the Pi intends to disconnect (simulate going underwater)
  }
  // else if (cmd == "")
  else {
    logSerial("BLE", ("Unknown control: " + cmd));
  }
}

void onStatusWritten(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  String val = String((char*)data).substring(0,len);
  logSerial("BLE", ("Status WRITE received: " + val));
  // Device could react if RPi writes status; for now we only log it.
}

void conn_cb(uint16_t conn_hdl) {
  (void) conn_hdl;
  logSerial("BLE", "Central connected");
}

void disconn_cb(uint16_t conn_hdl, uint8_t reason) {
  (void) conn_hdl;
  (void) reason;
  logSerial("BLE", "Central disconnected - calling MEASURE_COLLECTION_SENSOR_CONTROL()");
  // Start sensor collection because RPi is underwater / gone
  MEASURE_COLLECTION_SENSOR_CONTROL();
}

// Example function called when Pi disconnects (you will implement actual sensors)
void MEASURE_COLLECTION_SENSOR_CONTROL() {
  logSerial("SENSOR", "MEASURE_COLLECTION_SENSOR_CONTROL() started");
  // Example: toggle onboard LED to show activity (if available)
  for (int i=0;i<3;i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
  }
  // Log end
  logSerial("SENSOR", "MEASURE_COLLECTION_SENSOR_CONTROL() completed");
  fillPayloadWithDummyData(waterDataPayload);
  logSerial("SENSOR", "Dummy data filled into payload");
  
}

void beginBLECharacteristics() {
  waterDataService.begin();
  waterDataService.setPermission(SECMODE_OPEN, SECMODE_OPEN);

  fillPayloadWithDummyData(waterDataPayload);

  // Data characteristic (we keep it but not used in handshake)
  waterDataChar.setMaxLen(sizeof(WaterDataStruct));
  waterDataChar.begin();
  // set initial value — not critical
  waterDataChar.write((uint8_t*)waterDataPayload, sizeof(WaterDataStruct));

  // Control characteristic
  waterControlChar.setMaxLen(100);
  waterControlChar.begin();
  waterControlChar.write("Idle");
  waterControlChar.setWriteCallback(onControlWritten);

  // Config - not used right now
  waterConfigChar.setMaxLen(100);
  waterConfigChar.begin();
  waterConfigChar.write("Default");

  // Status characteristic
  waterStatusChar.setMaxLen(100);
  waterStatusChar.begin();
  waterStatusChar.write("Sleeping");
  waterStatusChar.setWriteCallback(onStatusWritten);
  // (we may notify status from code using waterStatusChar.notify())
}

// Advertising and start
#define MANUFACTURER_ID   0x0059
uint8_t beaconUuid[16] = {
  0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78,
  0x89, 0x9a, 0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf0
};
BLEBeacon beacon(beaconUuid, 1, 2, -54);

void startAdv(void)
{
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(waterDataService);
  Bluefruit.ScanResponse.addName();

  // Connectable so RPi can connect
  Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED );
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(160, 160);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

void setup() {
  // Serial for logs
  // Serial.begin(115200);
  // while (!Serial) { delay(10); } // wait for serial

  pinMode(LED_BUILTIN, OUTPUT);

  logSerial("SYS", "Starting Bluefruit BLE peripheral");

  Bluefruit.begin();
  Bluefruit.setName("DataStream Capsule");
  // Blue LED off for lowest power
  Bluefruit.autoConnLed(false);
  Bluefruit.setTxPower(0);
  beacon.setManufacturer(MANUFACTURER_ID);

  // register connection callbacks
  Bluefruit.Periph.setConnectCallback(conn_cb);
  Bluefruit.Periph.setDisconnectCallback(disconn_cb);

  setBLECharacteristicDefaults();
  beginBLECharacteristics();
  startAdv();

  logSerial("SYS", "Advertising started");

  // If you want lower-power and no loop activity, use suspendLoop()
  // but ensure callbacks still work (Bluefruit uses interrupts for BLE)
  // suspendLoop();  // comment out for debugging if you want loop() to run
}

void loop() {
  // keep loop empty, or print periodic heartbeat for logs
  static unsigned long last = 0;
  if (millis() - last > 10000) {
    last = millis();
    logSerial("SYS", "Heartbeat - state: " + String(devState));
  }
}
