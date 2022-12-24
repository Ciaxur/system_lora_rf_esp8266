#define RCSwitchDisableReceiving
#include <RCSwitch.h>
#include <Adafruit_MPL3115A2.h>
#include <pins_arduino.h>
#include <cmath>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>

RCSwitch mySwitch{};
Adafruit_MPL3115A2 baro;
Adafruit_INA219 ina219;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 for internal reset)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int protocolNum = 6;
const int repeatedTransmit = 16;

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
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.printf("Transmitting on pin %d\n", D3);
  pinMode(D3, OUTPUT);
  mySwitch.enableTransmit(D3);

  mySwitch.setProtocol(protocolNum);
  mySwitch.setRepeatTransmit(repeatedTransmit);

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

  // Get barometer sensor info.
  // float pressure = baro.getPressure(); // hPa
  float temperature = baro.getTemperature(); // C
  float altitude = baro.getAltitude(); // m

  // Display barometer info.
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.printf("B[%.1fm|%.1fC]", altitude, temperature);

  // Get system power draw.
  float shuntvoltage = ina219.getShuntVoltage_mV();
  float busvoltage = ina219.getBusVoltage_V();
  float current_mA = abs(ina219.getCurrent_mA());
  float loadvoltage = busvoltage + (shuntvoltage / 1000);
  float power_mW = ina219.getPower_mW();

  // Display power info.
  display.setCursor(0, 9); // Font size is 6x8
  display.printf("P[%.2fV|%.2fmA]", loadvoltage, current_mA);
  display.setCursor(0, 18);
  display.printf("P[%.2fmW]", power_mW);

  // Construct data.
  uint64_t dataToTransmit = round(altitude);

  // Transmit data!
  Serial.printf("[%lu] Sending code using protocol %d and repeats %d.\n", micros(), protocolNum, repeatedTransmit);
  Serial.printf("Sending altitude = %.2fm\n", altitude);
  mySwitch.send(dataToTransmit, 24);


  // Push RAM to display.
  display.display();
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
}