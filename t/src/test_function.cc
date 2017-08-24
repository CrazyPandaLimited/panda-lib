#include "catch.hpp"
#include "test_utils.h"
#include <panda/function.h>
#include <panda/refcnt.h>
#include <panda/string.h>

using panda::function;
using panda::make_function;
using panda::make_shared;
using panda::make_method;

namespace test {

void void_func(){}
void void_func2(){}

int Tracer::copy_calls = 0;
int Tracer::ctor_calls = 0;
int Tracer::move_calls = 0;
int Tracer::dtor_calls = 0;


int foo2() {return 1;};

class Test {
public:
    int value = 0;
    void foo(int) {}
    void foo2(int) {}
    int bar() {return value + 40;}

    int operator()(int v) {return v;}
    //bool operator == (const Test& oth) const { return false;}
};
}

using namespace test;

TEST_CASE("simplest function", "[function]") {
    function<void(void)> f = &void_func;
    REQUIRE(true);
}

TEST_CASE("simplest function call", "[function]") {
    function<int(void)> f = &foo2;
    REQUIRE(f() == 1);
}

TEST_CASE("simplest lambda call", "[function]") {
    int a = 13;
    function<int(void)> f = [&](){return a;};
    REQUIRE(f() == 13);
}

TEST_CASE("simplest method call", "[function]") {
    auto t = make_shared<Test>();
    t->value = 14;
    auto m = make_function(&Test::bar, t);
    REQUIRE(m() == 54);
}

TEST_CASE("mixedcall", "[function]") {
    auto t = make_shared<Test>();
    t->value = 14;
    auto f = make_function(&Test::bar, t);
    REQUIRE(f() == 54);

    f = &foo2;
    REQUIRE(f() == 1);

    int a = 13;
    f = [&](){return a;};
    REQUIRE(f() == 13);
}

TEST_CASE("function ptr comparations", "[function]") {
    function<void(void)> f1_void = &void_func;
    function<void(void)> f2_void = &void_func;
    function<void(void)> f3_void = &void_func2;

    REQUIRE(f1_void == f2_void);
    REQUIRE(f1_void != f3_void);
}

TEST_CASE("methods comparations", "[function]") {
    auto t = make_shared<Test>();
    auto m1 = make_function(&Test::foo, t);
    auto m2 = make_method(&Test::foo);
    REQUIRE(m1 != m2);

    m2->bind(t);
    REQUIRE(m1 == m2);

    auto t2 = make_shared<Test>();
    m2->bind(t2);
    REQUIRE(m1 != m2);

    auto m3 = make_method(&Test::foo2);
    REQUIRE(m1 != m3);

}

TEST_CASE("lambdas comparations", "[function]") {
    int a = 10;
    function<int(void)> l1 = [&](){return a;};
    auto l2 = l1;
    function<int(void)> l3 = [&](){return a;};

    REQUIRE(l1 == l2);
    REQUIRE(l1 != l3);
}

TEST_CASE("mixed function comparations", "[function]") {
    int a = 10;
    function<int(void)> l = [&](){return a;};
    function<int(void)> f = &foo2;
    auto t = make_shared<Test>();
    auto m = make_function(&Test::bar, t);

    REQUIRE(l != f);
    REQUIRE(m != l);
    REQUIRE(m != f);
}

TEST_CASE("function copy ellision", "[function]") {
    Tracer::refresh();
    {
        function<int(int)> f = Tracer(10);
        auto f2 = f;
        f(11);
        f2(12);
    }
    REQUIRE(Tracer::ctor_calls == 1); // 1 for temporary object Tracer(10);
    REQUIRE(Tracer::copy_calls == 0);
    REQUIRE(Tracer::move_calls == 1); // 1 construction from tmp object function<int(int)> f = Tracer(10);
    REQUIRE(Tracer::dtor_calls == 2);
}

TEST_CASE("covariant return type optional" , "[function]") {
    function<panda::optional<int> (int)> cb = [](int a) -> int {
        return a;
    };
    REQUIRE(cb(3).value_or(42) == 3);
}

TEST_CASE("covariant return type double" , "[function]") {
    function<double (int)> cb = [](int a) -> int {
        return a;
    };
    REQUIRE(cb(3) == 3.0);
}

TEST_CASE("conervariant arguments" , "[function]") {
    function<double (int)> cb = [](double) -> int {
        return 10;
    };
    REQUIRE(cb(3) == 10);
}

TEST_CASE("conervariant arguments classes" , "[function]") {
    using panda::string;
    struct Base {
        virtual ~Base(){}
        virtual string name() { return "base";}
    };
    struct Derrived : Base {
        virtual string name() override { return "override";}
    };
    function<Base& (Derrived&)> cb = [](Base& b) -> Base& {
        return b;
    };
    Derrived b;
    REQUIRE(cb(b).name() == b.name());
}
