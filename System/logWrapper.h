#include <iostream> // You can replace this with the logging library of your choice

// Log Interface Wrapper Class
class LogWrapper {
public:
    enum class LogLevel {
        INFO,
        WARNING,
        ERROR
    };

    // Log a message with the given log level
    static void log(LogLevel level, const std::string& message) {
        switch (level) {
            case LogLevel::INFO:
                logInfo(message);
                break;
            case LogLevel::WARNING:
                logWarning(message);
                break;
            case LogLevel::ERROR:
                logError(message);
                break;
        }
    }

private:
    // Log functions for different log levels
    static void logInfo(const std::string& message) {
        std::cout << "[INFO] " << message << std::endl;
        // Replace this with the appropriate logging mechanism if needed
    }

    static void logWarning(const std::string& message) {
        std::cerr << "[WARNING] " << message << std::endl;
        // Replace this with the appropriate logging mechanism if needed
    }

    static void logError(const std::string& message) {
        std::cerr << "[ERROR] " << message << std::endl;
        // Replace this with the appropriate logging mechanism if needed
    }
};
