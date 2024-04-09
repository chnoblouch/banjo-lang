add_executable(banjo-compiler
    compiler.cpp
    main.cpp
)

target_include_directories(banjo-compiler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(banjo-compiler PUBLIC banjo)
