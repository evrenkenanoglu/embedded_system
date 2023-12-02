/** @file       system.h
 *  @brief      System Wide Components
 *  @copyright  (c) 2021-Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       01/08/2021
 */
#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

/** INCLUDES ******************************************************************/

///** Standard Library Includes **///
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

///** Errors Includes **///
#include "error_definitions.h"

///** FreeRTOS Includes **///
#include "freertos/FreeRTOS.h"
#include "freertos/list.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"

///** POSIX Includes **///
#include "esp_pthread.h"
#include <pthread.h>

///** CPP Standard Includes **//
#include <chrono>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

//** Process Architecture Includes **//
#include "porting.h"
// #include "process.h"

///** uController Includes **///
#include "esp_log.h"

/** CONSTANTS *****************************************************************/
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define QUEUE_SIZE   32 // Default Queue List Size

/** TYPEDEFS ******************************************************************/

typedef enum
{
    //**Demo App **//
    eTaskDemo1 = 0,

    //**Demo App2 **//
    eTaskDemo2,

    eTaskMax,
} eTaskID_t;

typedef enum
{
    eProcessDemo1 = 1,
    eProcessDemo2,
    eProcessMax,
} eProcessID_t;

typedef struct
{
    eProcessID_t senderProcess; ///> Sender Process ID
    eTaskID_t    senderTask;    ///> Sender Task ID
    uint32_t     data;          ///> Data pointer
} Message_t;

/** MACROS ********************************************************************/

// #define GLOBAL_QUEUE_LIST_CREATE(_size) \ ///< Inter-Tasks Communications
// QueueHandle_t systemQueueList[eTaskMax];

#ifndef FILE_SYSTEM_C
#define INTERFACE extern
#else
#define INTERFACE
#endif

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/

#undef INTERFACE // Should not let this roam free

#endif // FILE_SYSTEM_H
