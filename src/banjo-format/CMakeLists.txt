add_executable(banjo-format
    "main.cpp"
)

target_include_directories(banjo-format PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(banjo-format PUBLIC banjo)
