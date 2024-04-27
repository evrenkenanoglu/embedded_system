#ifndef LOGIMPL_H
#define LOGIMPL_H

#include "System/ILog.h"
#include "esp_log.h"
#include <iostream>

class logImpl : public ILog
{
private:
    const char* _tag;

public:
    // Delete copy constructor and assignment operator
    logImpl(const logImpl&)            = delete;
    logImpl& operator=(const logImpl&) = delete;

    ~logImpl()
    {
        ESP_LOGI(_tag, "ESP logger wrapper implementation is deinitialized");
    };

    logImpl(const char* tag) : _tag(tag)
    {
        ESP_LOGI(_tag, "ESP logger wrapper implementation is initialized");
    }
    /**
     * @brief Logs an informational message to the console.
     *
     * @param message The message to log.
     */
    void logInfo(const std::string& message) override
    {
        ESP_LOGI(_tag, "%s", message.c_str());
    }

    /**
     * @brief Logs a warning message to the console.
     *
     * @param message The message to log.
     */
    void logWarning(const std::string& message) override
    {
        ESP_LOGW(_tag, "%s", message.c_str());
    }

    /**
     * @brief Logs an error message to the console.
     *
     * @param message The message to log.
     */
    void logError(const std::string& message) override
    {
        ESP_LOGE(_tag, "%s", message.c_str());
    }

    void logToFile(const std::string& filename, LogLevel level, const std::string& message) override
    {
        ESP_LOGE(_tag, "LOG-TO-FILE Feature Not Implemented");
    }
};

// Public method to get the Singleton instance
static LogHandler& logger()
{
    static logImpl    logEsp("APP");
    static LogHandler handler(&logEsp);
    return handler;
}

#endif // LOGIMPL_H
