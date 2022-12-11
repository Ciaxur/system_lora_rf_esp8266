#define RCSwitchDisableReceiving
#include <RCSwitch.h>
#include <pins_arduino.h>

RCSwitch mySwitch{};

const int protocolNum = 6;
const int repeatedTransmit = 16;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello world");

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.printf("Transmitting on pin %d\n", D3);
  pinMode(D3, OUTPUT);
  mySwitch.enableTransmit(D3);

  mySwitch.setProtocol(protocolNum);
  mySwitch.setRepeatTransmit(repeatedTransmit);
}


void loop() {
  Serial.printf("[%lu] Sending code using protocol %d and repeats %d.\n", micros(), protocolNum, repeatedTransmit);
  Serial.println("  Sending 69");
  mySwitch.send(69, 24);

  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
}