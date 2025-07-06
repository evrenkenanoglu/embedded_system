#ifndef IERRORTRANSLATE_H
#define IERRORTRANSLATE_H

#include "error_definitions.h"
#include <string>
/**
 * @brief
 *
 * This interface defines methods for translating error codes or conditions
 * into human-readable messages.
 */
class IErrorTranslate
{
public:
    /**
     * @brief
     */
    virtual ~IErrorTranslate() = default;

    /**
     * @brief
     *
     * @param errorCode The error code to translate
     * @return std::string The human-readable error message
     */
    virtual std::string getErrorMessageName(int errorCode) const = 0;

    /**
     * @brief Translates an error code into a system error type.
     *
     * @param errorCode The error code to translate
     * @return sys_error_t The translated system error type
     */
    virtual sys_error_t translateError(int errorCode) const = 0;

    /**
     * @brief
     *
     * @param errorCode The error code to check
     * @return int The severity level (higher means more severe)
     */
    virtual int getErrorSeverity(int errorCode) const = 0;
};

#endif // IERRORTRANSLATE_H