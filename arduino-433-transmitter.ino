#define RCSwitchDisableReceiving
#include <RCSwitch.h>
#include <Adafruit_MPL3115A2.h>
#include <pins_arduino.h>
#include <cmath>

RCSwitch mySwitch{};
Adafruit_MPL3115A2 baro;

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

  // Setup Barometer.
  if (!baro.begin()) {
    Serial.println("Could not find sensor. Check wiring.");
    while(true) {
      // Inidacte issue with LEDs.
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
    }
  }

  // Since we only care about altitude in order to be mapped to
  // parking floors, adjust the atlitude offset to not get negative values.
  // Currently we are roughly -30m in altitude.
  baro.setAltitudeOffset(50);
}


void loop() {
  // Get barometer sensor info.
  // float pressure = baro.getPressure(); // hPa
  // float temperature = baro.getTemperature(); // C
  float altitude = baro.getAltitude(); // m


  Serial.printf("[%lu] Sending code using protocol %d and repeats %d.\n", micros(), protocolNum, repeatedTransmit);
  Serial.printf("Sending adjusted (+50m offset) altitude = %.2fm\n", altitude);
  mySwitch.send(round(altitude), 24);

  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
}