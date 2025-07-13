#include "ErrorTranslateImpl.hpp"
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_definitions.h"
#include "esp_err.h"

ErrorTranslateImpl::ErrorTranslateImpl()
{
    SYS_LOG_I("ESP error translation implementation is initialized");
}

ErrorTranslateImpl& ErrorTranslateImpl::getInstance()
{
    static ErrorTranslateImpl instance;
    return instance;
}

void ErrorTranslateImpl::initialize()
{
    // Set Error TranslateHandler instance
    ErrorTranslateHandler::getInstance().setErrorTranslate(&getInstance());
}

std::string ErrorTranslateImpl::getErrorMessageName(int errorCode) const
{
    return std::string(esp_err_to_name(errorCode));
}

sys_error_t ErrorTranslateImpl::translateError(int errorCode) const
{
    switch (errorCode)
    {
        case ESP_OK:
            return ERROR_SUCCESS;

        case ESP_ERR_INVALID_ARG:
            return ERROR_INVALID_ARG;

        case ESP_ERR_TIMEOUT:
            return ERROR_TIMEOUT;

        case ESP_ERR_NO_MEM:
            return ERROR_OUT_OF_MEMORY;

        case ESP_ERR_INVALID_STATE:
            return ERROR_INVALID_STATE;

        case ESP_ERR_NOT_SUPPORTED:
            return ERROR_NOT_SUPPORTED;
            break;

        default:
            return ERROR_UNKNOWN;
    }
}