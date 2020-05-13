#include "logtest.h"
#include <panda/log/multi.h>

#define TEST(name) TEST_CASE("log-multi: " name, "[log-multi]")

TEST("log to multi channels") {
    Ctx c;
    int cnt = 0;
    set_format("%m");
    set_logger(new MultiLogger({
        {
            fn2logger([&](Level l, const CodePoint& cp, const string& msg) {
                CHECK(l == Critical);
                CHECK(cp.line == 28);
                CHECK(msg == "hi");
                ++cnt;
            })
        },
        {
            fn2logger([&](Level l, const CodePoint& cp, const string& msg) {
                CHECK(l == Critical);
                CHECK(cp.line == 28);
                CHECK(msg == "hi");
                ++cnt;
            })
        }
    }));
    panda_log_critical("hi");
    CHECK(cnt == 2);
}

TEST("using min_level") {
    Ctx c;
    int cnt = 0;
    set_logger(new MultiLogger({
        { fn2logger([&](Level, const CodePoint&, const string&) { cnt += 1; }), Notice },
        { fn2logger([&](Level, const CodePoint&, const string&) { cnt += 100; }), Error },
        { fn2logger([&](Level, const CodePoint&, const string&) { cnt += 10; }), Warning },
    }));
    panda_log_warning("hi");
    CHECK(cnt == 11);
}

TEST("using different formatters") {
    Ctx c;
    set_format("F1:%m");
    string m1,m2,m3;
    set_logger(new MultiLogger({
        { fn2logger([&](Level, const CodePoint&, const string& m) { m1=m; }), new PatternFormatter("F2:%m"), Debug },
        { fn2logger([&](Level, const CodePoint&, const string& m) { m2=m; }), new PatternFormatter("F3:%m") },
        { fn2logger([&](Level, const CodePoint&, const string& m) { m3=m; }) },
    }));
    panda_log_error("hi");
    CHECK(m1 == "F2:hi");
    CHECK(m2 == "F3:hi");
    CHECK(m3 == "F1:hi");
}
