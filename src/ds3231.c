/***************************************************************************//**
 * @file
 * @brief Driver for the DS3231 RTC: Obviously one on board of Zero Gecko blows
 * @version 1.0.0
 *******************************************************************************
 * @section License
 * <b>Copyright 2017 Marijan Kostrun, OSRAM </b>
 *******************************************************************************
 *
 * This file is for internal OSRAM use only.
 *
 ******************************************************************************/
#include <stddef.h>
#include "i2cspm.h"
#include "stddef.h"
#include "ds3231.h"

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/**************************************************************************//**
 * @brief
 *  Reads status of the T6713 sensor.
 * @param[in] i2c
 *   The I2C peripheral to use (not used).
 * @return
 *   Returns status byte .
 *****************************************************************************/
uint8_t ds3231_GetStatus(I2C_TypeDef *i2c)
{
  I2C_TransferSeq_TypeDef    seq;
  I2C_TransferReturn_TypeDef ret;
  uint8_t i2c_read_data[1]; // max value
  uint8_t i2c_write_data[1]; // max value

  seq.addr  = DS3231_ADDRESS;
  seq.flags = I2C_FLAG_WRITE_READ;

  /* Select command to issue */
  i2c_write_data[0] = STATUS;
  seq.buf[0].data   = i2c_write_data;
  seq.buf[0].len    = 1;

  /* Select location/length of data to be read */
  seq.buf[1].data = i2c_read_data;
  seq.buf[1].len  = 1;

  ret = I2CSPM_Transfer(i2c, &seq);
  if (ret != i2cTransferDone)
    return 0xff;

  return (i2c_read_data[0] & 0x04);
}

void ds3231_GetTime(I2C_TypeDef *i2c, volatile uint8_t * t)
{
  I2C_TransferSeq_TypeDef    seq;
  I2C_TransferReturn_TypeDef ret;
  uint8_t i2c_read_data[7]; // max value
  uint8_t i2c_write_data[1]; // max value

  seq.addr  = DS3231_ADDRESS;
  seq.flags = I2C_FLAG_WRITE_READ;

  /* Select command to issue */
  i2c_write_data[0] = SECOND;
  seq.buf[0].data   = i2c_write_data;
  seq.buf[0].len    = 1;

  /* Select location/length of data to be read */
  seq.buf[1].data = i2c_read_data;
  seq.buf[1].len  = 7;

  ret = I2CSPM_Transfer(i2c, &seq);
  while (ret != i2cTransferDone)
  {;}

  for (uint8_t i=0; i<7; i++)
  {
    t[i] = ds3231_ConvertTime(i2c_read_data[i]);
  }  
}

uint8_t ds3231_WriteCmdData(I2C_TypeDef *i2c, uint8_t cmd, uint8_t data)
{
  I2C_TransferSeq_TypeDef    seq;
  I2C_TransferReturn_TypeDef ret;
  uint8_t i2c_write_data[2];

  seq.addr  = DS3231_ADDRESS;
  seq.flags = I2C_FLAG_WRITE;

  i2c_write_data[0] = cmd;
  i2c_write_data[1] = data;

  /* Select command to issue */
  seq.buf[0].data   = i2c_write_data;
  seq.buf[0].len    = 2;

  ret = I2CSPM_Transfer(i2c, &seq);
  while (ret != i2cTransferDone)
  {;}

  return 0;
}


void  ds3231_SetTime(I2C_TypeDef *i2c, volatile uint8_t * local_time)
{
  I2C_TransferSeq_TypeDef    seq;
  I2C_TransferReturn_TypeDef ret;
  uint8_t i2c_write_data[8];

  seq.addr  = DS3231_ADDRESS;
  seq.flags = I2C_FLAG_WRITE;

  i2c_write_data[0] = SECOND;
  for (uint8_t i=1; i<8; i++)
    i2c_write_data[i] = ((local_time[i-1]/10)<<4) | (local_time[i-1] % 0x0a);

  /* Select command to issue */
  seq.buf[0].data   = i2c_write_data;
  seq.buf[0].len    = 8;

  ret = I2CSPM_Transfer(i2c, &seq);
  while (ret != i2cTransferDone)
  {;}
}


