#include "thread_pool.hpp"
#include "logger.hpp"
#include <random>
#include <chrono>
#include <functional>

void workerTask(int id) {
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dist(1000, 3000);

    int wait_time = dist(gen);
    std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));

    LOG_INFO("Task ", id, " done after ", wait_time, " ms");
}

int main() {
    auto& logger = Logger::instance();
    logger.setLevel(LoggerUtils::Level::INFO);

    auto fileSink = std::make_shared<FileSink>("logs/threadpool_test.log");
    auto consoleSink = std::make_shared<ConsoleSink>();

    logger.addSink(fileSink);
    logger.addSink(consoleSink);

    LOG_INFO("ThreadPool test started");

    ThreadPool pool;
    pool.start(8);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= 16; ++i) {
        pool.enqueue(std::bind(workerTask, i));
    }

    pool.stop();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    LOG_INFO("Elapsed time: ", elapsed.count(), " seconds");
    LOG_INFO("ThreadPool test finished");

    return 0;
}
