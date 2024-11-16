add_executable(banjo-hot-reloader
    diagnostics.cpp
    file_watcher.cpp
    hot_reloader.cpp 
    jit_compiler.cpp   
    main.cpp
    target_process.cpp
)

target_include_directories(banjo-hot-reloader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(banjo-hot-reloader PUBLIC banjo)
