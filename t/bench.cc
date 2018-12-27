#include "test.h"

struct Base {
    int x;
    virtual ~Base() {}
};

struct Base1 : virtual Base {
    int a;
};

struct Base2 : virtual Base1 {
    int b;
};

struct Der : virtual Base2 {
    int c;
};

struct ABC {
    int ttt;
    virtual ~ABC() {}
};

struct Epta : virtual Der, virtual ABC {
    int erc;
};

struct Wrong {
    virtual ~Wrong() {}
};


Base* get_suka () { return new Epta(); }

TEST_CASE("dyn_cast", "[!benchmark]") {
    using panda::dyn_cast;

    Base* b = get_suka();
    uint64_t res = 0;
    BENCHMARK("dyn_cast") {
        for (size_t i = 0; i < 1000000; i++) {
            res += (uint64_t)dyn_cast<Base1*>(b);
        }
    }
    WARN(res);
}
