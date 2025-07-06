#ifndef ERROR_TRANSLATE_ESP32_H
#define ERROR_TRANSLATE_ESP32_H

#include "System/IErrorTranslate.h"
#include "esp_err.h"
#include <unordered_map>

/**
 * @brief Implementation of error translation for ESP32 platform
 *
 * This class translates ESP32 error codes into human-readable messages
 * and provides information about error severity.
 * Singleton pattern is used to ensure a single instance of the error translator.
 */
class ErrorTranslateImpl : public IErrorTranslate
{
private:
    /**
     * @brief Default constructor
     *
     * Initializes the error messages and severities for common ESP32 errors.
     */
    void initialize();

    /**
     * @brief Default constructor
     */
    ErrorTranslateImpl();

public:
    // Delete copy constructor and assignment operator
    ErrorTranslateImpl(const ErrorTranslateImpl&)            = delete;
    ErrorTranslateImpl& operator=(const ErrorTranslateImpl&) = delete;

    /**
     * @brief Destructor
     */
    ~ErrorTranslateImpl() override = default;

    /**
     * @brief Get the singleton instance of ErrorTranslateImpl
     *
     * @return ErrorTranslateImpl& Reference to the singleton instance
     */
    static ErrorTranslateImpl& getInstance();

    std::string getErrorMessageName(int errorCode) const override;

    sys_error_t translateError(int errorCode) const override;

    int getErrorSeverity(int errorCode) const override;
};

#endif // ERROR_TRANSLATE_ESP32_H