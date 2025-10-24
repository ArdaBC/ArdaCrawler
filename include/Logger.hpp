#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <chrono>
#include <string>
#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <mutex>

enum class Level {
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL,
    OFF
};

inline const char* levelToString(Level l) {
    switch (l) {
    case Level::TRACE:    return "TRACE";
    case Level::DEBUG:    return "DEBUG";
    case Level::INFO:     return "INFO";
    case Level::WARN:     return "WARN";
    case Level::ERROR:    return "ERROR";
    case Level::CRITICAL: return "CRITICAL";
    default:              return "UNKNOWN";
    }
}

struct LogRecord {
    std::chrono::system_clock::time_point time;
    Level level;
    std::thread::id threadId;
    std::string file;
    int line;
    std::string func;
    std::string message;
};

//TO DO: File vs Console sink classes

class Logger {

    public:

        static Logger& instance() {
            static Logger logger;
            return logger;
        }

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;


        void setLevel(Level l){}

        Level getLevel(){}

        void log(){}

        void addSink(){}

        void clearSinks(){}



    private:
        Logger();
        ~Logger() = default;
        
        std::mutex mutex;
        Level level;

};

#endif