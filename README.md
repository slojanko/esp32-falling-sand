# ESP32 Falling Sand

ESP32-S3 implementation of the falling sand demo, displayed on a Matrix display. Project uses the following components:

* ESP32-S3
* P3 Indoor 64x64 LED display
* KY-023 Dual-Axis Joystick
* Adjustable Decoy Trigger Board (needs 5V) OR Adjustable DC power supply
* Lots of jumper cables

When installing the Matrix Display library, you need to set the pins used for connecting to the display. 
The configuration I used is in the esp32s3-default-pins.hpp file. This file needs to be copied over the one included with the library at:
```
~PathToMatrixDisplayLibrary/src/platforms/esp32s3/esp32s3-default-pins.hpp
```
This configuration uses pins from one row, avoiding any crossing connections and excludes the SPI pins (reserved for internal PSRAM) and RX/TX pins.

## Acknowledgments

* witnessmenow - Initial implementation of the [falling sand](https://github.com/witnessmenow/Falling-Sand-Matrix)
* mrfaptastic - [Library](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA) for the Matrix display
