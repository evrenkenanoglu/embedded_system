#ifndef LOG_HANDLER_H
#define LOG_HANDLER_H

#include "ILog.h"
#include <memory>

// Log macros for convenience
#define SYS_LOG_I(message, ...) LogHandler::getInstance().log(ILog::LogLevel::INFO, message, ##__VA_ARGS__)
#define SYS_LOG_W(message, ...) LogHandler::getInstance().log(ILog::LogLevel::WARNING, message, ##__VA_ARGS__)
#define SYS_LOG_E(message, ...) LogHandler::getInstance().log(ILog::LogLevel::ERROR, message, ##__VA_ARGS__)
#define SYS_LOG_D(message, ...) LogHandler::getInstance().log(ILog::LogLevel::DEBUG, message, ##__VA_ARGS__)

/**
 * @brief The LogHandler class is a wrapper for the ILog class to log messages with different severity levels.
 */
class LogHandler
{
private:
    ILog* _logImpl = nullptr;

private:
    /**
     * @brief Constructs a new LogHandler object with the given ILog implementation.
     *
     * @param logImpl An implementation of the ILog interface.
     */
    LogHandler(ILog* logImpl = nullptr)
        : _logImpl(logImpl)
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

// /**
//  * @brief The logWrapperImpl class is an implementation of the ILog interface for logging messages to the console.
//  */
// class logWrapperImpl : public ILog
// {
// public:
//     /**
//      * @brief Constructs a new logWrapperImpl object.
//      */
//     logWrapperImpl()
//     {
//         std::cout << "LogWrapperImpl is initialized" << std::endl;
//     }

//     /**
//      * @brief Destructor for the logWrapperImpl class.
//      */
//     ~logWrapperImpl() {}

//     /**
//      * @brief Logs an informational message to the console.
//      *
//      * @param message The message to log.
//      */
//     void logInfo(const std::string& message) override
//     {
//         // Replace this with the appropriate logging mechanism if needed
//         std::cout << "[INFO] " << message << std::endl;
//     }

//     /**
//      * @brief Logs a warning message to the console.
//      *
//      * @param message The message to log.
//      */
//     void logWarning(const std::string& message) override
//     {
//         // Replace this with the appropriate logging mechanism if needed
//         std::cerr << "[WARNING] " << message << std::endl;
//     }

//     /**
//      * @brief Logs an error message to the console.
//      *
//      * @param message The message to log.
//      */
//     void logError(const std::string& message) override
//     {
//         // Replace this with the appropriate logging mechanism if needed
//         std::cerr << "[ERROR] " << message << std::endl;
//     }

//     /**//      * @brief Logs a debug message to the console.
//      * @param message The message to log.
//      */
//     void logDebug(const std::string& message) override
//     {
//         // Replace this with the appropriate logging mechanism if needed
//         std::cout << "[DEBUG] " << message << std::endl;
//     }

//     void logToFile(const std::string& filename, LogLevel level, const std::string& message) override
//     {
//         std::ofstream file(filename, std::ios_base::app);
//         if (file.is_open())
//         {
//             switch (level)
//             {
//                 case LogLevel::INFO:
//                     file << "[INFO] " << message << std::endl;
//                     break;
//                 case LogLevel::WARNING:
//                     file << "[WARNING] " << message << std::endl;
//                     break;
//                 case LogLevel::ERROR:
//                     file << "[ERROR] " << message << std::endl;
//                     break;
//                 case LogLevel::DEBUG:
//                     file << "[DEBUG] " << message << std::endl;
//                     break;
//             }
//             file.close();
//         }
//         else
//         {
//             std::cerr << "Unable to open file " << filename << " for writing." << std::endl;
//         }
//     }
// };

// An example of usage
// int main()
// {
// // Create an implementation of the ILog interface
// LogWrapperImpl* logImpl = new LogWrapperImpl();

// // Set the implementation for the singleton logger
// logger().setLogImplementation(logImpl);

// // Now use the logging macros or the logger directly

// // Using macros (recommended way)
// SYS_LOG_I("This is an info message with parameter: %d", 42);
// SYS_LOG_W("This is a warning message");
// SYS_LOG_E("This is an error message with parameter: %s", "Error details");
// SYS_LOG_D("This is a debug message");

// // Or using the logger directly
// logger().log(ILog::LogLevel::INFO, "Direct info log: %s", "message");

// // Clean up (if application is ending)
// delete logImpl;
// }
