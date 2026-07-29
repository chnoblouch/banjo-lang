if(TARGET_OS STREQUAL "windows")
    set(PLATFORM_SOURCES "process_windows.cpp")
elseif(TARGET_OS STREQUAL "linux" OR TARGET_OS STREQUAL "macos")
    set(PLATFORM_SOURCES "process_unix.cpp")
endif()

add_executable(cli
    "argument_parser.cpp"
    "argument_parser.hpp"
    "cli.cpp"
    "cli.hpp"
    "CMakeLists.txt"
    "common.cpp"
    "common.hpp"
    "main.cpp"
    "manifest.hpp"
    "paths.cpp"
    "paths.hpp"
    "process.hpp"
    "target.cpp"
    "target.hpp"
    "toolchains.cpp"
    "toolchains.hpp"
    ${PLATFORM_SOURCES}
)

target_include_directories(cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cli PUBLIC banjo)
set_target_properties(cli PROPERTIES OUTPUT_NAME "banjo")
