/** @file       io_gpio.h
 *  @brief      HAL IO Driver For ESP32 GPIO
 *  @copyright  (c) 2023- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author      Evren Kenanoglu
 *  @date       12/02/2023
 */
#ifndef FILE_IO_GPIO_H
#define FILE_IO_GPIO_H

/** INCLUDES ******************************************************************/
#include "hal.h"
/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

#ifndef FILE_IO_GPIO_C
#define INTERFACE extern
#else
#define INTERFACE
#endif

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/

#undef INTERFACE // Should not let this roam free

#endif // FILE_IO_GPIO_H
