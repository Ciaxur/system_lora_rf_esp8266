#include <Adafruit_INA219.h>
#include <Adafruit_MPL3115A2.h>
#include <Adafruit_SSD1306.h>
#include <Base64.h>
#include <LoRa_E32.h>
#include <cmath>
#include <pins_arduino.h>

#include "include/shared_structs.h"

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
  0x01, // ADDH
  0x00, // ADDL
  0x17, // CHAN
};
NodeConfig peerNode = {
  0x00, // ADDH
  0x00, // ADDL
  0x17, // CHAN
};

Adafruit_MPL3115A2 baro;
Adafruit_INA219 ina219;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 for internal reset)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void enterErrorState() {
  while(true) {
      // Inidacte issue with LEDs.
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
    }
}

void setup() {
  // Wait for Software Serial to come up.
  Serial.begin(9600);
  while (!Serial) {};
  delay(2000);
  Serial.println("Serial initaialized");

  pinMode(LED_BUILTIN, OUTPUT);

  /// Setup LoRa device.
  if (!lora.begin()) {
    Serial.println("Failed to start LoRa.");
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

  // Setup Barometer.
  if (!baro.begin()) {
    Serial.println("Could not find sensor. Check wiring.");
    enterErrorState();
  }

  // Setup INA219.
  if (!ina219.begin()) {
    Serial.println("Failed to setup INA219. Check wiring");
    enterErrorState();
  }

  // Setup OLED Screen.
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED Screen failed");
    enterErrorState();
  }

  // Print the Adafruit Logo & clear, to indicate success.
  display.display();
  delay(1000);
}

bool configPrinted = false;

void loop() {
  display.clearDisplay();

  if (!configPrinted && micros64() > 2 * 1000000) {
    configPrinted = true;
    Serial.println("==== LoRa Configuration ====");
    Serial.printf("ADDH: %#02x\n", config.ADDH);
    Serial.printf("ADDL: %#02x\n", config.ADDL);
    Serial.printf("CHAN: %#02x\n", config.CHAN);
    Serial.printf("TX PWR: %#02x\n", config.OPTION.transmissionPower);
    Serial.printf("TX FIXED: %#02x\n", config.OPTION.fixedTransmission);
    Serial.println("============================");
    Serial.println("======== Peer Node =========");
    Serial.printf("ADDH: %#02x\n", peerNode.ADDH);
    Serial.printf("ADDL: %#02x\n", peerNode.ADDL);
    Serial.printf("CHAN: %#02x\n", peerNode.CHAN);
    Serial.println("============================");
  }

  // Store data in a shared struct to be transmitted.
  MessagePacket txPacket;

  // Get barometer sensor info.
  txPacket.pressure = baro.getPressure(); // hPa
  txPacket.temperature = baro.getTemperature(); // C
  txPacket.altitude = baro.getAltitude(); // m

  // Display barometer info.
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.printf("B[%.1fm|%.1fC]", txPacket.altitude, txPacket.temperature);

  // Get system power draw.
  float shuntvoltage = ina219.getShuntVoltage_mV();
  float busvoltage = ina219.getBusVoltage_V();
  txPacket.current_mA = abs(ina219.getCurrent_mA());
  txPacket.loadVoltage = busvoltage + (shuntvoltage / 1000);
  txPacket.power_mW = ina219.getPower_mW();

  // Display power info.
  display.setCursor(0, 9); // Font size is 6x8
  display.printf("P[%.2fV|%.2fmA]", txPacket.loadVoltage, txPacket.current_mA);
  display.setCursor(0, 18);
  display.printf("P[%.2fmW]", txPacket.power_mW);

  // Package the struct into a base64 string.g
  char *struct_bytes = reinterpret_cast<char*>(&txPacket);
  const int encoded_length = Base64.encodedLength(sizeof(MessagePacket));
  char buffer[encoded_length];
  Base64.encode(buffer, struct_bytes, sizeof(MessagePacket));
  String message{buffer};

  // Transmit data!
  Serial.printf("Sending data of size %ubytes:\n", message.length());
  Serial.printf("- pressure: %.2fhPa\n", txPacket.pressure);
  Serial.printf("- temperature: %.2fC\n", txPacket.temperature);
  Serial.printf("- altitude: %.2fm\n", txPacket.altitude);
  Serial.printf("- current_mA: %.2fmA\n", txPacket.current_mA);
  Serial.printf("- loadVoltage: %.2fV\n", txPacket.loadVoltage);
  Serial.printf("- power_mW: %.2fmW\n", txPacket.power_mW);

  ResponseStatus res = lora.sendFixedMessage(peerNode.ADDH, peerNode.ADDL, peerNode.CHAN, message);
  Serial.printf("TX Status: %s\n", res.getResponseDescription());


  // Push RAM to display.
  display.display();
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
}