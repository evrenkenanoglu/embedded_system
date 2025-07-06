#include "errorTranslate.hpp"
#include "System/errorTranslateHandler.h"
#include "System/error_definitions.h"
#include "esp_err.h"

ErrorTranslateImpl& ErrorTranslateImpl::getInstance()
{
    static ErrorTranslateImpl instance;
    return instance;
}

void ErrorTranslateImpl::initialize()
{
    // Set Error TranslateHandler instance
    ErrorTranslateHandler::getInstance().setErrorTranslate(this);
}

std::string ErrorTranslateImpl::getErrorMessageName(int errorCode) const
{
    return esp_err_to_name(errorCode);
}

int ErrorTranslateImpl::getErrorSeverity(int errorCode) const
{
    return ERROR_NOT_IMPLEMENTED;
}