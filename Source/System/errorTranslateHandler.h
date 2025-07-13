#ifndef IERROR_TRANSLATE_HANDLER_H
#define IERROR_TRANSLATE_HANDLER_H

#include "IErrorTranslate.h"
#include <string>

#define SYS_ERROR_TRANSLATER()     ErrorTranslateHandler::getInstance()
#define ERROR_MESSAGE(errorCode)   SYS_ERROR_TRANSLATER().getErrorMessageName(errorCode)
#define TRANSLATE_ERROR(errorCode) SYS_ERROR_TRANSLATER().translateError(errorCode)

/**
 * @brief Interface for translating error codes into human-readable messages
 *
 * This interface defines methods for translating error codes or conditions
 * into human-readable messages.
 * Singletons are used to ensure that there is only one instance of the error translation handler
 */
class ErrorTranslateHandler
{
private:
    IErrorTranslate* _errorTranslate = nullptr; ///< Pointer to the error translation implementation

private:
    /**
     * @brief Default constructor
     */
    ErrorTranslateHandler() = default;

public:
    /**
     * @brief Destructor
     */
    virtual ~ErrorTranslateHandler() = default;

    /**
     * @brief Get the instance of the error translation handler
     *
     * @return ErrorTranslateHandler& Reference to the singleton instance
     */
    static ErrorTranslateHandler& getInstance()
    {
        static ErrorTranslateHandler instance;
        return instance;
    }

    void setErrorTranslate(IErrorTranslate* errorTranslateImpl)
    {
        _errorTranslate = errorTranslateImpl;
    }

    std::string getErrorMessageName(int errorCode) const
    {
        if (_errorTranslate)
        {
            return _errorTranslate->getErrorMessageName(errorCode);
        }
        return "Unknown Error";
    }

    sys_error_t translateError(int errorCode) const
    {
        if (_errorTranslate)
        {
            return _errorTranslate->translateError(errorCode);
        }
        return ERROR_UNKNOWN;
    }
};

#endif // IERROR_TRANSLATE_HANDLER_H