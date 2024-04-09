add_library(test-utils STATIC
    ir_parser.cpp    
)

target_include_directories(test-utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test-utils PUBLIC banjo)
