#include <catch.hpp>
#include <panda/lib/traits.h>
#include <panda/string.h>

using namespace panda::lib::traits;
using namespace panda;

TEST_CASE("bool_or", "[panda-lib][traits]") {
    struct N {};

    REQUIRE(bool_or(nullptr, true) == false);
    REQUIRE(bool_or(1, false) == true);
    REQUIRE(bool_or(string(""), true) == false);
    REQUIRE(bool_or(string("1"), false) == true);

    REQUIRE(bool_or(N{}, true) == true);
    REQUIRE(bool_or(N{}, false) == false);

    int a = 1;
    int& ref = a;
    REQUIRE(bool_or(ref, false) == true);

    string s;
    string& sref = s;
    REQUIRE(bool_or(sref, true) == false);
    s = "123";
    REQUIRE(bool_or(sref, false) == true);

    N n;
    N& nref = n;
    REQUIRE(bool_or(nref, true) == true);
    REQUIRE(bool_or(nref, false) == false);
}
