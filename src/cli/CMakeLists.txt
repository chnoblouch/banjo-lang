add_executable(cli
    main.cpp
)

target_include_directories(cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cli PUBLIC banjo)
set_target_properties(cli PROPERTIES OUTPUT_NAME "banjo")
