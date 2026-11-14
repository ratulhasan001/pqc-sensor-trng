# Sensor-Seeded Random Number Generator (Arduino prototype)

An early Arduino prototype from my exploration of hardware entropy sources for key generation on IoT edge devices. Every 5 seconds the sketch reads a BMP180 pressure/temperature sensor and several analog and digital sensor channels, mixes the readings into a 32-bit seed and a set of masks, re-seeds a modified Mersenne Twister (MT19937) generator, shows the seed and three generated numbers on an SSD1306 OLED, and streams 624 numbers to the serial port.

This repository is an exploratory prototype. It is **not** the implementation used in my research paper on entropy-assured post-quantum key generation, and its output is **not** cryptographically secure (see [Limitations](#limitations)).

## Features

- Reads the BMP180 (temperature, pressure, two altitude estimates and sea-level pressure) plus eight GPIO channels: analog A0-A5 and digital pins 3 and 4
- Derives 20 32-bit masks from the readings (XOR with fixed constants, plus shifts, ORs and sums of earlier masks); the digital light and Hall-effect inputs control bits 31 and 30 of a value that is XORed into every mask
- XORs all masks into a 32-bit seed and re-seeds the generator on every loop
- Modified MT19937 seeding: each word of the 624-word state is XORed with one of the sensor-derived masks during initialisation
- OLED output: the current seed and the first three numbers, with 4 decimal places
- Serial output (9600 baud): 624 numbers in [0, 1] per cycle, with 5 decimal places, 8 per line

## Hardware

| Component | Connection (from the sketch) |
| --- | --- |
| SSD1306 OLED, 128x64, I2C | I2C bus, address `0x3C` |
| BMP180 pressure/temperature sensor | I2C bus (used through the Adafruit BMP085 library, which supports the BMP180) |
| Sensor with x/y/z analog outputs | A0 (x), A1 (y), A2 (z) |
| Light sensor module | D3 (digital out), A3 (analog out) |
| Sound sensor | A4 (analog) |
| Hall-effect sensor module | D4 (digital out), A5 (analog out) |

The sketch does not name the board. Choose it with these constraints in mind:

- **RAM:** the generator state alone is 624 x 4 bytes (about 2.5 KB) and the OLED buffer needs another 1 KB, so a board with only 2 KB of SRAM (Arduino Uno or Nano) is not enough. An Arduino Mega 2560 (8 KB SRAM) has enough.
- **I2C pins:** both I2C devices share the board's hardware I2C bus (`Wire`). On a Mega 2560 that is SDA = pin 20, SCL = pin 21, which leaves A4 and A5 free for the sound and Hall sensors. On ATmega328P boards, A4/A5 are the I2C pins and would clash with those sensors.

## Software and libraries

- Arduino IDE (or `arduino-cli`)
- Adafruit BMP085 Library
- Adafruit SSD1306
- Adafruit GFX Library
- Adafruit BusIO (installed automatically as a dependency)
- `Wire` (bundled with the Arduino core)

## Project structure

```
pqc-sensor-trng/
└── custom_trng_with_display.ino   # The complete sketch
```

## Getting started

```bash
git clone https://github.com/ratulhasan001/pqc-sensor-trng.git
cd pqc-sensor-trng
```

Arduino tools expect a sketch to sit in a folder with the same name as the `.ino` file. Arduino IDE offers to create that folder when you open the file; with `arduino-cli`, create it yourself.

**Arduino IDE**

1. Install the libraries above from **Tools > Manage Libraries**.
2. Open `custom_trng_with_display.ino`, select your board and port, and click **Upload**.
3. Open **Tools > Serial Monitor** at **9600 baud**.

**arduino-cli** (example for an Arduino Mega 2560; replace the port with your own)

```bash
mkdir custom_trng_with_display
cp custom_trng_with_display.ino custom_trng_with_display/
arduino-cli core install arduino:avr
arduino-cli lib install "Adafruit BMP085 Library" "Adafruit SSD1306" "Adafruit GFX Library"
arduino-cli compile --fqbn arduino:avr:mega custom_trng_with_display
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:mega custom_trng_with_display
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=9600
```

If the OLED or BMP180 is not detected at start-up, the sketch prints an error to the serial port and stops.

## Limitations

- **Not cryptographically secure.** Mersenne Twister is a statistical PRNG; its output is predictable once enough outputs are observed, and it must not be used to generate keys.
- The seed is only 32 bits, and slowly changing readings such as temperature and pressure contribute little unpredictability. The sketch does no entropy estimation or health testing of the raw sensor data (for example, as described in NIST SP 800-90B).
- On AVR boards `double` is a 32-bit float, so each output value carries only about 24 bits of precision.

---

Ratul Hasan · [ratulhasan.dev](https://ratulhasan.dev)
