add_executable(pass-tester pass_tester.cpp main.cpp)
target_include_directories(pass-tester PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pass-tester PRIVATE banjo test-utils)

set(tests_dir "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_compile_definitions(pass-tester PRIVATE TESTS_DIR="${tests_dir}")

file(GLOB passes RELATIVE "${tests_dir}" "${tests_dir}/*")

foreach(pass ${passes})
    set(pass_dir "${tests_dir}/${pass}")
    file(GLOB tests RELATIVE "${pass_dir}" "${pass_dir}/*")

    foreach(test ${tests})
        get_filename_component(test_name "${test}" NAME_WLE)
        add_test(NAME "${pass}.${test_name}" COMMAND $<TARGET_FILE:pass-tester> "${pass}" "${test}")
    endforeach()
endforeach()
