#include "test.h"
#include <panda/CallbackDispatcher.h>
#include <panda/function_utils.h>
#include <panda/lib/from_chars.h>

using panda::CallbackDispatcher;
using test::Tracer;
using panda::function_details::tmp_abstract_function;
using panda::function_details::make_abstract_function;
using panda::iptr;

using Dispatcher = CallbackDispatcher<int(int)>;
using Event = Dispatcher::Event;

using panda::function;
using panda::string;

TEST_CASE("empty callback dispatcher" , "[CallbackDispatcher]") {
    Dispatcher d;
    d(1);
    REQUIRE(true);
}

TEST_CASE("dispatcher void()" , "[CallbackDispatcher]") {
    CallbackDispatcher<void()> d;
    bool called = false;
    CallbackDispatcher<void()>::SimpleCallback f = [&](){called = true;};
    d.add(f);
    d();
    REQUIRE(called);

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
    d.add([](Event&, int) -> int {
        return 2;
    });
    d.add([&](Event& e, int a) -> int {
        d.remove_all();
        return 1 + e.next(a).value_or(0);
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
    bool called = false;
    Dispatcher::SimpleCallback s = [&](int) {
        called = true;
    };
    d.add(s);
    REQUIRE(d(2).value_or(42) == 42);
    REQUIRE(called);
}

TEST_CASE("remove callback dispatcher without event" , "[CallbackDispatcher]") {
    Dispatcher d;
    bool called = false;
    Dispatcher::SimpleCallback s = [&](int) {
        called = true;
    };
    d.add(s);
    REQUIRE(d(2).value_or(42) == 42);
    REQUIRE(called);
    d.remove(s);
    called = false;
    REQUIRE(d(2).value_or(42) == 42);
    REQUIRE(!called);
}

TEST_CASE("remove callback comparable functor" , "[CallbackDispatcher]") {
    Dispatcher d;
    static bool called;
    struct S {
        void operator()(int) {
            called = true;
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

TEST_CASE("remove callback comparable full functor" , "[CallbackDispatcher]") {
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

TEST_CASE("remove simple callback self lambda" , "[CallbackDispatcher]") {
    Dispatcher d;
    static bool called;
    auto l = [&](panda::Ifunction<void, int>& self, int) {
        d.remove(self);
        called = true;
    };

    Dispatcher::SimpleCallback s = l;
    d.add(s);
    CHECK(d(2).value_or(42) == 42);
    CHECK(called);
    called = false;
    CHECK(d(2).value_or(42) == 42);
    CHECK(!called);
}


TEST_CASE("remove callback self lambda" , "[CallbackDispatcher]") {
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

TEST_CASE("dispatcher to function conversion" , "[CallbackDispatcher]") {
    Dispatcher d;
    d.add([](Dispatcher::Event&, int a){return a*2;});
    function<panda::optional<int>(int)> f = d;
    REQUIRE(f(10).value_or(0) == 20);

}

TEST_CASE("dispatcher 2 string calls" , "[CallbackDispatcher]") {
    using Dispatcher = CallbackDispatcher<void(string)>;
    Dispatcher d;
    d.add([](string s) {CHECK(s == "value");});
    d.add([](string s) {CHECK(s == "value");});
    d(string("value"));
    string s = "value";
    d(s);
}

TEST_CASE("front order", "[CallbackDispatcher]") {
    using Dispatcher = CallbackDispatcher<void()>;
    Dispatcher d;
    std::vector<int> res;
    d.add([&]{ res.push_back(1); });
    d.add([&]{ res.push_back(2); });
    d.add([&](Dispatcher::Event& e){ res.push_back(3); e.next(); });
    d();
    REQUIRE(res == std::vector<int>({3,2,1}));
}

TEST_CASE("back order", "[CallbackDispatcher]") {
    using Dispatcher = CallbackDispatcher<void()>;
    Dispatcher d;
    std::vector<int> res;
    d.add([&](Dispatcher::Event& e){ res.push_back(1); e.next(); }, true);
    d.add([&]{ res.push_back(2); }, true);
    d.add_back([&]{ res.push_back(3); });
    d();
    REQUIRE(res == std::vector<int>({1,2,3}));
}

TEST_CASE("dispatcher const ref arg move" , "[CallbackDispatcher]") {
    struct S : public panda::Refcnt {
        using Dispatcher = CallbackDispatcher<void(const iptr<S>&)>;
        Dispatcher d;

        void call() {
            d(this);
        }

    };
    S s;
    s.call();
}
