#pragma once

// Shared struct between sender and receiver, which includes the intended payload.
struct MessagePacket {
  // Barometer data.
  float pressure;
  float temperature;
  float altitude;

  // Power consumption data.
  float current_mA;
  float loadVoltage;
  float power_mW;
};

// Fixed transmission peer node data.
struct PeerNode {
  byte ADDH;
  byte ADDL;
  byte CHAN;
};