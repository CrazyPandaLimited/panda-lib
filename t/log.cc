#include "test.h"
#include <panda/log.h>
#include <regex>

#define TEST(name) TEST_CASE("log: " name, "[log]")

using namespace panda::log;

struct Ctx {
    int       cnt = 0;
    Level     level;
    CodePoint cp;
    string    str;
    string    fstr;

    Ctx () {
        set_logger([this](Level _level, const CodePoint& _cp, std::string& _str, const IFormatter& formatter) {
            level = _level;
            cp    = _cp;
            str   = string(_str.data(), _str.length());
            fstr  = formatter.format(level, cp, _str);
            ++cnt;
        });
        set_level(Warning);
    }

    void check_called () {
        REQUIRE(cnt == 1);
        cnt = 0;
    }

    ~Ctx () {
        set_logger(nullptr);
    }
};

TEST("set_level") {
    Ctx c;
    set_level(VerboseDebug);
    panda_log_verbose_debug("");
    c.check_called();
    panda_log_critical("");
    c.check_called();

    set_level(Debug);
    panda_log_verbose_debug("");
    REQUIRE(c.cnt == 0);
    panda_log_debug("");
    c.check_called();
}

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

TEST("set_formatter") {
    Ctx c;
    string str;

    set_logger([&str](Level, const CodePoint&, const string& s) {
        str = s;
    });

    SECTION("callback") {
        set_formatter([](Level, const CodePoint&, std::string&) -> string {
            return "jopa";
        });
    }
    SECTION("object") {
        struct Formatter : IFormatter {
            string format (Level, const CodePoint&, std::string&) const override {
                return "jopa";
            }
        };
        set_formatter(new Formatter());
    }

    panda_log_alert("hello");
    REQUIRE(str == "jopa");
}

TEST("set_format") {
    Ctx c;
    Module panda_log_module("epta");
    set_format("LEVEL=%L FILE=%f LINE=%l FUNC=%F MODULE=%M MESSAGE=%m DATE=%d DATEH=%D TIME=%t TIMEH=%T PID=%p THREAD=%P UNIX=%u UNIXH=%U ");
    panda_log_alert("mymsg");
    WARN(c.fstr);
    auto res = std::regex_search(std::string(c.fstr.c_str()), std::regex(
        "LEVEL=alert FILE=log.cc LINE=185 FUNC=\\w+ MODULE=epta MESSAGE=mymsg "
        "DATE=\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2} "
        "DATEH=\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}(\\.\\d+)? "
        "TIME=\\d{2}:\\d{2}:\\d{2} "
        "TIMEH=\\d{2}:\\d{2}:\\d{2}(\\.\\d+)? "
        "PID=\\d+ "
        "THREAD=\\w+ "
        "UNIX=\\d+ "
        "UNIXH=\\d+(\\.\\d+)? "
    ));
    CHECK(res);
}

TEST("should_log") {
    Ctx c;
    set_level(Debug);
    REQUIRE_FALSE(panda_should_log(VerboseDebug));
    REQUIRE(panda_should_log(Debug));
    set_level(Error);
    REQUIRE_FALSE(panda_should_log(Debug));
    REQUIRE(panda_should_log(Critical));
}

TEST("streaming params") {
    Ctx c;
    panda_log_warning("1" << "2" << "3");
    CHECK(c.str == "123");
}

TEST("code-eval logging") {
    Ctx c;
    bool val = false;

    panda_log_debug([&]{
        val = true;
    });

    REQUIRE_FALSE(val);

    panda_log_warning([&]{
        log << "text";
        val = true;
    });
    CHECK(c.str == "text");

    panda_log(Error, [&]{
        log << "hello";
    });
    CHECK(c.str == "hello");

    REQUIRE(val);
}

TEST("modules") {
    SECTION("single") {
        auto mod = new Module("mymod");
        CHECK(mod->name == "mymod");
        set_level(Debug, "mymod");
        delete mod;
        CHECK_THROWS(set_level(Debug, "mymod"));
    }
    SECTION("with submodule") {
        auto mod = new Module("mymod");
        Module smod("sub", mod);
        CHECK(mod->name == "mymod");
        CHECK(smod.name == "mymod::sub");
        CHECK(mod->children.size() == 1);

        delete mod;
        CHECK(smod.parent == nullptr); // became a root module
        CHECK(smod.name == "mymod::sub"); // name didn't change
        CHECK_THROWS(set_level(Debug, "mymod"));
        set_level(Debug, "mymod::sub"); // submodule still can be used
    }
}

TEST("logging to module") {
    Ctx c;
    static Module mod("mymod1");
    CHECK(mod.name == "mymod1");
    mod.level = Debug;

    panda_log_verbose_debug("hi");
    CHECK(c.cnt == 0);
    panda_log_verbose_debug(mod, "hi");
    CHECK(c.cnt == 0);

    panda_log_debug("hi");
    CHECK(c.cnt == 0);
    panda_log_debug(mod, "hi");
    c.check_called();
    CHECK(c.cp.module == &mod);

    panda_log_warning("hi");
    c.check_called();
    CHECK(c.cp.module == &panda_log_module);
    panda_log_warning(mod, "hi");
    c.check_called();
    CHECK(c.cp.module == &mod);

    mod.level = Notice;

    panda_log_debug("hi");
    CHECK(c.cnt == 0);
    panda_log_debug(mod, "hi");
    CHECK(c.cnt == 0);
}

TEST("set level for module") {
    Ctx c;
    static Module mod("mymod2");
    static Module submod("submod2", &mod);
    mod.level = Warning;
    submod.level = Warning;

    SECTION("parent affects all children") {
        panda_log_module.set_level(Info);
        CHECK(panda_log_module.level == Info);
        CHECK(mod.level == Info);
        CHECK(submod.level == Info);

        //this is the same
        set_level(Notice);
        CHECK(panda_log_module.level == Notice);
        CHECK(mod.level == Notice);
        CHECK(submod.level == Notice);
    }

    SECTION("children do not affect parents") {
        mod.set_level(Debug);
        CHECK(panda_log_module.level == Warning);
        CHECK(mod.level == Debug);
        CHECK(submod.level == Debug);
    }

    SECTION("setting via module's name") {
        set_level(Error, "mymod2");
        CHECK(panda_log_module.level == Warning);
        CHECK(mod.level == Error);
        CHECK(submod.level == Error);
        set_level(Critical, "mymod2::submod2");
        CHECK(panda_log_module.level == Warning);
        CHECK(mod.level == Error);
        CHECK(submod.level == Critical);
    }
}

TEST("secondary root module") {
    Ctx c;
    static Module rmod("rmod", nullptr);
    static Module rsmod("rsmod", &rmod);
    CHECK(rmod.parent == nullptr);
    rmod.level = Info;
    rsmod.level = Info;

    set_level(Debug);
    CHECK(panda_log_module.level == Debug);
    CHECK(rmod.level == Info);
    CHECK(rsmod.level == Info);

    rmod.set_level(Warning);
    CHECK(panda_log_module.level == Debug);
    CHECK(rmod.level == Warning);
    CHECK(rsmod.level == Warning);
}

TEST("logging by scopes") {
    Ctx c;

    panda_log_error("");
    CHECK(c.cp.module == &::panda_log_module);

    static Module panda_log_module("scope1");
    panda_log_error("");
    CHECK(c.cp.module->name == "scope1");

    {
        panda_log_error("");
        CHECK(c.cp.module->name == "scope1");

        static Module panda_log_module("scope2");
        panda_log_error("");
        CHECK(c.cp.module->name == "scope2");
    }
}

TEST("empty log") {
    Ctx c;
    SECTION("case 1") {
        panda_log(Error);
    }
    SECTION("case 2") {
        panda_log_error();
    }
    c.check_called();
    CHECK(c.str == "==> MARK <==");
}

TEST("panda_rlog_*") {
    Ctx c;
    static Module panda_log_module("non-root");
    panda_rlog_error("");
    CHECK(c.cp.module == &::panda_log_module);

    //panda_
}
