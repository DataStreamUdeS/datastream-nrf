/*  Heavily based on the link below, Tested on iOS with "nRF Connect for Mobile"  */
/*  https://github.com/adafruit/Adafruit_nRF52_Arduino/blob/master/libraries/Bluefruit52Lib/examples/Peripheral/beacon/beacon.ino */

#include <Arduino.h>
#include <bluefruit.h>

struct WaterDataStruct {
  float depth;
  float temperature;
  float pH;
  float orp;
  float o2;
} waterDataPayload;


BLEService waterDataService = BLEService(0x180A); 
BLECharacteristic waterDataChar = BLECharacteristic(0x181A); 
BLECharacteristic waterControlChar = BLECharacteristic(0x182A);
BLECharacteristic waterConfigChar = BLECharacteristic(0x183A);
BLECharacteristic waterStatusChar = BLECharacteristic(0x184A);

void setBLECharacteristicDefaults() {
  waterDataChar.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  waterDataChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);

  waterControlChar.setProperties(CHR_PROPS_WRITE_WO_RESP | CHR_PROPS_WRITE);
  waterControlChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);

  waterConfigChar.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE);
  waterConfigChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);

  waterStatusChar.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  waterStatusChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);
}

void beginBLECharacteristics() {
  waterDataService.begin();

  waterDataChar.setMaxLen(sizeof(WaterDataStruct));
  waterDataChar.begin();

  waterControlChar.setMaxLen(20);
  waterControlChar.begin();

  waterConfigChar.setMaxLen(20);
  waterConfigChar.begin();

  waterStatusChar.setMaxLen(20);
  waterStatusChar.begin();
}


// For testing with this sketch, you can use nRF Beacon app
#define MANUFACTURER_ID   0x0059

// "nRF Connect" app can be used to detect beacon
uint8_t beaconUuid[16] = {
  0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78,
  0x89, 0x9a, 0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf0
};

// A valid Beacon packet consists of the following information:
// UUID, Major, Minor, RSSI @ 1M
BLEBeacon beacon(beaconUuid, 1, 2, -54);


void startAdv(void)
{  
  // Advertising packet
  // Set the beacon payload using the BLEBeacon class populated
  // earlier in this example
  waterDataService.begin();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(waterDataService);
  Bluefruit.Advertising.setBeacon(beacon);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * Apple Beacon specs
   * - Type: Non-connectable, scannable, undirected
   * - Fixed interval: 100 ms -> fast = slow = 100 ms
   */
  Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED);
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(160, 160);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

void setup() {

  Bluefruit.begin();
  Bluefruit.setName("DataStream Capsule");

  // off Blue LED for lowest power consumption
  Bluefruit.autoConnLed(false);
  Bluefruit.setTxPower(0);    // Check bluefruit.h for supported values

  // Manufacturer ID is required for Manufacturer Specific Data
  beacon.setManufacturer(MANUFACTURER_ID);

  // Setup the advertising packet
  startAdv();

  // Suspend Loop() to save power, since we didn't have any code there
  suspendLoop();
}



void loop() {
              // n o
}
