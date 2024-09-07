cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0091 NEW)

project(banjo VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
enable_testing()

if (MSVC)
    add_compile_options(/W2)
else ()
    add_compile_options(-Wall -Wno-c++98-compat)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG=1)
else ()
    add_compile_definitions(DEBUG=0)

    if (MSVC)
        string(REGEX REPLACE "/EH[a-z]+|/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        add_compile_options(/EHs-c- /GR-)
    else ()
        string(REGEX REPLACE "-fexceptions|frtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        add_compile_options(-fno-exceptions -fno-rtti)
    endif ()

    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GCC")
        message(STATUS "LTO enabled")
        add_compile_options(-flto)
        add_link_options(-flto)
    endif ()
endif ()

# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
# add_compile_options(/fsanitize=address -fsanitize=undefined)
# link_libraries(clang_rt.asan-x86_64.lib)

string(TIMESTAMP VERSION "%Y-%m-%d")
add_compile_definitions(BANJO_VERSION="${VERSION}")

if (BANJO_ENABLE_SIR)
    add_compile_definitions(BANJO_ENABLE_SIR=1)
else ()
    add_compile_definitions(BANJO_ENABLE_SIR=0)
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    message(STATUS "Hot reloader enabled")
    set(BUILD_HOT_RELOADER ON)
else ()
    message(STATUS "Hot reloader disabled")
    set(BUILD_HOT_RELOADER OFF)
endif ()

add_subdirectory(src)
add_subdirectory(test)

install(TARGETS banjo-compiler RUNTIME DESTINATION bin)
install(TARGETS banjo-lsp RUNTIME DESTINATION bin)
install(TARGETS cli RUNTIME DESTINATION bin)

if (BUILD_HOT_RELOADER)
    install(TARGETS banjo-hot-reloader RUNTIME DESTINATION bin)
endif ()

if (WIN32)
    install(FILES $<TARGET_PDB_FILE:banjo-compiler> DESTINATION bin OPTIONAL)
    install(FILES $<TARGET_PDB_FILE:banjo-lsp> DESTINATION bin OPTIONAL)

    if (BUILD_HOT_RELOADER)
        install(FILES $<TARGET_PDB_FILE:banjo-hot-reloader> DESTINATION bin OPTIONAL)
    endif ()
endif()

install(DIRECTORY src/cli/python/ DESTINATION scripts/cli)
install(DIRECTORY src/bindgen DESTINATION scripts)

install(DIRECTORY lib/stdlib2 DESTINATION lib)
install(DIRECTORY lib/runtime/bin/ DESTINATION lib/runtime)
