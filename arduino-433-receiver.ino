#include <LoRa_E32.h>
#include <pins_arduino.h>
#include <Base64.h>

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
* AUX        ----- Not connected
* VCC        ----- 3.3v/5v
* GND        ----- GND
*/
#define TX   D3
#define RX   D4
#define AUX  D5 // not connected though.
#define M0   D7
#define M1   D6
LoRa_E32 lora(TX ,RX, AUX, M0, M1);


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
  Serial.println("Setting up LoRa");
  if (!lora.begin()) {
    Serial.println("Failed to setup LoRa");
    enterErrorState();
  }
  lora.setMode(MODE_0_NORMAL);

  /// Configure the device's address & channel.
  ResponseStructContainer configContainer = lora.getConfiguration();
  Configuration config = *(Configuration*) configContainer.data;

  // Device address = 0x0069 on Channel 7.
  config.ADDL = 0x00;
  config.ADDH = 0x69;
  config.CHAN = 0x07;
  config.OPTION.fixedTransmission = FT_FIXED_TRANSMISSION;

  // Applying the configuration will handle setting the device to PROGRAM mode
  // and reverting back to the initial mode.
  lora.setConfiguration(config, WRITE_CFG_PWR_DWN_SAVE);
}


bool ledState = false;
ulong lastLedStateChange = millis();

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

void loop() {
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

      // Construct the packet.
      MessagePacket packet = *(MessagePacket*) decodedBuffer;

      Serial.print("Received message: ");
      Serial.printf("- pressure: %.2fhPa\n", packet.pressure);
      Serial.printf("- temperature: %.2fC\n", packet.temperature);
      Serial.printf("- altitude: %.2fm\n", packet.altitude);
      Serial.printf("- current_mA: %.2fmA\n", packet.current_mA);
      Serial.printf("- loadVoltage: %.2fV\n", packet.loadVoltage);
      Serial.printf("- power_mW: %.2fmW\n", packet.power_mW);
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
