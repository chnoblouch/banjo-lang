add_executable(test-x86_64-encoder x86_64_encoder.cpp)
target_include_directories(test-x86_64-encoder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test-x86_64-encoder PRIVATE banjo)
add_test(NAME x86_64_encoder COMMAND $<TARGET_FILE:test-x86_64-encoder>)

add_executable(test-large-int large_int.cpp)
target_include_directories(test-large-int PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test-large-int PRIVATE banjo)
add_test(NAME large_int COMMAND $<TARGET_FILE:test-large-int>)
