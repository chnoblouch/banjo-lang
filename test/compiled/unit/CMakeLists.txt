add_executable(test-large-int large_int.cpp)
target_include_directories(test-large-int PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test-large-int PRIVATE banjo)
add_test(NAME large_int COMMAND $<TARGET_FILE:test-large-int>)
