#include <Adafruit_INA219.h>
#include <Adafruit_MPL3115A2.h>
#include <Adafruit_SSD1306.h>
#include <base64.hpp>
#include <LoRa_E32.h>
#include <cmath>
#include <pins_arduino.h>

#include "include/shared_structs.h"

/*
* ESP8266MOD Pinout: https://randomnerdtutorials.com/esp8266-pinout-reference-gpios
* LoRa Library: https://github.com/sandeepmistry/arduino-LoRa
*/
/* Only program the module once. */
// #define IS_PROGRAM_MODULE

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
NodeConfig curNode = {
  0x01, // ADDH
  0x00, // ADDL
  0x17, // CHAN
};
NodeConfig peerNode = {
  0x69, // ADDH
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
  #ifdef IS_PROGRAM_MODULE
    ResponseStructContainer configContainer = lora.getConfiguration();
    Configuration config = *(Configuration*) configContainer.data;
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
  #endif

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


void loop() {
  display.clearDisplay();

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

  // Package the struct into a base64 string.
  u_char *struct_bytes = reinterpret_cast<u_char*>(&txPacket);
  u_char buffer[255];
  const int buffer_len = encode_base64(struct_bytes, encode_base64_length(sizeof(MessagePacket)), buffer);

  // Transmit data!
  Serial.printf("Sending message struct of size %ubytes\n", buffer_len);
  Serial.printf("- pressure: %.2fhPa\n", txPacket.pressure);
  Serial.printf("- temperature: %.2fC\n", txPacket.temperature);
  Serial.printf("- altitude: %.2fm\n", txPacket.altitude);
  Serial.printf("- current_mA: %.2fmA\n", txPacket.current_mA);
  Serial.printf("- loadVoltage: %.2fV\n", txPacket.loadVoltage);
  Serial.printf("- power_mW: %.2fmW\n", txPacket.power_mW);

  ResponseStatus res = lora.sendFixedMessage(peerNode.ADDH, peerNode.ADDL, peerNode.CHAN, buffer, buffer_len);
  Serial.printf("TX Status: %s\n", res.getResponseDescription());


  // Push RAM to display.
  display.display();
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
}