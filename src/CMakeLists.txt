add_subdirectory(banjo)
add_subdirectory(banjo-compiler)
add_subdirectory(banjo-lsp)
add_subdirectory(cli)

if (BUILD_HOT_RELOADER)
    add_subdirectory(banjo-hot-reloader)
endif ()
