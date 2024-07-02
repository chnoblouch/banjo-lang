#include "banjo/utils/large_int.hpp"

#include <cstdlib>
#include <iostream>

using namespace banjo;

template <typename R, typename E>
void check_assertion(std::string description, R result, E expected) {
    if (result != expected) {
        std::cout << "assertion failed: " << description << std::endl;
        std::cout << "    result: " << result << std::endl;
        std::cout << "  expected: " << expected << std::endl;
        std::exit(1);
    }
}

#define ASSERT_EQUAL(result, expected) check_assertion(std::string(#result) + " == " + #expected, (result), (expected))
#define ASSERT_TRUE(result) check_assertion(std::string(#result), (result), true)

int main(int argc, const char *argv[]) {
    ASSERT_EQUAL(LargeInt{"100"}.get_magnitude(), 100);
    ASSERT_TRUE(LargeInt{"100"}.is_positive());
    ASSERT_EQUAL(LargeInt{"50"}.get_magnitude(), 50);
    ASSERT_TRUE(LargeInt{"50"}.is_positive());
    ASSERT_EQUAL(LargeInt{"0"}.get_magnitude(), 0);
    ASSERT_TRUE(LargeInt{"0"}.is_positive());
    ASSERT_EQUAL(LargeInt{"-0"}.get_magnitude(), 0);
    ASSERT_TRUE(LargeInt{"-0"}.is_positive());

    ASSERT_EQUAL(LargeInt{"100"}.to_string(), "100");
    ASSERT_EQUAL(LargeInt{"-50"}.to_string(), "-50");
    ASSERT_EQUAL(LargeInt{"0"}.to_string(), "0");
    ASSERT_EQUAL(LargeInt{"-0"}.to_string(), "0");

    ASSERT_EQUAL(LargeInt{"0x5"}, LargeInt{"5"});
    ASSERT_EQUAL(LargeInt{"0xB4"}, LargeInt{"180"});
    ASSERT_EQUAL(LargeInt{"0x37CA9D"}, LargeInt{"3656349"});

    ASSERT_EQUAL(LargeInt{"80"}, LargeInt{"80"});
    ASSERT_EQUAL(LargeInt{"-20"}, LargeInt{"-20"});
    ASSERT_EQUAL(LargeInt{"0"}, LargeInt{"0"});
    ASSERT_EQUAL(LargeInt{"0"}, LargeInt{"-0"});
    ASSERT_TRUE(LargeInt{"80"} != LargeInt{"20"});
    ASSERT_TRUE(LargeInt{"-3"} != LargeInt{"3"});

    ASSERT_EQUAL(LargeInt{"50"} + LargeInt{"4"}, LargeInt{"54"});

    return 0;
}
