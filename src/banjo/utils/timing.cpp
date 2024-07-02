#include "timing.hpp"

#include <iomanip>
#include <iostream>

namespace banjo {

std::unordered_map<std::string, std::chrono::time_point<TimingClock>> ScopeTimer::timers;
std::vector<TimingResult> ScopeTimer::results;

ScopeTimer::ScopeTimer(std::string task, bool section) : task(task) {
    start_timer(task, section);
}

ScopeTimer::~ScopeTimer() {
    stop_timer(task);
}

void ScopeTimer::start_timer(const std::string &task, bool section) {
    bool task_present = false;

    for (TimingResult &result : results) {
        if (result.task == task) {
            task_present = true;
            break;
        }
    }

    if (!task_present) {
        results.push_back({task, section, 0});
    }

    timers[task] = TimingClock::now();
}

void ScopeTimer::stop_timer(const std::string &task) {
    auto start = timers[task];
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(TimingClock::now() - start);

    for (TimingResult &result : results) {
        if (result.task == task) {
            result.duration += duration.count();
            break;
        }
    }
}

void ScopeTimer::dump_results() {
    int max_task_name_len = 0;
    for (const TimingResult &result : results) {
        int task_name_len = (int)result.task.length();
        max_task_name_len = std::max(max_task_name_len, task_name_len);
    }

    for (const TimingResult &result : results) {
        if (result.section) {
            std::cout << std::string(64, '-') << "\n";
        }

        int task_name_len = (int)result.task.length();
        int padding = max_task_name_len - task_name_len;
        std::cout << std::string(padding, ' ') << result.task << ": ";
        std::cout << std::setfill(' ') << std::setw(8) << std::fixed << std::setprecision(3);
        std::cout << result.duration / 1000.0 << "ms\n";
    }

    std::cout << std::string(64, '-') << std::endl;
}

} // namespace banjo
