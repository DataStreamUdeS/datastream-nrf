#pragma once

struct WaterDataStruct {
  float depth;
  float temperature;
  float pH;
  float orp;
  float o2;
  float pression;};

// void fillPayloadWithDummyData(WaterDataStruct* payload) {
//   for (int i = 0; i < 12; i++) {
//     payload[i].depth = i * 10.0f;
//     payload[i].temperature = 20.0f + i;
//     payload[i].pH = 7.0f + (i * 0.1f);
//     payload[i].orp = 250.0f + (i * 5.0f);
//     payload[i].o2 = 8.0f - (i * 0.2f);
    
//   }
// }