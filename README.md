Board:  Silicon Labs EFM32ZG-STK3200 Development Kit
Device: EFM32ZG222F32

Digital clock with
  Real Time Clock DS3231 (I2C),
  Rel. Humidity and Temperature Sensor Si7021 (I2C),
  and SD Card (SPI) with Memory LCD (SPI)
on the EFM32ZG-STK3200.

This is extension of the work by Jan Cumps, from 2014, posted at
https://www.element14.com/community/groups/roadtest/blog/2014/11/17/zero-gecko-follow-up-talking-to-a-microsd-card-via-spi
which allows data logging through SD Card to EFM32 Zero Gecko STK3200.

In the present project, the digital clock (example project for EFM32ZG) is combined
with the environmental monitor (temperature and relative humidity), where all data
is displayed on the LCD screen.
In addition, the data can be saved to the SD card, if user chooses so.

Challenge of the project was how to share SPI bus between the display and the SD Card,
considering that two library combined require more memory then what EMF32ZG has (4kB RAM),
see discussion initiated by WithBoobs at
http://community.silabs.com/t5/32-bit-MCU/ZeroGecko-with-SDK3200-How-to-add-SPI-SD-card-to-the-GLIB-ef/td-p/189292

This being said, the datalogger is operated as follows:
1.  The user can start/stop data recording by pressing PB1 button.
2.  The user can set time, and date, and write it to DS3231. PB0 button is used to select
what to change, while the Capsense buttons are used to increase/decrease current value. Cycling
PB0 makes a bar be drawn under a time/date setting that can be changed. Continue cycling PB0 until
the bar is no longer visible makes device accepts new values and store them in DS3231.

Also, for this project the original 16x20 numbers-only font was expanded with few characters ('C', '.' and '%')
so that temperature and relative humidity can be printed in the same size as the time digits
with their units.


Lastly, this project combines all files for the SimplicityStudio v.4.4 and later, minus the system specific files.



Author: OneBoobLess, 2017
