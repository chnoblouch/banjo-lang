#include "parallel_runner.hpp"

#include <iostream>
#include <mutex>
#include <string>
#include <thread>

namespace banjo {

namespace utils {

void ParallelRunner::Worker::launch(ParallelRunner &runner) {
    while (true) {
        std::unique_lock lock(mutex);
        condition_variable.wait(lock, [this] { return task.has_value() || !running; });
        lock.unlock();

        if (running) {
            (*task)();
        } else {
            break;
        }

        lock.lock();
        task = {};
        lock.unlock();

        tasks_run += 1;

        runner.finished_mutex.lock();
        runner.available_workers.push(this);
        runner.num_tasks_left -= 1;
        runner.finished_mutex.unlock();
        runner.finished_condition_variable.notify_one();
    }

    // std::string string = std::to_string(tasks_run) + "\n";
    // std::cout << string;
}

ParallelRunner::ParallelRunner(unsigned num_workers) {
    for (unsigned i = 0; i < num_workers; i++) {
        Worker *worker = new Worker();
        worker->running = true;
        worker->thread = std::thread([this, worker] { worker->launch(*this); });
        workers.push_back(worker);
    }
}

ParallelRunner::~ParallelRunner() {
    for (Worker *worker : workers) {
        worker->mutex.lock();
        worker->running = false;
        worker->mutex.unlock();
        worker->condition_variable.notify_one();
    }

    for (Worker *worker : workers) {
        worker->thread.join();
        delete worker;
    }
}

void ParallelRunner::run_blocking(std::deque<Task> tasks) {
    std::queue<Task> queue(std::move(tasks));
    num_tasks_left = queue.size();
    available_workers = {};

    // Fill up all workers with tasks.
    for (Worker *worker : workers) {
        Task task = std::move(queue.front());
        queue.pop();

        worker->mutex.lock();
        worker->task = {std::move(task)};
        worker->mutex.unlock();
        worker->condition_variable.notify_one();

        if (queue.empty()) {
            break;
        }
    }

    // While the task queue isn't empty, wait for a worker to finish and assign it a new task.
    while (!queue.empty()) {
        Task task = std::move(queue.front());
        queue.pop();

        std::unique_lock lock(finished_mutex);
        finished_condition_variable.wait(lock, [this] { return !available_workers.empty(); });

        Worker *worker = available_workers.front();
        available_workers.pop();

        worker->mutex.lock();
        worker->task = {std::move(task)};
        worker->mutex.unlock();
        worker->condition_variable.notify_one();

        lock.unlock();
    }

    // Once all tasks are assigned, wait for all workers to finish.
    while (num_tasks_left > 0) {
        std::unique_lock lock(finished_mutex);
        finished_condition_variable.wait(lock, [this] { return !available_workers.empty(); });
        available_workers.pop();
        lock.unlock();
    }
}

} // namespace utils

} // namespace banjo
