#ifndef ILOG_H
#define ILOG_H

#include <iostream>
/**
 * @brief The ILog class is a platform interface for logging messages with different severity levels.
 */
class ILog
{
public:
    /**
     * @brief The LogLevel enum defines the different severity levels of log messages.
     */
    enum class LogLevel
    {
        INFO,    /**< Informational messages */
        WARNING, /**< Warning messages */
        ERROR,   /**< Error messages */
        DEBUG    /**< Debug messages */
    };

    /**
     * @brief Destructor for the ILog class.
     */
    virtual ~ILog() {};

    /**
     * @brief Logs an informational message.
     * @param message The message to log.
     */
    virtual void logInfo(const std::string& message) = 0;

    /**
     * @brief Logs a warning message.
     * @param message The message to log.
     */
    virtual void logWarning(const std::string& message) = 0;

    /**
     * @brief Logs an error message.
     * @param message The message to log.
     */
    virtual void logError(const std::string& message) = 0;

    /**
     * @brief Logs a debug message.
     * @param message The message to log.
     *
     */
    virtual void logDebug(const std::string& message) = 0;

    /**
     * @brief Logs a message to a file with the specified severity level.
     *
     * @param filename
     * @param level
     * @param message
     */
    virtual void logToFile(const std::string& filename, LogLevel level, const std::string& message) = 0;
};
#endif // ILOG_H

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
//     logWrapperImpl logWrapperImpl;
//     LogHandler logWrapper(&logWrapperImpl);
//     logWrapper.log(ILog::LogLevel::INFO, "heyy");

//     return 0;
// }
