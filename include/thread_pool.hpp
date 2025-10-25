#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <queue>
#include <thread>
#include <condition_variable>
#include <vector>
#include <functional>
#include <stdexcept>

class ThreadPool {

    public:

        ThreadPool() : shutdown_(false) {}

        ~ThreadPool() {
            stop();
        }

        void start(int num) {

            if (!workers_.empty()) {
                throw std::runtime_error("ThreadPool already started");
            }

            int num_threads = std::thread::hardware_concurrency();
            
            if (num_threads == 0) {
                num_threads = 4;
            }
            
            num = std::min(num, num_threads);

            for(int i = 0; i < num; i++){
                workers_.emplace_back(&ThreadPool::loop, this);
            }
        }

        template<typename F>
        bool enqueue(F&& task) {
            {
                std::lock_guard<std::mutex> lock(mutex_);

                if (shutdown_) {
                    return false;
                }

                tasks_.push(std::forward<F>(task));
            }

            condition_.notify_one();
            return true;
        }


        void stop() {

            if (shutdown_) {
                return;
            }

            {
                std::unique_lock<std::mutex> lock(mutex_);
                shutdown_ = true;
            }

            condition_.notify_all();
            
            for(std::thread& worker: workers_){
                worker.join();
            }

            workers_.clear();
        }

    private:

        void loop() {

            while(true) {

                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(mutex_);

                    while(tasks_.empty() && !shutdown_){
                        condition_.wait(lock);
                    }

                    if(shutdown_ && tasks_.empty()){
                        return;
                    }

                    task = std::move(tasks_.front());
                    tasks_.pop();

                }

                task();
            }
        }

        std::vector<std::thread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex mutex_;
        std::condition_variable condition_;
        bool shutdown_;
};

#endif