#include "hello.hpp"
#include <gtest/gtest.h>
#include <sstream>

TEST(hello, greet_writes_hello_world) {
    std::ostringstream out;
    hello::greet(out);
    EXPECT_EQ(out.str(), "hello, world\n");
}
