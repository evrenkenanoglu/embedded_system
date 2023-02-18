/** @file       process.c
 *  @brief      Process 
 *  @copyright  (c) 2021- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Deniz Fer; Evren Kenanoglu; 
 *  @date       07/08/2021
 */
#define FILE_PROCESS_C

/** INCLUDES ******************************************************************/
#include "process.h"

using namespace std;

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTION DECLARATIONS ***********************************************/

/** INTERFACE FUNCTION DEFINITIONS ********************************************/
uint32_t processStart(Process* process)
{
    if ((process->_state == eProcessStateIdle) || (process->_state == eProcessStateTerminated))
    {
        // Critical Sections Enter

        process->start();

        // Critical Section Stop

        // Debug print for starting

        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}

uint32_t processSuspend(Process* process)
{
    if (process->_state == eProcessStateRunning)
    {
        // Critical Sections Enter

        process->suspend();

        // Critical Section Stop

        // Debug print for suspend

        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}

uint32_t processResume(Process* process)
{
    if (process->_state == eProcessStateSuspended)
    {
        // Critical Sections Enter
        process->resume();

        // Critical Section Stop

        // Debug print for suspend

        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}


uint32_t processTerminate(Process* process)
{
    if ((process->_state == eProcessStateRunning) || (process->_state == eProcessStateSuspended))
    {
        // Critical Sections Enter

        process->terminate();

        // Critical Section Stop

        // Debug print for termination

        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}
/** LOCAL FUNCTION DEFINITIONS ************************************************/
