/** @file       demoprocess.h
 *  @brief      Demo Process CPP Template
 *  @copyright  (c) 2021- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       03/10/2021
 */
#ifndef FILE_DEMOPROCESS_H
#define FILE_DEMOPROCESS_H

/** INCLUDES ******************************************************************/
#include "system.h"
/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

struct demoParams_t
{
    uint32_t dummyValue;
};

struct demoConsts_t
{
    const uint32_t constDummyValue;
};

class DemoProcess : public Process
{
private:
public:
    DemoProcess(/* args */) {}
    ~DemoProcess() {}
    uint32_t start();
    uint32_t suspend();
    uint32_t resume();
    uint32_t terminate();
};

#define PROCESS_DEMO_CREATE(_name, _enum) \
    DemoProcess _name;                    \
    demoParams_t _name##Params = {        \
        .dummyValue = 10,                 \
    };                                    \
    demoConsts_t _name##Consts = {        \
        .constDummyValue = 0,             \
    };

// _name._state = eProcessStateIdle;
// _name._ID = _enum;
// _name._parameters = &_name##Params;
// _name._constants = &_name##Consts;

/** MACROS ********************************************************************/

#ifndef FILE_DEMOPROCESS_C
#define INTERFACE extern
#else
#define INTERFACE
#endif

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/

#undef INTERFACE // Should not let this roam free

#endif // FILE_DEMOPROCESS_H
