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
#include <filesystem>


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

    inline std::string formatTime(const std::chrono::system_clock::time_point& tp, bool utc = false) {
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

           out  << '[' << formatted_time << "] "
                << '[' << LoggerUtils::colorCode(record.level)
                << LoggerUtils::levelToString(record.level)
                << "\033[0m] "
                << "[tid " << std::hex << tid_num << std::dec << "] "
                << record.file << ':' << record.line << ' '
                << record.func << "() -> "
                << record.message << '\n';


            if (record.level >= LoggerUtils::Level::DEBUG) {
                out.flush();
            }
        
        }

    private:

        std::mutex mutex_;

};


class FileSink: public Sink {

    public:

        FileSink(const std::string& base_path) : base_path_(base_path) {
            openDailyFile();
            if (!outfile_.is_open()) {
                throw std::runtime_error("FileSink: failed to open log file for " + current_path_.string());
            }
        }

        ~FileSink() override {
            std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
            if (lock.owns_lock() && outfile_.is_open()) {
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

            if (!outfile_.is_open() || todayString() != last_date_) {
                openDailyFile();
            }

            if (!outfile_.is_open()) {
                std::cerr << "FileSink::log: unable to open log file; dropping log entry\n";
                return;
            }

            outfile_<< '[' << formatted_time << "] "
                    << '[' << LoggerUtils::levelToString(record.level) << "] "
                    << "[tid " << std::hex << tid_num << std::dec << "] "
                    << record.file << ':' << record.line << ' '
                    << record.func << "() -> "
                    << record.message << '\n';

            if (record.level >= LoggerUtils::Level::DEBUG) {
                outfile_.flush();
            }
        
        }

    private:

        // return dd-mm-yyyy
        static std::string todayString(bool utc = false) {
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm tm;
        #if defined(_WIN32)
            if (utc) gmtime_s(&tm, &t); else localtime_s(&tm, &t);
        #else
            if (utc) gmtime_r(&t, &tm); else localtime_r(&t, &tm);
        #endif
            std::ostringstream ds;
            ds << std::setw(2) << std::setfill('0') << tm.tm_mday << '-'
            << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1) << '-'
            << (tm.tm_year + 1900);
            return ds.str();
        }


        void openDailyFile() {
            try {
                current_path_.clear();
                std::filesystem::path base(base_path_);
                std::string stem = base.stem().string();
                std::string ext = base.extension().string();
                std::filesystem::path parent = base.parent_path();

                if (stem.empty()) stem = "log";
                if (ext.empty()) ext = ".log";

                std::string date = todayString(); //dd-mm-yyyy
                std::string filename = stem + "." + date + ext; //e.g. app.26-10-2025.log

                std::filesystem::path dest = parent / filename;
                current_path_ = dest;

                last_date_ = date;

                if (outfile_.is_open()) {
                    outfile_.close();
                }

                try {
                    if (!parent.empty() && !std::filesystem::exists(parent)) {
                        std::filesystem::create_directories(parent);
                    }
                } 
                catch (const std::exception& e) {
                    std::cerr << "FileSink::openDailyFile exception: " << e.what() << '\n';
                } catch (...) {
                    std::cerr << "FileSink::openDailyFile unknown exception\n";
                }

                outfile_.open(current_path_, std::ios::out | std::ios::app);

            } catch (const std::exception& e) {
                std::cerr << "FileSink::openDailyFile exception: " << e.what() << '\n';
            } catch (...) {
                std::cerr << "FileSink::openDailyFile unknown exception\n";
            }
        }

        std::mutex mutex_;
        std::ofstream outfile_;
        std::string base_path_;
        std::filesystem::path current_path_;
        std::string last_date_;
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


        void logRaw(LoggerUtils::Level level, const char* file,
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
            logRaw(level, file, line, func, oss.str());
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