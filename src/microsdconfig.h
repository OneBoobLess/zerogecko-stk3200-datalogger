/***************************************************************************//**
 * @file
 * @brief Provide MicroSD SPI configuration parameters.
 * @version 3.20.5
 *******************************************************************************
 * @section License
 * <b>(C) Copyright 2014 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#ifndef __MICROSDCONFIG_H
#define __MICROSDCONFIG_H


#define SPI_PORTD     gpioPortD // USART1 (location #3) MISO and MOSI are on PORTD
#define SPI_PORTC     gpioPortC // USART1 (location #3) SS and SCLK are on PORTC
#define SPI_MISO_PIN  6  // PD6
#define SPI_MOSI_PIN  7  // PD7
#define SPI_CS_PIN    14 // PC14
#define SPI_SCLK_PIN  15 // PC15
#define SPI_nHLD_PIN  13 // PC13
#define SPI_nWP_PIN   11 // PC11




/* Don't increase MICROSD_HI_SPI_FREQ beyond 8MHz. Next step will be 16MHz  */
/* which is out of spec.                                                    */
#define MICROSD_HI_SPI_FREQ     8000000

#define MICROSD_LO_SPI_FREQ     100000
#define MICROSD_USART           USART1
#define MICROSD_LOC             USART_ROUTE_LOCATION_LOC3
#define MICROSD_CMUCLOCK        cmuClock_USART1
#define MICROSD_GPIOPORTC       SPI_PORTC
#define MICROSD_GPIOPORTD       SPI_PORTD
#define MICROSD_MOSIPIN         SPI_MOSI_PIN
#define MICROSD_MISOPIN         SPI_MISO_PIN
#define MICROSD_CSPIN           SPI_CS_PIN
#define MICROSD_CLKPIN          SPI_SCLK_PIN

#endif /* __MICROSDCONFIG_H */
