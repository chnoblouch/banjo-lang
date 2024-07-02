#ifndef HOT_RELOADER_H
#define HOT_RELOADER_H

#include "banjo/ir/addr_table.hpp"
#include "target_process.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>

namespace banjo {

namespace hot_reloader {

class HotReloader {

private:
    TargetProcess process{NULL, NULL};
    bool running = true;
    bool thread_created = false;
    void *addr_table_ptr;
    AddrTable addr_table;

public:
    HotReloader();
    void load(const std::string &executable);
    void run(const std::filesystem::path &src_path);

private:
    void process_event(void *event);
    void process_create_thread();
    void load_addr_table_ptr();
    void reload_file(const std::filesystem::path &file_path);
    void update_func_addr(unsigned index, void *new_addr, const std::string &name);
    void free_func();
    void terminate();
};

} // namespace hot_reloader

} // namespace banjo

#endif
