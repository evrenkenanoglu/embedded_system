/** @file       IHal_Sys_AppInfo.h
 *  @brief      Specialized interface for reading the running application image descriptor.
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

#include <cstdint>

/**
 * @struct hal_app_info_t
 * @brief Platform-independent view of the running application image descriptor.
 */
struct hal_app_info_t
{
    const char* version;       ///< Firmware version string (null-terminated, owned by the image).
    uint32_t    secureVersion; ///< Anti-rollback secure version of the running image.
};

/**
 * @class IHal_Sys_AppInfo
 * @brief Specialization of IHAL_SYS providing access to the running application descriptor.
 */
class IHal_Sys_AppInfo : public IHAL_SYS
{
public:
    /**
     * @brief Reads the running application image descriptor.
     *
     * @param[out] info Receives the descriptor fields on success.
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_NULL_POINTER when info is
     *         nullptr, ERROR_NOT_FOUND when the image carries no descriptor.
     */
    virtual sys_error_t getRunningAppInfo(hal_app_info_t* info) = 0;
};
