if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM_SOURCES
        file_watcher_windows.cpp    
        target_process_windows.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_SOURCES
        file_watcher_linux.cpp
        platform_utils_linux.cpp
        target_process_linux.cpp
    )   
endif()

add_executable(banjo-hot-reloader
    diagnostics.cpp
    hot_reloader.cpp 
    jit_compiler.cpp   
    main.cpp
    ${PLATFORM_SOURCES}
)

target_include_directories(banjo-hot-reloader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(banjo-hot-reloader PUBLIC banjo)
