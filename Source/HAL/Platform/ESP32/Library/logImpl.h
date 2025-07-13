#ifndef LOGIMPL_H
#define LOGIMPL_H

#include "System/ILog.h"
#include "System/LogHandler.h"
#include "esp_log.h"
#include <iostream>

class logImpl : public ILog
{
private:
    const char* _tag;

private:
    logImpl(const char* tag)
        : _tag(tag)
    {
        esp_log_level_set("APP", ESP_LOG_INFO); // Set default log level for the tag
        ESP_LOGI(_tag, "ESP logger wrapper implementation is initialized");
    }

public:
    // Delete copy constructor and assignment operator
    logImpl(const logImpl&)            = delete;
    logImpl& operator=(const logImpl&) = delete;

    ~logImpl()
    {
        ESP_LOGI(_tag, "ESP logger wrapper implementation is deinitialized");
    };

    static logImpl& getInstance()
    {
        static logImpl logEsp("APP");
        return logEsp;
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

    void logDebug(const std::string& message) override
    {
        std::cout << "[DEBUG] " << message << std::endl;
    }

    void logToFile(const std::string& filename, LogLevel level, const std::string& message) override
    {
        ESP_LOGE(_tag, "LOG-TO-FILE Feature Not Implemented");
    }

    // Initialize the global logger with this implementation
    static void initialize()
    {
        LogHandler::getInstance().setLogImplementation(&getInstance());
    }
};

#endif // LOGIMPL_H
