#include "logger.hpp"
#include <thread>
#include <vector>
#include <chrono>

void workerTask(int id) {
    for (int i = 0; i < 5; ++i) {
        LOG_INFO("Worker ", id, " iteration ", i);
        LOG_DEBUG("Debug info from worker ", id, " iteration ", i);
        LOG_TRACE("Trace detail ", i, " of worker ", id);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    LOG_WARN("Worker ", id, " finished work.");
}

int main() {

    auto& logger = Logger::instance();

    logger.setLevel(LoggerUtils::Level::TRACE);

    auto fileSink = std::make_shared<FileSink>("logs/app.log");
    auto consoleSink = std::make_shared<ConsoleSink>();

    logger.addSink(fileSink);
    logger.addSink(consoleSink);

    LOG_INFO("Application started");
    LOG_DEBUG("Debug mode is active");
    LOG_WARN("This is a warning message");
    LOG_ERROR("This is an error message");
    LOG_CRITICAL("Critical failure simulation");

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(workerTask, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    logger.setLevel(LoggerUtils::Level::ERROR);
    
    LOG_INFO("This should NOT appear");
    LOG_ERROR("This should appear, level ERROR");

    LOG_CRITICAL("Application finished");

    return 0;
}
