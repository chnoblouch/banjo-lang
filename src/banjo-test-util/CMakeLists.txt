add_executable(banjo-test-util
    assembly_util.cpp
    assembly_util.hpp
    main.cpp
    line_based_reader.cpp
    line_based_reader.hpp
    ssa_parser.cpp
    ssa_parser.hpp
    ssa_util.cpp
    ssa_util.hpp
)

target_include_directories(banjo-test-util PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(banjo-test-util PUBLIC banjo)
