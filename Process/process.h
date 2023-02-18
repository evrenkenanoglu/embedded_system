/** @file       process.h
 *  @brief      Process 
 *  @copyright  (c) 2021- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Deniz Fer
 *              Evren Kenanoglu 
 *  @date       07/08/2021
 */
#ifndef FILE_PROCESS_H
#define FILE_PROCESS_H

/** INCLUDES ******************************************************************/
#include "system.h"
/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/
class Process;

///@brief    Process number used for defining a process throughout the system
///@ingroup   PROCESS
typedef uint32_t processID_t;

///@brief     Process Enumaration
///@ingroup   PROCESS
typedef enum
{
    eProcessStateIdle,
    eProcessStateRunning,
    eProcessStateSuspended,
    eProcessStateTerminated,
} eProcessState_t;

/** MACROS ********************************************************************/
//** PROCESS Definitions **//

/** @brief      Process init function prototype
 *  @param      _name   Name of the function
 *  @details    process Pointer to tsProcess structure that holds the process information
 */
#define PROCESS_LIST_CREATE() std::list<Process *> ProcessList;

/** @brief      Process init function prototype
 *  @param      _name   Name of the function
 *  @details    process Pointer to tsProcess structure that holds the process information
 */
#define PROCESS_START_FUNC(_name) uint32_t _name()

/** @brief      Process suspend function prototype
 *  @param      _name   Name of the function
 *  @details    process Pointer to tsProcess structure that holds the process information
 */
#define PROCESS_SUSPEND_FUNC(_name) uint32_t _name()

/** @brief      Process resume function prototype
 *  @param      _name   Name of the function
 *  @details    process Pointer to tsProcess structure that holds the process information
 */
#define PROCESS_RESUME_FUNC(_name) uint32_t _name()

/** @brief      Process terminate function prototype
 *  @param      _name   Name of the function
 *  @details    process Pointer to tsProcess structure that holds the process information
 */
#define PROCESS_TERMINATE_FUNC(_name) uint32_t _name()

/** @brief      Process event handler function prototype
 *  @details    Process can change its event handler depending on the state.
 *              process Pointer to tsProcess structure that holds the process information
 *  @param      _name           Name of the function
 */
#define PROCESS_HANDLER_PROTO(_name) void _name()

/** @brief      Process state change macro
 *  @param      _processPtr   pointer of process object
 *  @param      _stateEnum    enumeration of state
 */
#define PROCESS_STATE_CHANGE(_processPtr, _stateEnum) _processPtr->state = _stateEnum

/** @brief      Process state change macro
 *  @param      _processPtr   pointer of process object
 *  @param      _stateEnum    enumeration of state
 */
// #define PROCESS_STATE_CURRENT(_eProcessID)
// {
//     do{
//         _eProcessID =
//     }while()
// }

#ifndef FILE_PROCESS_C
#define INTERFACE extern
#else
#define INTERFACE
#endif

/**
 * @brief Process Abstract Class
 * 
 */
class Process
{
protected:
    processID_t _ID;
    void *_parameters;
    const void *_constants;

public:
    std::list<TaskHandle_t *> _taskList;
    uint8_t _state;
    // virtual ~Process() {}
    virtual PROCESS_START_FUNC(start) = 0;
    virtual PROCESS_SUSPEND_FUNC(suspend) = 0;
    virtual PROCESS_RESUME_FUNC(resume) = 0;
    virtual PROCESS_TERMINATE_FUNC(terminate) = 0;
};

#define PROCESS_CREATE(_name, _id, _params, _consts) \
    _name._state = eProcessStateIdle;                \
    _name._ID = _enum;                               \
    _name._parameters = _params;                     \
    _name._constants = _consts;

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/

/** @brief      Start a Process
 *  @param[in]  Process Object pointer to target object
 *  @retval     EXIT_FAILURE
 *  @retval     EXIT_SUCCESS
 */
INTERFACE uint32_t processStart(Process *process);

/** @brief      Suspend a Process
 *  @param[in]  Process Object pointer to target object
 *  @retval     EXIT_FAILURE
 *  @retval     EXIT_SUCCESS
 */
INTERFACE uint32_t processSuspend(Process *process);

/** @brief      Suspend a Process
 *  @param[in]  Process Object pointer to target object
 *  @retval     EXIT_FAILURE
 *  @retval     EXIT_SUCCESS
 */
INTERFACE uint32_t processResume(Process *process);

/** @brief      Terminate a Process
 *  @param[in]  Process Object pointer to target object
 *  @retval     EXIT_FAILURE
 *  @retval     EXIT_SUCCESS
 */
INTERFACE uint32_t processTerminate(Process *process);

#undef INTERFACE // Should not let this roam free

#endif // FILE_PROCESS_H
