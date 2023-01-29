#include <LoRa_E32.h>
#include <pins_arduino.h>

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

  // TODO: Configure the device's address & channel.
}
bool ledState = false;
ulong lastLedStateChange = millis();

void loop() {
  if (lora.available()) {
    ResponseContainer res = lora.receiveMessage();

    if (res.status.code != E32_SUCCESS) {
      Serial.print("Failed to receive message: ");
      Serial.println(res.status.getResponseDescription());
    } else {
      Serial.print("Received message: ");
      Serial.println(res.data);
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
