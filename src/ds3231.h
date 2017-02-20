#ifndef _DS3231_H_
#define _DS3231_H_

// Define registers per Extremely Accurate I2C Integrated RTC/TCXO/Crystal data sheet
// http://datasheets.maximintegrated.com/en/ds/DS3231.pdf
//
#define SECOND             0x00
#define MINUTE             0x01
#define HOUR               0x02
#define DAY                0x03
#define DATE               0x04
#define MONTH              0x05
#define YEAR               0x06
#define ALARM_1_SECONDS    0x07
#define ALARM_1_MINUTES    0x08
#define ALARM_1_HOURS      0x09
#define ALARM_1_DAY_DATE   0x0A
#define ALARM_2_MINUTES    0x0B
#define ALARM_2_HOURS      0x0C
#define ALARM_2_DAY_DATE   0x0D
#define CONTROL            0x0E
#define STATUS             0x0F
#define AGING_OFFSET       0x10
#define MSB_TEMP           0x11
#define LSB_TEMP           0x12  

// this is what we care about
#define DS3231_ADDRESS (0x68<<1)

// On-board 32k byte EEPROM; 128 pages of 32 bytes each
#define AT24C32_ADDRESS    0x57

#define ds3231_ConvertTime(x) (((x >> 4) * 10) + (x & 0x0F))

uint8_t ds3231_GetStatus(I2C_TypeDef *i2c);
void    ds3231_GetTime(I2C_TypeDef *i2c, volatile uint8_t * t);
void    ds3231_SetTime(I2C_TypeDef *i2c, volatile uint8_t * t);
uint8_t ds3231_WriteCmdData(I2C_TypeDef *i2c, uint8_t cmd, uint8_t data);

#endif

