add_executable(banjo-test-util
    assembly_util.cpp
    main.cpp
)

target_include_directories(banjo-test-util PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(banjo-test-util PUBLIC banjo)
