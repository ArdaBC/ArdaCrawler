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

        ThreadPool() : shutdown(false) {}

        ~ThreadPool() {
            stop();
        }

        void start(int num) {

            if (!workers.empty()) {
                throw std::runtime_error("ThreadPool already started");
            }

            int num_threads = std::thread::hardware_concurrency();
            
            if (num_threads == 0) {
                num_threads = 4;
            }
            
            num = std::min(num, num_threads);

            for(int i = 0; i < num; i++){
                workers.emplace_back(&ThreadPool::loop, this);
            }
        }

        template<typename F>
        bool enqueue(F&& task) {
            {
                std::lock_guard<std::mutex> lock(mutex);

                if (shutdown) {
                    return false;
                }

                tasks.push(std::forward<F>(task));
            }

            condition.notify_one();
            return true;
        }


        void stop() {

            if (shutdown) {
                return;
            }

            {
                std::unique_lock<std::mutex> lock(mutex);
                shutdown = true;
            }

            condition.notify_all();
            
            for(std::thread& worker: workers){
                worker.join();
            }

            workers.clear();
        }

    private:

        void loop() {

            while(true) {

                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(mutex);

                    while(tasks.empty() && !shutdown){
                        condition.wait(lock);
                    }

                    if(shutdown && tasks.empty()){
                        return;
                    }

                    task = std::move(tasks.front());
                    tasks.pop();

                }

                task();
            }
        }

        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex mutex;
        std::condition_variable condition;
        bool shutdown;
};

#endif