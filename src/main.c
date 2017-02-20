/**************************************************************************//**
 * @file main.c
 * @brief Clock example for EFM32ZG-STK3200
 * @version 4.4.0
 *
 * This example shows how to optimize your code in order to drive
 * a graphical display in an energy friendly way.
 *
 ******************************************************************************
 * @section License
 * <b>Copyright 2015 Silicon Labs, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/
#define ZG_SHARE_MEMORY_WITH_FSFAT

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "em_i2c.h"
#include "i2cspm.h"
#include "si7013.h"
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_pcnt.h"
#include "em_prs.h"
#include "capsense.h"
#include "glib.h"
#include "bsp.h"
#include "bspconfig.h"
#include <stdio.h>

#include "ds3231.h"

#include "display.h"
#include "displayconfigall.h"


#define TIME_UPDATE_FROM_RTC_S    (120)
#define TIME_UPDATE_RHT_S         (5)


/* The current time reference. Number of seconds since midnight
 * January 1, 1970.  */
static volatile struct {
  unsigned int Display    : 1; /* redraw a frame */
  unsigned int RHT        : 1; /* get data from RHT sensor */
  unsigned int fromClock  : 1; /* update local_time from RTC */
  unsigned int toClock    : 1; /* update RTC from local_time */
  unsigned int saveFile   : 1; /* PB1 toggled to start saving data ? */
  unsigned int appendFile : 1; /* Append data point to file */
  unsigned int printMsg   : 1; /* print message on screen after point has been written to SD Card */
} update;

static volatile uint8_t adjust_time_mode = 0;

/* Global glib context */
GLIB_Context_t gc;

/** This flag tracks if we need to perform a new measurement. */
int32_t          tempData=0;
uint32_t         rhData=0;

/** RTC - because one with Zero Gecko blows **/
volatile uint8_t local_time[7] = {0}; // second, minute, hour, day, date, month, year
const char *ds3231_date_long[]      = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const uint8_t ds3231_date_strlen[7] = {      6,        6,         7,           9,          8,        6,          8 };
const char * num2str_2digits = "00010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899";
char clockString[13];


/***************************************************************************
 *
 * Zero Gecko: This is the buffer that will be shared between GLIB and FATFS 
 *
 ****************************************************************************/
#ifdef ZG_SHARE_MEMORY_WITH_FSFAT

/* Global glib context */
extern DISPLAY_PixelMatrix_t pixelMatrixBuffer;
GLIB_Context_t gc;
#include "ff.h"
#include "microsd.h"
#include "diskio.h"

// sd card
/* Ram buffers
 * BUFFERSIZE should be between 512 and 1024, depending on available ram on efm32
 */
#define TEST_FILENAME "WSLOG.TXT"
FIL fsrc;                       /* File objects */
FATFS Fatfs;                    /* File system specific */
FRESULT res;                    /* FatFs function common result code */
UINT br, bw;                    /* File read/write count */
DSTATUS resCard;                /* SDcard status */
int8_t *ramBufferWrite = NULL;  /* Temporary buffer for write file */

#define FILE_SAVE_INTERVAL_S  5
uint32_t irqCounter = 0;


/***************************************************************************//**
 * @brief
 *   This function is required by the FAT file system in order to provide
 *   timestamps for created files.
 *
 *   Refer to drivers/fatfs/doc/en/fattime.html for the format of this DWORD.
 * @return
 *    A DWORD containing the current time and date as a packed datastructure.
 *    The DOS date/time format is a bitmask:
 *
 *                   24                16                 8                 0
 *    +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+
 *    |Y|Y|Y|Y|Y|Y|Y|M| |M|M|M|D|D|D|D|D| |h|h|h|h|h|m|m|m| |m|m|m|s|s|s|s|s|
 *    +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+
 *    \___________/\________/\_________/ \________/\____________/\_________/
 *        year        month       day      hour       minute        second
 *
 *    The year is stored as an offset from 1980.
 *    Seconds are stored in two-second increments.
 ******************************************************************************/
DWORD get_fattime(void)
{
  DWORD t = (local_time[SECOND] >> 1);
  t |=  (local_time[MINUTE] << 5);
  t |=  (local_time[HOUR]   << 11);
  t |=  (local_time[DATE]   << 16);
  t |=  (local_time[MONTH]  << 21);
  t |=  (local_time[YEAR]   << 25);
  return t;
}


/*------------------------------------------------------------/
/ Open or create a file in append mode
/------------------------------------------------------------*/
FRESULT open_append (
    FIL* fp,            /* [OUT] File object to create */
    const char* path    /* [IN]  File name to be opened */
)
{
  FRESULT fr;

  /* Opens an existing file. If not exist, creates a new file. */
  fr = f_open(fp, path, FA_WRITE | FA_OPEN_ALWAYS);
  if (fr == FR_OK) {
    /* Seek to end of the file to append data */
    fr = f_lseek(fp, f_size(fp));
    if (fr != FR_OK)
      f_close(fp);
  }
  return fr;
}

int8_t logToSD(int32_t tempData, uint32_t rhData) 
{
  int8_t rval = 0;

  /*Detect micro-SD*/
  MICROSD_Init();                     /*Initialize MicroSD driver */

  /* Error.No micro-SD with FAT32 is present */
  rval = disk_initialize(0);       /*Check micro-SD card status */
  if (rval)
    return 1;

  /* Initialize filesystem */
  rval = f_mount(0, &Fatfs);
  if (rval != FR_OK)
  {
    /* Error.No micro-SD with FAT32 is present */
    return 2;
  }

  /* Open  the file for append */
  rval = open_append(&fsrc, TEST_FILENAME);
  if (rval != FR_OK) 
  {
    /* Error. Cannot create the file */
    return 3;
  }

  // if file empty, write header
  rval = f_size(&fsrc); 
  if (!rval) 
  {
    rval = f_printf(&fsrc, "temperature,humidity\n");
    if (rval < 0) 
    {
      /* Error. Cannot write header */
      return 4;
    }
  }

  // print full time stamp
  rval = f_printf(&fsrc, "20%02d", local_time[YEAR]);
  rval = f_printf(&fsrc, "%02d",   local_time[MONTH]);
  rval = f_printf(&fsrc, "%02d",   local_time[DATE]);
  rval = f_printf(&fsrc, "%02d",   local_time[HOUR]);
  rval = f_printf(&fsrc, "%02d",   local_time[MINUTE]);
  rval = f_printf(&fsrc, "%02d,",  local_time[SECOND]);
  if (rval < FR_OK)
  {
    /* Error. Cannot log data */
    return 4;
  }
  rval = f_printf(&fsrc, "%03d,%03d\n", tempData, rhData);
  if (rval < FR_OK)
  {
    /* Error. Cannot log data */
    return 4;
  }

  /* Close the file */
  rval = f_close(&fsrc);
  if (rval != FR_OK)
  {
    /* Error. Cannot close the file */
    return 5;
  }

  return 0;
}


#endif


/*****************************************************************************
 * @brief Setup GPIO interrupt for pushbuttons.
 *****************************************************************************/
static void gpioSetup(void)
{
  /* Enable GPIO clock */
  CMU_ClockEnable(cmuClock_GPIO, true);

  /* configure PD5 as input and enable interrupt on it */
  GPIO_PinModeSet(gpioPortD, 5, gpioModeInput, 0);
  GPIO_IntConfig (gpioPortD, 5, true, false, true); //rising, falling, enable

  /* Configure PB0 as input and enable interrupt  */
  GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInputPull, 1);
  GPIO_IntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, false, true, true);

  /* Configure PB1 as input and enable interrupt */
  GPIO_PinModeSet(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, gpioModeInputPull, 1);
  GPIO_IntConfig(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, false, true, true);

  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);

  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

/**************************************************************************//**
* @brief Unified GPIO Interrupt handler (pushbuttons)
*        PB0 allows to correct time shown on the LCD
*****************************************************************************/
void GPIO_Unified_IRQ(void)
{
  /* Get and clear all pending GPIO interrupts */
  uint32_t interruptMask = GPIO_IntGet();
  GPIO_IntClear(interruptMask);

  /* Act on interrupts */
  if (interruptMask & (1 << BSP_GPIO_PB0_PIN))
  {
    /* cycle through adjust time modes:
      0 - no adjustment
      1 - hours
      2 - minutes
      3 - seconds.
      4 - year
      5 - month
      6 - date
      7 - day
     */
    adjust_time_mode = (adjust_time_mode + 1) % 0x08;
  }

  /* Act on interrupts */
  if (interruptMask & (1 << BSP_GPIO_PB1_PIN))
  {
    /* toggle recording function */
    update.saveFile = (update.saveFile == 0);
  }

  /* PD5 connected to SQW from RTC - triggered every second */
  if (interruptMask & 0x20)
  {
    /* Increase time by 1s */
    irqCounter++;

    /* Do we save new data point? */
    if( !(irqCounter % FILE_SAVE_INTERVAL_S) )
      update.appendFile = update.saveFile;

    if( !(irqCounter % TIME_UPDATE_RHT_S) )
      update.RHT = 1;

    /* Do we update local_time from RTC ? */
    if (!adjust_time_mode)
    {
      if( !(irqCounter % TIME_UPDATE_FROM_RTC_S) )
        update.fromClock = 1;
      else
      {
        /* keep updating local_time */
        local_time[SECOND]++;
        if (local_time[SECOND]> 59)
        {
          local_time[SECOND] = 0;
          local_time[MINUTE]++;
          if (local_time[MINUTE] > 59)
          {
            local_time[MINUTE] = 0;
            local_time[HOUR]++;
            if (local_time[HOUR] > 23)
              local_time[HOUR] = 0;
          }
        }
      }
    }

    /* Notify main loop to redraw clock on display. */
    update.Display = true;
  }

}

/**************************************************************************//**
 * @brief GPIO Interrupt handler for even pins
 *****************************************************************************/
void GPIO_EVEN_IRQHandler(void)
{
  GPIO_Unified_IRQ();
}

/**************************************************************************//**
 * @brief GPIO Interrupt handler for odd pins
 *****************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
  GPIO_Unified_IRQ();
}


/**************************************************************************//**
 * @brief Helper function for printing numbers. Consumes less space than
 *        snprintf. Limited to two digits and one decimal.
 * @param string
 *        The string to print to
 * @param value
 *        The value to print
 *****************************************************************************/
static void GRAPHICS_CreateString(char *string, int32_t value)
{
  if (value < 0)
  {
    value = -value;
    string[0] = '-';
  }
  else
  {
    string[0] = ' ';
  }
  string[5] = 0;
  string[4] = '0' + (value % 1000) / 100;
  string[3] = '.';
  string[2] = '0' + (value % 10000) / 1000;
  string[1] = '0' + (value % 100000) / 10000;

  if (string[1] == '0')
  {
    string[1] = ' ';
  }
}

/**************************************************************************//**
 * @brief  Updates the digital clock.
 *
 *****************************************************************************/
void digitalClockUpdate()
{
  gc.backgroundColor = White;
  gc.foregroundColor = Black;
  GLIB_clear(&gc);

  // print time using 15x20 font
  GLIB_setFont(&gc, (GLIB_Font_t *)&GLIB_FontNumber16x20);
  clockString[0] = num2str_2digits[ 2 * local_time[HOUR] ];
  clockString[1] = num2str_2digits[ 2 * local_time[HOUR] + 1];
  clockString[2] = ':';
  clockString[3] = num2str_2digits[ 2 * local_time[MINUTE] ];
  clockString[4] = num2str_2digits[ 2 * local_time[MINUTE] + 1];
  clockString[5] = ':';
  clockString[6] = num2str_2digits[ 2 * local_time[SECOND] ];
  clockString[7] = num2str_2digits[ 2 * local_time[SECOND] + 1];
  GLIB_drawString(&gc, clockString, 8, 1, 105, true);

  if (adjust_time_mode)
  {
    switch(adjust_time_mode)
    {
      case 1:
      case 2:
      case 3:
        GLIB_drawLine(&gc, 1 + 48*(adjust_time_mode-1), 125,
                       1 + 48*(adjust_time_mode-1) + 32, 125);
        break;

      case 4:
      case 5:
      case 6:
        GLIB_drawLine(&gc, 16 + 24*(adjust_time_mode-4), 92,
                       32 + 24*(adjust_time_mode-4), 92);
        break;

      case 7:
        GLIB_drawLine(&gc, 1, 105, 32, 105);
        break;
    }
  }

  GRAPHICS_CreateString(clockString, tempData);
  clockString[5] = 'C';
  GLIB_drawString(&gc, clockString, 6, 32, 10, true);
  // rhData contains RH*10 
  GRAPHICS_CreateString(clockString, rhData);
  clockString[5] = '%';
  GLIB_drawString(&gc, clockString, 6, 32, 40, true);

  // create time stamp using 8x8 font
  GLIB_setFont(&gc, (GLIB_Font_t *)&GLIB_FontNormal8x8);
  clockString[0] = '2';
  clockString[1] = '0';
  clockString[2] = num2str_2digits[ 2 * local_time[YEAR] ];
  clockString[3] = num2str_2digits[ 2 * local_time[YEAR] + 1];
  clockString[4] = '/';
  clockString[5] = num2str_2digits[ 2 * local_time[MONTH] ];
  clockString[6] = num2str_2digits[ 2 * local_time[MONTH] + 1];
  clockString[7] = '/';
  clockString[8] = num2str_2digits[ 2 * local_time[DATE] ];
  clockString[9] = num2str_2digits[ 2 * local_time[DATE] + 1];
  GLIB_drawString(&gc, clockString, 10, 1, 83, true);
  GLIB_drawString(&gc, (char *) ds3231_date_long[local_time[DAY] % 7], ds3231_date_strlen[local_time[DAY] % 7], 1, 96, true);

  //
  if (update.printMsg)
  {
    GLIB_drawString(&gc, "REC", 3, 1, 12, true);
    update.printMsg = 0;
  }

  /* Update display */
  DMD_updateDisplay();
}

/**************************************************************************//**
 * @brief  Main function of clock example.
 *
 *****************************************************************************/
int main(void)
{
  EMSTATUS status;

  // I2C sensor for RTC, RH and TEMP data
  I2CSPM_Init_TypeDef i2cInit = I2CSPM_INIT_DEFAULT; // 100kHz simple master-slave
  i2cInit.i2cMaxFreq = I2C_FREQ_FAST_MAX;

  /* Chip errata */
  CHIP_Init();

  /* Use the 21 MHz band in order to decrease time spent awake.
     Note that 21 MHz is the highest HFRCO band on ZG. */
  CMU_ClockSelectSet( cmuClock_HF, cmuSelect_HFRCO  );
  CMU_HFRCOBandSet( cmuHFRCOBand_21MHz );

  /* Start capacitive sense buttons */
  CAPSENSE_Init();

  /* Setup GPIO for pushbuttons. */
  gpioSetup();

  /* Initialize display module */
  status = DISPLAY_Init();
  if (DISPLAY_EMSTATUS_OK != status)
  {
    while (true)
    {;}
  }

  /* Initialize the DMD module for the DISPLAY device driver. */
  status = DMD_init(0);
  if (DMD_OK != status)
  {
    while (true)
    {;}
  }

  status = GLIB_contextInit(&gc);
  if (GLIB_OK != status)
  {
    while (true)
    {;}
  }

#ifdef ZG_SHARE_MEMORY_WITH_FSFAT
  Fatfs.win = (BYTE *) pixelMatrixBuffer;
  ramBufferWrite = (int8_t *) &pixelMatrixBuffer[_MAX_SS];
#endif

  /* Set initial update flags */
  update.Display    = 1;
  update.RHT        = 1;
  update.fromClock  = 1;
  update.saveFile   = 0;
  update.appendFile = 0;
  update.toClock    = 0;


  /* Initialize I2C driver, using standard rate. */
  I2CSPM_Init(&i2cInit);

  /* Have RTC output 1 Hz pulse on SQW pin */
  ds3231_WriteCmdData(I2C0, CONTROL, 0x00);

  /* Get time from RTC and copy it to internal variables */
  ds3231_GetTime(I2C0, local_time);

  /* Infinite loop ONE AND ONLY */
  while (true)
  {
    EMU_EnterEM2(false); /* going green, no? */

   /*
    * update sensor data: here temperature and relative humidity from SI7021 over I2C
    */
    if (update.RHT)
    {
      Si7013_MeasureRHAndTemp(I2C0, SI7021_ADDR, &rhData, &tempData);
      update.RHT = 0;
    }

    /* if capsense buttons were active then local_time contains new time entered by user
     * and update_ds3231 is true.
     * once user finishes modifying the time (adjust_time_mode becomes false)
     * new local_time is used to update RTC
     *
     * conversely: every so often we update local_time from RTC
     */
    if(update.toClock && !adjust_time_mode)
    {
      ds3231_SetTime(I2C0, local_time);
      update.toClock = 0;
    }
    else if (update.fromClock)
    {
      ds3231_GetTime(I2C0, local_time);
      update.fromClock = 0;
    }

    /* print new time on display. also print time while user is modifying it. */
    if (update.Display)
    {
      /* Convert time format */
      digitalClockUpdate();
      update.Display = 0;
    }

    /* process capsense buttons as they indicate that the user has modified local_time */
    if (adjust_time_mode)
    {
      CAPSENSE_Sense();
      if ( CAPSENSE_getPressed(BUTTON1_CHANNEL) && !CAPSENSE_getPressed(BUTTON0_CHANNEL) )
      {
        update.toClock = 1;
        switch(adjust_time_mode)
        {
          case 1:
            local_time[HOUR] = (local_time[HOUR] + 1) % 24;
            break;

          case 2:
            local_time[MINUTE] = (local_time[MINUTE] + 1) % 60;
            break;

          case 3:
            local_time[SECOND] = (local_time[SECOND] + 1) % 60;
            break;

          case 4:
            local_time[YEAR] = (local_time[YEAR] + 1) % 100;
            break;

          case 5:
            local_time[MONTH] = (local_time[MONTH] + 1) % 12 + 1;
            break;

          case 6:
            local_time[DATE] = (local_time[DATE] + 1) % 31 + 1;
            break;

          case 7:
            local_time[DAY] = (local_time[DAY] + 1) % 7;
            break;
        }

        continue;

      } // if ( CAPSENSE_getPressed(BUTTON1_CHANNEL) && !CAPSENSE_getPressed(BUTTON0_CHANNEL) )

      if ( !CAPSENSE_getPressed(BUTTON1_CHANNEL) && CAPSENSE_getPressed(BUTTON0_CHANNEL) )
      {
        update.toClock = 1;
        switch(adjust_time_mode)
        {
          case 1:
            if (local_time[HOUR] == 0)
              local_time[HOUR] = 23;
            else
              local_time[HOUR]--;
            break;

          case 2:
            if (local_time[MINUTE] == 0)
              local_time[MINUTE] = 59;
            else
              local_time[MINUTE]--;
            break;

          case 3:
            if (local_time[SECOND] == 0)
              local_time[SECOND] = 59;
            else
              local_time[SECOND]--;
            break;

          case 4:
            if (local_time[YEAR] == 0)
              local_time[YEAR] = 99;
            else
              local_time[YEAR]--;
            break;

          case 5:
            if (local_time[MONTH] == 1)
              local_time[MONTH] = 12;
            else
              local_time[MONTH]--;
            break;

          case 6:
            if (local_time[DATE] == 1)
              local_time[DATE] = 31;
            else
              local_time[DATE]--;
            break;

          case 7:
            if (local_time[DAY] == 0)
              local_time[DAY] = 6;
            else
              local_time[DAY]--;
            break;
        }

        continue;

      }

    } // if (adjust_time_mode)

    /* if PB1 was toggled, then every so often save the data to SD Card.
     * Don't forget to reset SPI for DISPLAY Driver.
     */
    if (update.appendFile)
    {
      logToSD(tempData, rhData);
      update.appendFile = 0;
      update.printMsg = 1;

      /* Reset SPI for the DISPLAY Driver */
      PAL_TimerInit();
      PAL_SpiInit();
      PAL_GpioInit();
    }
  }
}



