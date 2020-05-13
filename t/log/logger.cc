#include "logtest.h"

#define TEST(name) TEST_CASE("log-logger: " name, "[log-logger]")

TEST("set_logger") {
    Ctx c;
    Level       level;
    CodePoint   cp;
    std::string str;
    uint32_t    chk_line;
    bool        grep = false;

    SECTION("formatting callback") {
        set_logger([&](Level _level, const CodePoint& _cp, std::string& _str, const IFormatter&) {
            level = _level;
            cp    = _cp;
            str   = _str;
        });

        panda_log_alert("hello"); chk_line = __LINE__;
    }

    SECTION("simple callback") {
        grep = true;
        set_logger([&](Level _level, const CodePoint& _cp, const std::string& _str) {
            level = _level;
            cp    = _cp;
            str   = _str;
        });

        panda_log_alert("hello"); chk_line = __LINE__;
    }

    SECTION("object") {
        struct Logger : ILogger {
            Level     level;
            CodePoint cp;
            string    str;
        };
        Logger* logger;


        SECTION("formating") {
            struct Logger1 : Logger {
                void log_format (Level _level, const CodePoint& _cp, std::string& _str, const IFormatter&) override {
                    level = _level;
                    cp    = _cp;
                    str   = string(_str.data(), _str.length());
                }
            };
            logger = new Logger1();
        }

        SECTION("simple") {
            grep = true;
            struct Logger2 : Logger {
                void log (Level _level, const CodePoint& _cp, const string& _str) override {
                    level = _level;
                    cp    = _cp;
                    str   = _str;
                }
            };
            logger = new Logger2();
        }

        set_logger(logger);

        panda_log_alert("hello"); chk_line = __LINE__;

        level = logger->level;
        cp    = logger->cp;
        str   = logger->str;
    }

    if (grep) REQUIRE_THAT(str, Catch::Contains("hello"));
    else      REQUIRE(str == "hello");
    REQUIRE(level == Alert);
    REQUIRE(cp.func == __func__);
    REQUIRE(cp.file.length() > 0);
    REQUIRE(cp.line == chk_line);
    REQUIRE(cp.module == &::panda_log_module);
}

TEST("destroy old logger") {
    struct Logger : ILogger {
        int* dtor;
        void log (Level, const CodePoint&, const string&) override {}
        ~Logger () { (*dtor)++; }
    };

    int dtor = 0;
    auto logger = new Logger();
    logger->dtor = &dtor;

    set_logger(logger);
    panda_log_error("");
    REQUIRE(dtor == 0);

    auto logger2 = new Logger();
    logger2->dtor = &dtor;
    set_logger(logger2);
    REQUIRE(dtor == 1);

    set_logger(nullptr);
    REQUIRE(dtor == 2);
}
