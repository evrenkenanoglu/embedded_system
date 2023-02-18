/** @file       porting.h
 *  @brief      Porting macros functions... for System-OS
 *  @copyright  (c) 2021- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       08/08/2021
 */
#ifndef FILE_PORTING_H
#define FILE_PORTING_H

/** INCLUDES ******************************************************************/
#include "system.h"
/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

//** PORTING PROCESS MACROS **//

/**
 * @brief   Task Create Macro
 * @param   _taskFunc       Task function
 * @param   _tag            Name of task
 * @param   _stackSize      Stack size of function
 * @param   _parameterPtr   Pointer of parameter to pass task function
 * @param   _priorLevel     Priority Level 
 * @param   _handle         Handle of Task(Address of Handle -> &xhandle)
 *
 */
#define TASK_CREATE(_taskFunc, _tag, _stackSize, _parametersPtr, _priorLevel, _handle) xTaskCreate(_taskFunc, _tag, _stackSize, _parametersPtr, _priorLevel, _handle)

/**
 * @brief Task suspend macro
 * @param _handle Task Handle
 */
#define TASK_SUSPEND(_handle)                                                          vTaskSuspend(_handle)
/**
 * @brief Task resume macro
 * @param _handle Task Handle
 */
#define TASK_RESUME(_handle)                                                           vTaskResume(_handle)
/**
 * @brief Task terminate macro
 * @param _handle Task Handle
 */
#define TASK_TERMINATE(_handle)                                                        vTaskDelete(_handle)

#ifndef FILE_PORTING_C
#define INTERFACE extern
#else
#define INTERFACE
#endif

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/

#undef INTERFACE // Should not let this roam free

#endif // FILE_PORTING_H
