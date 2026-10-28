/** @file       system.h
 *  @brief      System Wide Components
 *  @copyright  (c) 2021- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       01/08/2021
 */

#pragma once

/** INCLUDES ******************************************************************/

// 1. Local / System Headers
#include "error_definitions.h"
#include "error_macros.h"
#include "system_macros.h"

// 2. Third-Party / FreeRTOS Headers
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/list.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>

// 3. C++ Standard Library Headers
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/
