#include <Base64.h>
#include <LoRa_E32.h>
#include <pins_arduino.h>

#include "include/shared_structs.h"

/*
* Debug mode enables human readable serial prints for the received
* MessagePacket. If disabled, the packet is printed as a bytes array to
* be ingested and used from the host device.
*/
#define ENABLE_DEBUG


/*
* ESP8266MOD Pinout: https://randomnerdtutorials.com/esp8266-pinout-reference-gpios
* LoRa Library: https://github.com/sandeepmistry/arduino-LoRa
*/

/*
* LoRa E32-TTL-100 (SX1278) - Connections
* Write on serial to transfer a message to other device
*
* M0         ----- D7 (Dynamic adjustment) - Used to configure the chip.
* M1         ----- D6 (Dynamic adjustment) - Used to configure the chip.
* RX         ----- D4 (PullUP; R4.7kOhm)
* TX         ----- D3 (PullUP; R4.7kOhm)
* AUX        ----- D5
* VCC        ----- 3.3v/5v
* GND        ----- GND
*/
#define TX   D3
#define RX   D4
#define AUX  D5
#define M0   D7
#define M1   D6
LoRa_E32 lora(TX ,RX, AUX, M0, M1);
Configuration config;
NodeConfig curNode = {
  0x00, // ADDH
  0x00, // ADDL
  0x17, // CHAN
};


void enterErrorState() {
  pinMode(LED_BUILTIN, OUTPUT);
  while(true) {
    // Indiacate issue with LEDs.
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {};
  delay(2000);
  Serial.println("Serial initialized");

  Serial.println("Setting up LoRa");
  if (!lora.begin()) {
    Serial.println("Failed to setup LoRa");
    enterErrorState();
  }
  lora.setMode(MODE_0_NORMAL);

  /// Configure the device's address & channel.
  ResponseStructContainer configContainer = lora.getConfiguration();
  config = *(Configuration*) configContainer.data;
  config.ADDL = curNode.ADDL;
  config.ADDH = curNode.ADDH;
  config.CHAN = curNode.CHAN;
  config.OPTION.fixedTransmission = FT_FIXED_TRANSMISSION;

  // Applying the configuration will handle setting the device to PROGRAM mode
  // and reverting back to the initial mode.
    ResponseStatus rc = lora.setConfiguration(config, WRITE_CFG_PWR_DWN_SAVE);
    if (rc.code != E32_SUCCESS) {
      Serial.println("Failed to write config to LoRa");
      enterErrorState();
    }
}


bool ledState = false;
ulong lastLedStateChange = millis();
bool configPrinted = false;

void loop() {
  if (!configPrinted && micros64() > 2 * 1000000) {
    configPrinted = true;
    Serial.println("==== LoRa Configuration ====");
    Serial.printf("ADDH: %#02x\n", config.ADDH);
    Serial.printf("ADDL: %#02x\n", config.ADDL);
    Serial.printf("CHAN: %#02x\n", config.CHAN);
    Serial.printf("TX PWR: %#02x\n", config.OPTION.transmissionPower);
    Serial.printf("TX FIXED: %#02x\n", config.OPTION.fixedTransmission);
    Serial.println("============================");
  }


  if (lora.available()) {
    ResponseContainer res = lora.receiveMessage();

    if (res.status.code != E32_SUCCESS) {
      Serial.print("Failed to receive message: ");
      Serial.println(res.status.getResponseDescription());
    } else {
      Serial.printf("Received message of size %ubytes.\n", res.data.length());

      // Decode the received message.
      // Deep copy to remove the const.
      char encodedBuffer[res.data.length()];
      memcpy((void*)encodedBuffer, res.data.c_str(), res.data.length());

      const int decodedLength = Base64.decodedLength(encodedBuffer, sizeof(encodedBuffer));
      char decodedBuffer[decodedLength];
      Base64.decode(decodedBuffer, encodedBuffer, sizeof(encodedBuffer));

      #ifdef ENABLE_DEBUG
        // Construct the packet.
        MessagePacket packet = *(MessagePacket*) decodedBuffer;

        Serial.print("Received message: ");
        Serial.printf("- pressure: %.2fhPa\n", packet.pressure);
        Serial.printf("- temperature: %.2fC\n", packet.temperature);
        Serial.printf("- altitude: %.2fm\n", packet.altitude);
        Serial.printf("- current_mA: %.2fmA\n", packet.current_mA);
        Serial.printf("- loadVoltage: %.2fV\n", packet.loadVoltage);
        Serial.printf("- power_mW: %.2fmW\n", packet.power_mW);
      #else
        // Print decoded buffer to Serial.
        Serial.println(decodedBuffer);
      #endif
    }
  }

  // Blink LED for live check every 1s.
  if (millis() >= lastLedStateChange + 1000) {
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    ledState = !ledState;
    lastLedStateChange = millis();
  }

  // Reset variable if millis overflowed.
  // Arbitrary 10s check to verify that millis didn't reset to 0, since we're
  // setting the led state every 1s.
  else if (lastLedStateChange > (10*1000) && millis() < 1000) {
    lastLedStateChange = millis();
  }
}
