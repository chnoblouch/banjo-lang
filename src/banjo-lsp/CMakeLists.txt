add_executable(banjo-lsp
    ast_navigation.cpp
    base_message_stream.cpp
    connection.cpp
    main.cpp
    memory_module_loader.cpp
    protocol_structs.cpp
    server.cpp
    symbol_ref_info.cpp
    uri.cpp
    workspace.cpp
    handlers/completion_handler.cpp
    handlers/definition_handler.cpp
    handlers/initialize_handler.cpp
    handlers/references_handler.cpp
    handlers/rename_handler.cpp
    handlers/semantic_tokens_handler.cpp
    handlers/shutdown_handler.cpp
)

target_include_directories(banjo-lsp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(banjo-lsp PUBLIC banjo)
