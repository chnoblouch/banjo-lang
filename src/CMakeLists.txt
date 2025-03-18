set(BANJO_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(banjo)
add_subdirectory(banjo-compiler)
add_subdirectory(banjo-lsp)
add_subdirectory(banjo-format)
add_subdirectory(banjo-test-util)
add_subdirectory(cli)

if(BUILD_HOT_RELOADER)
    add_subdirectory(banjo-hot-reloader)
endif()
