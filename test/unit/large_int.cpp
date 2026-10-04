#include "banjo/utils/large_int.hpp"

#include "unit_test.hpp"

int main(int, const char *[]) {
    using namespace banjo;

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
