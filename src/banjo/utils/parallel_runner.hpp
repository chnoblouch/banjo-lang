#ifndef PARALLEL_RUNNER_H
#define PARALLEL_RUNNER_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>

namespace banjo {

namespace utils {

class ParallelRunner {

public:
    typedef std::function<void()> Task;

private:
    struct Worker {
        std::thread thread;
        bool running;

        std::optional<Task> task;
        std::mutex mutex;
        std::condition_variable condition_variable;

        unsigned tasks_run = 0;

        void launch(ParallelRunner &runner);
    };

    std::vector<Worker *> workers;
    std::queue<Worker *> available_workers;
    unsigned num_tasks_left;
    std::mutex finished_mutex;
    std::condition_variable finished_condition_variable;

public:
    ParallelRunner(unsigned num_workers);
    ParallelRunner(const ParallelRunner &) = delete;
    ParallelRunner(ParallelRunner &&) = delete;
    ~ParallelRunner();

    ParallelRunner &operator=(const ParallelRunner &) = delete;
    ParallelRunner &operator=(ParallelRunner &&) = delete;

    void run_blocking(std::deque<Task> tasks);
};

} // namespace utils

} // namespace banjo

#endif
