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
    pool.start(4);

    Downloader& downloader = Downloader::instance(pool, "Downloads", "Adam/0.1"); //full path or just folder

    downloader.enqueue("https://www.britannica.com");
    downloader.enqueue("https://www.britannica.com/money/u3-unemployment-vs-u6-underemployment");
    downloader.enqueue("https://www.britannica.com/event/2025-NBA-Betting-and-Gambling-Scandal");
    downloader.enqueue("https://www.britannica.com/topic/National-Basketball-Association");

    pool.stop();

    LOG_INFO("All downloads completed");
    LOG_INFO("Downloader test finished");

    return 0;
}
