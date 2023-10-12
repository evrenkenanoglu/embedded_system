#include <iostream>

/**
 * @brief The ILogWrapper class is a platform interface for logging messages with different severity levels.
 */
class ILogWrapper
{
public:
    /**
     * @brief The LogLevel enum defines the different severity levels of log messages.
     */
    enum class LogLevel
    {
        INFO,    /**< Informational messages */
        WARNING, /**< Warning messages */
        ERROR    /**< Error messages */
    };

    /**
     * @brief Destructor for the ILogWrapper class.
     */
    virtual ~ILogWrapper(){};

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
     * @brief
     *
     * @param filename
     * @param level
     * @param message
     */
    virtual void logToFile(const std::string& filename, LogLevel level, const std::string& message) = 0;
};

/**
 * @brief The LogWrapperHandler class is a wrapper for the ILogWrapper class to log messages with different severity levels.
 */
class LogWrapperHandler
{
private:
    ILogWrapper* _logWrapperImpl = nullptr;

public:
    /**
     * @brief Constructs a new LogWrapperHandler object with the given ILogWrapper implementation.
     *
     * @param logWrapperImpl An implementation of the ILogWrapper interface.
     */
    LogWrapperHandler(ILogWrapper* logWrapperImpl) : _logWrapperImpl(logWrapperImpl) {}

    /**
     * @brief Destructor for the LogWrapperHandler class.
     */
    ~LogWrapperHandler() {}

    /**
     * @brief Logs a message with the given severity level.
     *
     * @param level The severity level of the message to log.
     * @param message The message to log.
     */
    void log(ILogWrapper::LogLevel level, const std::string& message)
    {
        switch (level)
        {
            case ILogWrapper::LogLevel::INFO:
                _logWrapperImpl->logInfo(message);
                break;
            case ILogWrapper::LogLevel::WARNING:
                _logWrapperImpl->logWarning(message);
                break;
            case ILogWrapper::LogLevel::ERROR:
                _logWrapperImpl->logError(message);
                break;
        }
    }
};

/**
 * @brief The logWrapperImpl class is an implementation of the ILogWrapper interface for logging messages to the console.
 */
class logWrapperImpl : public ILogWrapper
{
public:
    /**
     * @brief Constructs a new logWrapperImpl object.
     */
    logWrapperImpl()
    {
        std::cout << "LogWrapperImpl is initialized" << std::endl;
    }

    /**
     * @brief Destructor for the logWrapperImpl class.
     */
    ~logWrapperImpl() {}

    /**
     * @brief Logs an informational message to the console.
     *
     * @param message The message to log.
     */
    void logInfo(const std::string& message) override
    {
        // Replace this with the appropriate logging mechanism if needed
        std::cout << "[INFO] " << message << std::endl;
    }

    /**
     * @brief Logs a warning message to the console.
     *
     * @param message The message to log.
     */
    void logWarning(const std::string& message) override
    {
        // Replace this with the appropriate logging mechanism if needed
        std::cerr << "[WARNING] " << message << std::endl;
    }

    /**
     * @brief Logs an error message to the console.
     *
     * @param message The message to log.
     */
    void logError(const std::string& message) override
    {
        // Replace this with the appropriate logging mechanism if needed
        std::cerr << "[ERROR] " << message << std::endl;
    }

    void logToFile(const std::string& filename, LogLevel level, const std::string& message) override
    {
        std::ofstream file(filename, std::ios_base::app);
        if (file.is_open())
        {
            switch (level)
            {
                case LogLevel::INFO:
                    file << "[INFO] " << message << std::endl;
                    break;
                case LogLevel::WARNING:
                    file << "[WARNING] " << message << std::endl;
                    break;
                case LogLevel::ERROR:
                    file << "[ERROR] " << message << std::endl;
                    break;
            }
            file.close();
        }
        else
        {
            std::cerr << "Unable to open file " << filename << " for writing." << std::endl;
        }
    }
};

// An example of usage
// int main()
// {
//     logWrapperImpl logWrapperImpl;
//     LogWrapperHandler logWrapper(&logWrapperImpl);
//     logWrapper.log(ILogWrapper::LogLevel::INFO, "heyy");

//     return 0;
// }
