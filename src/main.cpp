#include "thread_pool.hpp"
#include "logger.hpp"
#include "downloader.hpp"
#include <chrono>
#include <string>

int main() {

    auto& logger = Logger::instance();

    logger.setLevel(LoggerUtils::Level::TRACE);

    auto consoleSink = std::make_shared<ConsoleSink>();

    logger.addSink(consoleSink);

    LOG_INFO("Downloader test started");

    ThreadPool pool;
    pool.start(8);

    Downloader& downloader = Downloader::instance(pool);

    downloader.enqueue("https://www.google.com");
    downloader.enqueue("https://www.wikipedia.org");
    downloader.enqueue("https://www.britannica.com");
    downloader.enqueue("https://www.youtube.com");

    pool.stop();

    LOG_INFO("All downloads completed");
    LOG_INFO("Downloader test finished");

    return 0;
}
