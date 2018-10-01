#include "test.h"
#include <panda/CallbackDispatcher.h>
#include <panda/function_utils.h>
#include <panda/lib/from_chars.h>
#include <panda/string.h>

using panda::CallbackDispatcher;
using test::Tracer;
using panda::function_details::tmp_abstract_function;
using panda::function_details::make_abstract_function;
using panda::string;

using Dispatcher = CallbackDispatcher<int(int)>;
using Event = Dispatcher::Event;

using panda::function;

TEST_CASE("empty callback dispatcher" , "[panda-lib][callbackdispatcher]") {
    Dispatcher d;
    d(1);
    REQUIRE(true);
}

TEST_CASE("dispatcher void()" , "[panda-lib][callbackdispatcher]") {
    CallbackDispatcher<void()> d;
    bool called = false;
    CallbackDispatcher<void()>::SimpleCallback f = [&](){called = true;};
    d.add(f);
    d();
    REQUIRE(called);

}

TEST_CASE("simplest callback dispatcher" , "[panda-lib][callbackdispatcher]") {
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

TEST_CASE("remove callback dispatcher" , "[panda-lib][callbackdispatcher]") {
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

TEST_CASE("remove_all in process" , "[panda-lib][callbackdispatcher]") {
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

TEST_CASE("callback dispatcher copy ellision" , "[panda-lib][callbackdispatcher]") {
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

TEST_CASE("callback dispatcher without event" , "[panda-lib][callbackdispatcher]") {
    Dispatcher d;
    bool called = false;
    Dispatcher::SimpleCallback s = [&](int a) {
        called = true;
        return a;
    };
    d.add(s);
    REQUIRE(d(2).value_or(42) == 42);
    REQUIRE(called);
}

TEST_CASE("remove callback dispatcher without event" , "[panda-lib][callbackdispatcher]") {
    Dispatcher d;
    bool called = false;
    Dispatcher::SimpleCallback s = [&](int a) {
        called = true;
        return a;
    };
    d.add(s);
    REQUIRE(d(2).value_or(42) == 42);
    REQUIRE(called);
    d.remove(s);
    called = false;
    REQUIRE(d(2).value_or(42) == 42);
    REQUIRE(!called);
}

TEST_CASE("remove callback dispatcher with compatible type" , "[panda-lib][callbackdispatcher]") {
    Dispatcher d;
    bool called = false;
    function<int(int16_t)> s = [&](int a) {
        called = true;
        return a;
    };
    d.add(s);
    REQUIRE(d(2).value_or(42) == 42);
    REQUIRE(called);
    d.remove(s);
    called = false;
    REQUIRE(d(2).value_or(42) == 42);
    REQUIRE(!called);
}

TEST_CASE("remove callback comparable functor" , "[panda-lib][callbackdispatcher]") {
    Dispatcher d;
    static bool called;
    struct S {
        int operator()(int a) {
            called = true;
            return a +10;
        }
        bool operator ==(const S&) const {
            return true;
        }
    };

    static_assert(panda::lib::traits::has_call_operator<S, int>::value,
                  "S shuld be callable, it can be wrong implementation of panda::has_call_operator or a compiler error");

    S src;
    called = false;
    Dispatcher::SimpleCallback s = src;
    d.add(s);
    CHECK(d(2).value_or(42) == 42);
    CHECK(called);
    d.remove(s);
    called = false;
    CHECK(d(2).value_or(42) == 42);
    CHECK(!called);

    called = false;
    d.add(s);
    CHECK(d(2).value_or(42) == 42);
    CHECK(called);

    d.remove_object(S(src));
    called = false;
    CHECK(d(2).value_or(42) == 42);
    CHECK(!called);
}

TEST_CASE("remove callback comparable full functor" , "[panda-lib][callbackdispatcher]") {
    Dispatcher d;
    static bool called;
    struct S {
        int operator()(Dispatcher::Event& e, int a) {
            called = true;
            e.next(a);
            return a + 10;
        }
        bool operator ==(const S&) const {
            return true;
        }
    };

    static_assert(panda::lib::traits::has_call_operator<S,Dispatcher::Event&, int>::value,
                  "S shuld be callable, it can be wrong implementation of panda::has_call_operator or a compiler error");

    S src;
    called = false;
    Dispatcher::Callback s = src;
    d.add(s);

    CHECK(d(2).value_or(42) == 12);
    CHECK(called);
    called = false;

    SECTION("standatd remove") {
        d.remove(s);
    }

    SECTION("fast remove") {
        d.remove_object(S(src));
    }

    CHECK(d(2).value_or(42) == 42);
    CHECK(!called);
}

TEST_CASE("remove simple callback self lambda" , "[panda-lib][callbackdispatcher]") {
    Dispatcher d;
    static bool called;
    auto l = [&](panda::Ifunction<int, int>& self, int a) {
        d.remove(self);
        called = true;
        return a + 10;
    };

    Dispatcher::SimpleCallback s = l;
    d.add(s);
    CHECK(d(2).value_or(42) == 42);
    CHECK(called);
    called = false;
    CHECK(d(2).value_or(42) == 42);
    CHECK(!called);
}


TEST_CASE("remove callback self lambda" , "[panda-lib][callbackdispatcher]") {
    using panda::optional;
    Dispatcher d;
    static bool called;
    auto l = [&](panda::Ifunction<optional<int>, Dispatcher::Event&, int>& self, Dispatcher::Event&, int a) {
        d.remove(self);
        called = true;
        return a + 10;
    };

    Dispatcher::Callback s = l;
    d.add(s);
    CHECK(d(2).value_or(42) == 12);
    CHECK(called);
    called = false;
    CHECK(d(2).value_or(42) == 42);
    CHECK(!called);
}

TEST_CASE("dispatcher to function conversion" , "[panda-lib][callbackdispatcher]") {
    Dispatcher d;
    d.add([](Dispatcher::Event&, int a){return a*2;});
    function<panda::optional<int>(int)> f = d;
    REQUIRE(f(10).value_or(0) == 20);

}

TEST_CASE("dispatcher 2 string calls" , "[panda-lib][callbackdispatcher]") {
    using Dispatcher = CallbackDispatcher<void(string)>;
    Dispatcher d;
    d.add([](string s){REQUIRE(s == "value");});
    d.add([](string s){REQUIRE(s == "value");});
    d("value");
}
