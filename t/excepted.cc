#include "test.h"
#include <panda/excepted.h>

using panda::excepted;

#define TEST(name) TEST_CASE("excepted: " name, "[excepted]")

TEST("moveable") {
    excepted<int, double> a;
    excepted<int, double> b(std::move(a));
}
