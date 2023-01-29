# ESP8266 Transmitter (433MHz)
The transmitter device will use `fixed tranmission` to send data to a specific listening
device (*receiver*).

## TX Configuration
- `Transmission`: Destination traffic to a known `Device Address` and `Channel`.
- `Mode`: Normal mode (*since the ESP8266 will be going to deep sleep & thus handling low power mode*).

### Pinout
- `M0` and `M1` set to specific pins for dynamic adjustment ([configuring modes][4])


## Resources
- [esp8266 Pinout Reference][1]
- [E32 LoRa sx1278 Reference][2]
- [LoRa E32 Arduino Library GitHub][3]

[1]: https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
[2]: https://www.teachmemicro.com/e32-ttl-100-sx1278-lora-module/
[3]: https://github.com/xreef/LoRa_E32_Series_Library
[4]: https://www.teachmemicro.com/e32-ttl-100-sx1278-lora-module/#Configuring_Modes