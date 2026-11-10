/** @file       IHal_Sys_Reset.h
 *  @brief      Specialized interface for requesting a software system restart.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#pragma once

#include "IHal.h"

/**
 * @class IHal_Sys_Reset
 * @brief Specialization of IHAL_SYS providing a software-initiated system restart.
 */
class IHal_Sys_Reset : public IHAL_SYS
{
public:
    /**
     * @brief Restarts the microcontroller. Does not return.
     */
    [[noreturn]] virtual void restartSystem() = 0;
};
