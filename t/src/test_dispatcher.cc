#include "catch.hpp"
#include "test_utils.h"
#include <panda/CallbackDispatcher.h>

using panda::CallbackDispatcher;
using test::Tracer;

using Dispatcher = CallbackDispatcher<int(int)>;
using Event = Dispatcher::Event;

using panda::function;

TEST_CASE("empty callback dispatcher" , "[CallbackDispatcher]") {
    Dispatcher d;
    d(1);
    REQUIRE(true);
}

TEST_CASE("simplest callback dispatcher" , "[CallbackDispatcher]") {
    Dispatcher d;
    function<panda::optional<int> (Dispatcher::Event&, int)> cb = [](Event& e, int a) -> int {
        return 1 + e.next(a).value_or(0);
    };
    d.add(cb);
    d.add([](Event& e, int a) -> int {
        return a + e.next(a).value_or(0);
    });
    REQUIRE(d(2).value_or(0) == 3);
}

TEST_CASE("remove callback dispatcher" , "[CallbackDispatcher]") {
    Dispatcher d;
    d.add([](Event& e, int a) -> int {
        return 1 + e.next(a).value_or(0);
    });
    Dispatcher::Callback c = [](Event& e, int a) -> int {
        return a + e.next(a).value_or(0);
    };
    d.add(c);
    REQUIRE(d(2).value_or(0) == 3);
    d.remove(c);
    REQUIRE(d(2).value_or(0) == 1);
}

TEST_CASE("remove_all in process" , "[CallbackDispatcher]") {
    Dispatcher d;
    d.add([&](Event& e, int a) -> int {
        d.remove_all();
        return 1 + e.next(a).value_or(0);
    });
    d.add([](Event&, int) -> int {
        return 2;
    });
    REQUIRE(d(2).value_or(0) == 1);
}

TEST_CASE("callback dispatcher copy ellision" , "[CallbackDispatcher]") {
    Dispatcher d;
    Tracer::refresh();
    {
        Dispatcher::Callback cb = Tracer(14);
        d.add(cb);
        REQUIRE(d(2).value_or(0) == 16);
        d.remove(cb);
        REQUIRE(d(2).value_or(0) == 0);
    }
    REQUIRE(d(2).value_or(0) == 0);

    REQUIRE(Tracer::ctor_calls == 1); // 1 for temporary object Tracer(10);
    REQUIRE(Tracer::copy_calls == 0);
    REQUIRE(Tracer::move_calls == 1); // 1 construction from tmp object function<int(int)> f = Tracer(10);
    REQUIRE(Tracer::dtor_calls == 2);
}

TEST_CASE("callback dispatcher without event" , "[CallbackDispatcher]") {
    Dispatcher d;
    Dispatcher::SimpleCallback s = [](int a) {
        return a;
    };
    d.add(s);
    REQUIRE(d(2).value_or(42) == 42);
}

