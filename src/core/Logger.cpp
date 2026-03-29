/**
 * Phoenix Engine - Logger
 * 
 * Centralized logging system with multiple log levels and outputs
 */

#include "../../include/phoenix/core/logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <mutex>

namespace phoenix::core {

// Global logger state
static struct {
    LogLevel minLevel = LogLevel::Info;
    std::ofstream fileHandle;
    bool logToFile = false;
    bool logToConsole = true;
    std::mutex mutex;
} g_logger;

static const char* getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

static std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void Logger::init(LogLevel minLevel, const std::string& logFile) {
    std::lock_guard<std::mutex> lock(g_logger.mutex);
    
    g_logger.minLevel = minLevel;
    
    if (!logFile.empty()) {
        g_logger.fileHandle.open(logFile, std::ios::app);
        g_logger.logToFile = g_logger.fileHandle.is_open();
    }
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(g_logger.mutex);
    
    if (g_logger.fileHandle.is_open()) {
        g_logger.fileHandle.close();
    }
    g_logger.logToFile = false;
}

void Logger::setMinLevel(LogLevel level) {
    g_logger.minLevel = level;
}

void Logger::enableFileLogging(const std::string& logFile) {
    std::lock_guard<std::mutex> lock(g_logger.mutex);
    
    if (g_logger.fileHandle.is_open()) {
        g_logger.fileHandle.close();
    }
    
    g_logger.fileHandle.open(logFile, std::ios::app);
    g_logger.logToFile = g_logger.fileHandle.is_open();
}

void Logger::disableFileLogging() {
    std::lock_guard<std::mutex> lock(g_logger.mutex);
    
    if (g_logger.fileHandle.is_open()) {
        g_logger.fileHandle.close();
    }
    g_logger.logToFile = false;
}

void Logger::enableConsoleLogging() {
    g_logger.logToConsole = true;
}

void Logger::disableConsoleLogging() {
    g_logger.logToConsole = false;
}

void Logger::log(LogLevel level, const std::string& message, const std::string& category) {
    if (level < g_logger.minLevel) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(g_logger.mutex);
    
    std::string timestamp = getTimestamp();
    std::string levelStr = getLevelString(level);
    
    std::stringstream line;
    line << "[" << timestamp << "] ";
    if (!category.empty()) {
        line << "[" << category << "] ";
    }
    line << "[" << levelStr << "] " << message;
    
    // Log to console
    if (g_logger.logToConsole) {
        if (level >= LogLevel::Error) {
            std::cerr << line.str() << std::endl;
        } else {
            std::cout << line.str() << std::endl;
        }
    }
    
    // Log to file
    if (g_logger.logToFile && g_logger.fileHandle.is_open()) {
        g_logger.fileHandle << line.str() << std::endl;
        g_logger.fileHandle.flush();
    }
}

void Logger::trace(const std::string& message, const std::string& category) {
    log(LogLevel::Trace, message, category);
}

void Logger::debug(const std::string& message, const std::string& category) {
    log(LogLevel::Debug, message, category);
}

void Logger::info(const std::string& message, const std::string& category) {
    log(LogLevel::Info, message, category);
}

void Logger::warning(const std::string& message, const std::string& category) {
    log(LogLevel::Warning, message, category);
}

void Logger::error(const std::string& message, const std::string& category) {
    log(LogLevel::Error, message, category);
}

void Logger::fatal(const std::string& message, const std::string& category) {
    log(LogLevel::Fatal, message, category);
}

} // namespace phoenix::core
