#include "hello.hpp"

#include <catch2/catch_test_macros.hpp>
#include <sstream>

TEST_CASE("greet writes hello world to stream") {
    std::ostringstream out;
    hello::greet(out);
    REQUIRE(out.str() == "hello, world\n");
}
