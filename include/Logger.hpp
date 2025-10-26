#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <chrono>
#include <string>
#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <mutex>
#include <memory>
#include <sstream>
#include <iomanip>
#include <utility>
#include <functional>
#include <stdexcept>
#include <ctime>


namespace LoggerUtils {

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

    inline const char* colorCode(Level level) {
        switch(level) {
            case Level::TRACE:    return "\033[90m";
            case Level::DEBUG:    return "\033[36m";
            case Level::INFO:     return "\033[32m";
            case Level::WARN:     return "\033[33m";
            case Level::ERROR:    return "\033[31m";
            case Level::CRITICAL: return "\033[1;31m";
            default:              return "\033[0m";
        }
    }

    inline std::string formatTime(const std::chrono::system_clock::time_point& tp, bool utc = true) {
        using namespace std::chrono;
        std::time_t t = system_clock::to_time_t(tp);

        std::tm tm;
    #if defined(_WIN32)
        if (utc) gmtime_s(&tm, &t); else localtime_s(&tm, &t);
    #else
        if (utc) gmtime_r(&t, &tm); else localtime_r(&t, &tm);
    #endif

        auto ms = duration_cast<milliseconds>(tp.time_since_epoch()).count() % 1000;

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.'
            << std::setw(3) << std::setfill('0') << ms;
        return oss.str();
    }


}


struct LogRecord {
    std::chrono::system_clock::time_point time;
    LoggerUtils::Level level;
    std::thread::id threadId;
    const char* file;
    int line;
    const char* func;
    std::string message;
};


class Sink {
    
    public:
        virtual ~Sink() = default;

        virtual void log(const LogRecord& record) = 0;

        void setLevel(LoggerUtils::Level l) { 
            level_ = l; 
        }

    protected:
        LoggerUtils::Level level_ = LoggerUtils::Level::TRACE;

        bool shouldLog(LoggerUtils::Level recordLevel) const {
            return recordLevel >= level_;
        }
};

class ConsoleSink: public Sink {

    public:

        ConsoleSink(){}

        void log(const LogRecord& record) override {

            if (!shouldLog(record.level)) {
                return;
            }

            auto tid_num = std::hash<std::thread::id>{}(record.threadId);
            
            auto formatted_time = LoggerUtils::formatTime(record.time);

            std::ostream& out = (record.level >= LoggerUtils::Level::ERROR) ? std::cerr : std::cout;

            std::lock_guard<std::mutex> lock(mutex_);

            out << formatted_time;
            out << " [" << LoggerUtils::colorCode(record.level) << LoggerUtils::levelToString(record.level) << "\033[0m" << "] ";
            out << "(tid: " << tid_num << ") ";
            out << "@ file: " << record.file << " function: " << record.func << " line: " << record.line;
            out << " Message: " << record.message << '\n';

            if (record.level >= LoggerUtils::Level::DEBUG) {
                out.flush();
            }
        
        }

    private:

        std::mutex mutex_;

};


class FileSink: public Sink {

    public:

        FileSink(const std::string& path): path_(path) {
            outfile_.open(path_, std::ios::out | std::ios::app);
            if (!outfile_.is_open()) {
                throw std::runtime_error("FileSink: failed to open log file: " + path_);
            }
        }

        ~FileSink() override {
            std::lock_guard<std::mutex> lock(mutex_);
            if (outfile_.is_open()) {
                outfile_.close();
            }
        }

        void log(const LogRecord& record) override {

            if (!shouldLog(record.level)) {
                return;
            }

            auto tid_num = std::hash<std::thread::id>{}(record.threadId);
            
            auto formatted_time = LoggerUtils::formatTime(record.time);

            std::lock_guard<std::mutex> lock(mutex_);

            if (!outfile_.is_open()) {
                return;
            }

            outfile_ << formatted_time;
            outfile_ << " [" << LoggerUtils::levelToString(record.level) << "] ";
            outfile_ << "(tid: " << tid_num << ") ";
            outfile_ << "@ file: " << record.file << " function: " << record.func << " line: " << record.line;
            outfile_ << " Message: " << record.message << '\n';

            if (record.level >= LoggerUtils::Level::DEBUG) {
                outfile_.flush();
            }
        
        }

    private:

        std::mutex mutex_;
        std::string path_;
        std::ofstream outfile_;
};


class Logger {

    public:

        static Logger& instance() {
            static Logger logger;
            return logger;
        }

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        void setLevel(LoggerUtils::Level l) {

            std::lock_guard<std::mutex> lock(mutex_);
            level_ = l;

            for (auto &s : sinks_) {
                if (s) {
                    s->setLevel(l);
                }
            }
        }

        LoggerUtils::Level getLevel() {
            std::lock_guard<std::mutex> lock(mutex_);
            return level_;
        }


        void log(LoggerUtils::Level level, const char* file,
                int line, const char* func, const std::string& message) {

            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (level < level_) {
                    return;
                }
            }

            LogRecord rec;
            rec.time = std::chrono::system_clock::now();
            rec.level = level;
            rec.threadId = std::this_thread::get_id();
            rec.file = file;
            rec.line = line;
            rec.func = func;
            rec.message = message;

            std::vector<std::shared_ptr<Sink>> sinks_copy;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                sinks_copy = sinks_;
            }

            for (auto &sink : sinks_copy) {
                if (!sink) {
                    continue;
                }
                try {
                    sink->log(rec);
                } catch (const std::exception& e) {
                    std::cerr << "Logger: sink threw exception: " << e.what() << '\n';
                } catch (...) {
                    std::cerr << "Logger: sink threw unknown exception\n";
                }
            }

        }

        template<typename... Args>
        void log(LoggerUtils::Level level, const char* file,
                int line, const char* func, Args&&... args) {
                    
            std::ostringstream oss;
            (oss << ... << std::forward<Args>(args));
            log(level, file, line, func, oss.str());
        }

        void addSink(std::shared_ptr<Sink> sink) {
            if (!sink) {
                return;
            }

            std::lock_guard<std::mutex> lock(mutex_);
            sink->setLevel(level_);
            sinks_.push_back(std::move(sink));
        }

        void clearSinks() {
            std::lock_guard<std::mutex> lock(mutex_);
            sinks_.clear();
        }

    private:
        Logger() : level_(LoggerUtils::Level::TRACE) {}
        ~Logger() = default;
        
        std::mutex mutex_;
        LoggerUtils::Level level_;
        std::vector<std::shared_ptr<Sink>> sinks_;

};


#define LOG_TRACE(...)   Logger::instance().log(LoggerUtils::Level::TRACE,   __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_DEBUG(...)   Logger::instance().log(LoggerUtils::Level::DEBUG,   __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(...)    Logger::instance().log(LoggerUtils::Level::INFO,    __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN(...)    Logger::instance().log(LoggerUtils::Level::WARN,    __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...)   Logger::instance().log(LoggerUtils::Level::ERROR,   __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_CRITICAL(...) Logger::instance().log(LoggerUtils::Level::CRITICAL, __FILE__, __LINE__, __func__, __VA_ARGS__)

#endif