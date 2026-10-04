#ifndef BANJO_TEST_UNIT_TEST_H
#define BANJO_TEST_UNIT_TEST_H

#include <cstdlib>
#include <iostream>
#include <string_view>

template <typename R, typename E>
void check_assertion(std::string_view description, R result, E expected) {
    if (result != expected) {
        std::cerr << "assertion failed: " << description << '\n';
        std::cerr << "    result: " << result << '\n';
        std::cerr << "  expected: " << expected << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define ASSERT_EQUAL(result, expected)                                                                                 \
    check_assertion(std::string{(#result)} + " == " + #expected, (result), (expected))

#define ASSERT_TRUE(result) check_assertion(std::string{(#result)}, (result), true)

#endif
