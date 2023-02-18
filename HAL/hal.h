/** @file       ihal.h
 *  @brief      Hardware Abstraction Layer Interface
 *  @copyright  (c) 2023- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       12/02/2023
 */
#ifndef FILE_IHAL_H
#define FILE_IHAL_H

#include "system.h"

/** INCLUDES ******************************************************************/

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

class IHAL_IO
{
private:
    /* data */
public:
    virtual void*   get();
    virtual error_t set(void* data);
    ~IHAL_IO() {}
};

class IHAL_COM
{
private:
    /* data */
public:
    virtual error_t open(void);
    virtual void*   send();
    virtual void*   receive(void* data);
    virtual error_t close(void);
    ~IHAL_COM() {}
};

class IHAL_MEM
{
private:
    /* data */
public:
    virtual error_t init(void);
    virtual void*   get();
    virtual error_t set(void* data);
    ~IHAL_IO() {}
};

class IHAL_CPX
{
private:
    /* data */
public:
    virtual error_t start(void);
    virtual void*   get();
    virtual error_t set(void* data);
    virtual error_t stop(void);
    ~IHAL_IO() {}
};

/** MACROS ********************************************************************/

// #ifndef FILE_IHAL_C
// #define INTERFACE extern
// #else
// #define INTERFACE
// #endif

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/

#undef INTERFACE // Should not let this roam free

#endif // FILE_IHAL_H
