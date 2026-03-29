/**
 * Phoenix Engine - Logger Header
 * 
 * Multi-level logging system with file and console output
 */

#pragma once

#include <string>
#include <mutex>
#include <fstream>
#include <cstdarg>

namespace phoenix::core {

/**
 * @brief Log severity levels
 */
enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Fatal = 5
};

/**
 * @brief Convert log level to string
 * @param level Log level
 * @return String representation
 */
std::string logLevelToString(LogLevel level);

/**
 * @brief Logger configuration
 */
struct LoggerConfig {
    LogLevel minLevel = LogLevel::Info;
    bool logToFile = false;
    bool logToConsole = true;
    std::string logFilePath = "phoenix.log";
    bool includeTimestamp = true;
    bool includeLogLevel = true;
};

/**
 * @brief Centralized logging system
 */
class Logger {
public:
    /**
     * @brief Get the singleton logger instance
     * @return Reference to logger
     */
    static Logger& getInstance();
    
    /**
     * @brief Initialize logger with configuration
     * @param config Logger configuration
     * @return true on success, false on failure
     */
    bool initialize(const LoggerConfig& config);
    
    /**
     * @brief Shutdown logger and close file handles
     */
    void shutdown();
    
    /**
     * @brief Set minimum log level
     * @param level Minimum level to log
     */
    void setMinLevel(LogLevel level);
    
    /**
     * @brief Get current minimum log level
     * @return Current minimum log level
     */
    LogLevel getMinLevel() const;
    
    /**
     * @brief Log a message
     * @param level Log level
     * @param message Message to log
     * @param ... Format arguments
     */
    void log(LogLevel level, const std::string& message, ...);
    
    /**
     * @brief Log a formatted message
     * @param level Log level
     * @param format Format string
     * @param args Format arguments
     */
    void logV(LogLevel level, const char* format, va_list args);
    
    /**
     * @brief Convenience methods for different log levels
     */
    void trace(const std::string& message, ...);
    void debug(const std::string& message, ...);
    void info(const std::string& message, ...);
    void warn(const std::string& message, ...);
    void error(const std::string& message, ...);
    void fatal(const std::string& message, ...);
    
    /**
     * @brief Static convenience methods
     */
    static void traceMsg(const std::string& message, ...);
    static void debugMsg(const std::string& message, ...);
    static void infoMsg(const std::string& message, ...);
    static void warnMsg(const std::string& message, ...);
    static void errorMsg(const std::string& message, ...);
    static void fatalMsg(const std::string& message, ...);

private:
    Logger() = default;
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::string formatMessage(LogLevel level, const std::string& message);
    void writeToConsole(const std::string& message);
    void writeToFile(const std::string& message);
    
    LoggerConfig m_config;
    std::mutex m_mutex;
    std::ofstream m_logFile;
    bool m_initialized = false;
};

// Global logger instance for convenience
Logger& getLogger();

} // namespace phoenix::core
