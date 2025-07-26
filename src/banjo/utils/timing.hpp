#ifndef BANJO_UTILS_TIMING_H
#define BANJO_UTILS_TIMING_H

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#define PROFILING 0

#if PROFILING
#    define PROFILE_SCOPE_BEGIN(name) ScopeTimer::start_timer(name, false);
#    define PROFILE_SCOPE_END(name) ScopeTimer::stop_timer(name);
#    define PROFILE_SECTION_BEGIN(name) ScopeTimer::start_timer(name, true);
#    define PROFILE_SECTION_END(name) ScopeTimer::stop_timer(name);
#    define PROFILE_SCOPE(name) ScopeTimer timer(name, false);
#    define PROFILE_SECTION(name) ScopeTimer timer(name, true);
#else
#    define PROFILE_SCOPE_BEGIN(name)
#    define PROFILE_SCOPE_END(name)
#    define PROFILE_SECTION_BEGIN(name)
#    define PROFILE_SECTION_END(name)
#    define PROFILE_SCOPE(name)
#    define PROFILE_SECTION(name)
#endif

namespace banjo {

using TimingClock = std::chrono::high_resolution_clock;

struct TimingResult {
    std::string task;
    bool section;
    TimingClock::rep duration = 0;
};

class ScopeTimer {

private:
    static std::unordered_map<std::string, std::chrono::time_point<TimingClock>> timers;
    static std::vector<TimingResult> results;

    std::string task;

public:
    ScopeTimer(std::string task, bool section);
    ~ScopeTimer();

    static void start_timer(const std::string &task, bool section);
    static void stop_timer(const std::string &task);
    static void dump_results();
};

} // namespace banjo

#endif
