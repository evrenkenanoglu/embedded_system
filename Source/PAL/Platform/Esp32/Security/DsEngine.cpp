/** @file       DsEngine.cpp
 *  @brief      Implementation of the hardware Digital Signature (DS) peripheral driver.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *  @author     Evren Kenanoglu
 *  @date       19/09/2026
 */

/** INCLUDES ******************************************************************/

// 1. Matching Header File
#include "DsEngine.hpp"

// 2. Local Project / Protocol / HAL Headers
#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

// 3. C++ Standard Library Headers
#include <cstring>

/** FUNCTIONS *****************************************************************/

DsEngine::DsEngine()
    : _config{nullptr, HMAC_KEY0, 2048}
    , _mutex(xSemaphoreCreateMutex())
    , _isInitialized(false)
{
}

DsEngine::~DsEngine()
{
    deInit();

    if (_mutex != nullptr)
    {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }
}

sys_error_t DsEngine::init(const DsConfig_t& config)
{
    RETURN_IF_ERROR(
        (_isInitialized),                                      // Expression
        ERROR_SUCCESS,                                         // Error code
        SYS_LOG_I("Hardware DS Engine is already initialized") // Error message
    );

    RETURN_IF_ERROR(
        (_mutex == nullptr),                                            // Expression
        ERROR_OUT_OF_MEMORY,                                            // Error code
        SYS_LOG_E("Failed to allocate DS Engine synchronization mutex") // Error message
    );

    RETURN_IF_ERROR(
        (config.dsContext == nullptr),                           // Expression
        ERROR_INVALID_ARG,                                       // Error code
        SYS_LOG_E("Invalid null esp_ds_data_t pointer provided") // Error message
    );

    RETURN_IF_ERROR(
        (config.rsaBitLength != 2048 && config.rsaBitLength != 3072 && config.rsaBitLength != 4096), // Expression
        ERROR_INVALID_ARG,                                                                           // Error code
        SYS_LOG_E("Unsupported DS RSA bit length: %zu", config.rsaBitLength)                         // Error message
    );

    _config        = config;
    _isInitialized = true;

    SYS_LOG_I("Hardware DS Engine initialized successfully (RSA %zu bits, HMAC Key %d)", _config.rsaBitLength, _config.hmacKeyId);
    return ERROR_SUCCESS;
}