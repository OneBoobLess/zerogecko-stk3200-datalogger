Digital clock with
  Real Time Clock DS3231 (I2C),
  Rel. Humidity and Temperature Sensor Si7021 (I2C),
  and SD Card (SPI) with Memory LCD (SPI)
on the EFM32ZG-STK3200.

The user can start/stop data recording by pressing PB1 button.

The user can set time, and date using PB0 button, with Capsense buttons.

What makes this project special, is that it allows for simultaneous use
of the display over SPI bus, as well as writing the data to the SD Card.

For purpose of this project the original 16x20 font was expanded so to
include numbers and some text characters of the textdisplay driver.

Board:  Silicon Labs EFM32ZG-STK3200 Development Kit
Device: EFM32ZG222F32
Author: OneBoobLess, 2017
