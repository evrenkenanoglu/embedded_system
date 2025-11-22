#ifndef LOG_HANDLER_H
#define LOG_HANDLER_H

#include "ILog.h"
#include <memory>

// Log macros for convenience
#define SYS_LOGGER()            LogHandler::getInstance()
#define SYS_LOG_I(message, ...) SYS_LOGGER().log(ILog::LogLevel::INFO, message, ##__VA_ARGS__)
#define SYS_LOG_W(message, ...) SYS_LOGGER().log(ILog::LogLevel::WARNING, message, ##__VA_ARGS__)
#define SYS_LOG_E(message, ...) SYS_LOGGER().log(ILog::LogLevel::ERROR, message, ##__VA_ARGS__)

/**
 * @brief The LogHandler class is a wrapper for the ILog class to log messages with different severity levels.
 */
class LogHandler
{
private:
    ILog*          _logImpl = nullptr;
    ILog::LogLevel _logLevel;

private:
    /**
     * @brief Constructs a new LogHandler object with the given ILog implementation.
     *
     * @param logImpl An implementation of the ILog interface.
     */
    LogHandler(ILog* logImpl = nullptr)
        : _logImpl(logImpl)
        , _logLevel(ILog::LogLevel::INFO)
    {
        std::cout << "LogHandler is initialized" << std::endl;
    }

public:
    /**
     * @brief Destructor for the LogHandler class.
     */
    ~LogHandler() {}

    // Delete copy constructor and assignment operator
    LogHandler(const LogHandler&)            = delete;
    LogHandler& operator=(const LogHandler&) = delete;

    static LogHandler& getInstance()
    {
        static LogHandler _instance; // Ensure a single instance
        return _instance;
    }

    /**
     * @brief Sets the log implementation to use.
     *
     * @param logImpl An implementation of the ILog interface.
     */
    void setLogImplementation(ILog* logImpl)
    {
        _logImpl = logImpl;
    }

    void setLogLevel(ILog::LogLevel logLevel)
    {
        _logLevel = logLevel;
    }

    /**
     * @brief Logs a message with the given severity level.
     *
     * @param level The severity level of the message to log.
     * @param format The format string for the message to log.
     * @param args The arguments to format the message with.
     */
    template <typename... Args>
    void log(ILog::LogLevel level, const std::string& format, Args... args)
    {
        // Check if the log level is enabled
        if (level > _logLevel)
            return;

        if (_logImpl == nullptr)
        {
            return;
        }

        // Calculate required buffer size for the formatted string
        size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1; // +1 for null terminator
        if (size <= 0)
        {
            _logImpl->logError("Formatting error in log message");
            return;
        }

        // Create a buffer for the formatted string
        std::unique_ptr<char[]> buf(new char[size]);
        snprintf(buf.get(), size, format.c_str(), args...);

        // Create a C++ string from the buffer (excluding null terminator)
        std::string formattedMessage(buf.get(), buf.get() + size - 1);

        switch (level)
        {
            case ILog::LogLevel::INFO:
                _logImpl->logInfo(formattedMessage);
                break;
            case ILog::LogLevel::WARNING:
                _logImpl->logWarning(formattedMessage);
                break;
            case ILog::LogLevel::ERROR:
                _logImpl->logError(formattedMessage);
                break;
            case ILog::LogLevel::DEBUG:
                _logImpl->logDebug(formattedMessage);
                break;
        }
    }
};
#endif // LOG_HANDLER_H

#ifdef SYS_LOG_D
#undef SYS_LOG_D
#endif

// 2. Redefine based on the current state of the flag
#ifdef ENABLE_SYS_LOG_D
#define SYS_LOG_D(message, ...) SYS_LOGGER().log(ILog::LogLevel::DEBUG, message, ##__VA_ARGS__)
#else
#define SYS_LOG_D(message, ...) ((void)0)
#endif