#include "hot_reloader.hpp"

#include "ast/ast_utils.hpp"
#include "diagnostics.hpp"
#include "ir_builder/ir_builder_utils.hpp"
#include "jit_compiler.hpp"

#include <cstring>
#include <fstream>

// clang-format off
#include <windows.h>
#include <dbghelp.h>
#include "file_watcher.hpp"
// clang-format on

namespace banjo {

namespace hot_reloader {

HotReloader::HotReloader() {
    Diagnostics::log("platform: x86_64-windows");
}

void HotReloader::load(const std::string &executable) {
    STARTUPINFO startup_info;
    ZeroMemory(&startup_info, sizeof(STARTUPINFO));

    char *command_line = new char[executable.size() + 1];
    std::memcpy(command_line, executable.c_str(), executable.size() + 1);

    PROCESS_INFORMATION process_info;
    BOOL result =
        CreateProcess(NULL, command_line, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &startup_info, &process_info);

    delete[] command_line;

    if (!result) {
        Diagnostics::abort("failed to load executable");
    }

    Diagnostics::log("executable loaded");
    process = TargetProcess(process_info.hProcess, process_info.hThread);

    std::ifstream stream("out/x86_64-windows-msvc-debug/addr_table.txt");
    addr_table.load(stream);
    Diagnostics::log("address table layout loaded");
}

void HotReloader::run(const std::filesystem::path &src_path) {
    running = true;

    FileWatcher watcher(src_path, [this](std::filesystem::path file_path) { reload_file(file_path); });

    while (running) {
        DEBUG_EVENT event;
        if (!WaitForDebugEvent(&event, INFINITE)) {
            break;
        }

        process_event(&event);
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE);
    }

    watcher.stop();
    terminate();
}

void HotReloader::process_event(void *event) {
    DEBUG_EVENT *win_event = (DEBUG_EVENT *)event;

    switch (win_event->dwDebugEventCode) {
        case CREATE_THREAD_DEBUG_EVENT: process_create_thread(); break;
        case EXIT_PROCESS_DEBUG_EVENT: running = false; break;
        default: break;
    }
}

void HotReloader::process_create_thread() {
    if (thread_created) {
        return;
    }
    thread_created = true;

    process.init_symbols();
    load_addr_table_ptr();
}

void HotReloader::load_addr_table_ptr() {
    addr_table_ptr = process.get_symbol_addr("addr_table");
    if (!addr_table_ptr) {
        Diagnostics::abort("failed to load pointer to address table");
    }

    Diagnostics::log("pointer to address table loaded");
}

void HotReloader::reload_file(const std::filesystem::path &file_path) {
    JITCompiler compiler(lang::Config::instance(), addr_table);
    if (!compiler.build_ir()) {
        Diagnostics::log("failed to reload file");
        return;
    }

    lang::SymbolTable *symbol_table = nullptr;
    std::filesystem::path absolute_path = std::filesystem::absolute(file_path);

    for (lang::ASTNode *module_node : compiler.get_module_manager().get_module_list()) {
        lang::ASTModule *module_ = module_node->as<lang::ASTModule>();

        if (std::filesystem::absolute(module_->get_file_path()) == absolute_path) {
            symbol_table = lang::ASTUtils::get_module_symbol_table(module_);
            break;
        }
    }

    if (!symbol_table) {
        return;
    }

    lang::ASTUtils::iterate_funcs(symbol_table, [this, &compiler](lang::Function *func) {
        std::string name = ir_builder::IRBuilderUtils::get_func_link_name(func);

        auto it = addr_table.find(name);
        if (it == addr_table.end()) {
            return;
        }

        unsigned index = it->second;
        BinModule module_ = compiler.compile_func(name);
        TargetProcess::LoadedFunc loaded_func = process.load_func(module_);
        update_func_addr(index, loaded_func.text_addr, name);
    });
}

void HotReloader::update_func_addr(unsigned index, void *new_addr, const std::string &name) {
    void *item_addr = (void *)((std::size_t)addr_table_ptr + 8 * index);
    BOOL result = WriteProcessMemory(process.get_process(), item_addr, &new_addr, 8, NULL);
    if (!result) {
        Diagnostics::abort("failed to write function address to address table");
    }

    Diagnostics::log("updated function '" + name + "'");
}

void HotReloader::free_func() {
    /*
    if(!func_addr) {
        return;
    }

    BOOL result = VirtualFreeEx(process, func_addr, func_size, MEM_DECOMMIT);
    if(!result) {
        Diagnostics::abort("failed to free memory from previous function version");
    }
    */
}

void HotReloader::terminate() {
    process.close();
    Diagnostics::log("process exited");
}

} // namespace hot_reloader

} // namespace banjo
