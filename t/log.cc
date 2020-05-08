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

static inline void REGCHECK (const string& s, const string& re) {
    WARN("checking '" << s << "' with regex '" << re <<"'");
    CHECK(std::regex_search(std::string(s.c_str()), std::regex(std::string(re.c_str()))));
}

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

    SECTION("level") {
        set_format("LEVEL=%L");
        panda_log_alert();
        CHECK(c.fstr == "LEVEL=alert");
    }

    SECTION("module") {
        SECTION("default") {
            set_format("MODULE=%M");
            panda_log_alert();
            CHECK(c.fstr == "MODULE=epta");
        }
        SECTION("strip") {
            set_format(" MOD=%4.1M ");
            panda_log_alert(::panda_log_module, "");
            CHECK(c.fstr == " ");
        }
    }

    SECTION("function") {
        set_format("FUNC=%F");
        panda_log_alert();
        CHECK(c.fstr.find("FUNC=____C_A_T_C_H____T_E_S_T____") == 0);
    }

    SECTION("file") {
        SECTION("short name") {
            set_format("FILE=%f");
            panda_log_alert();
            CHECK(c.fstr == "FILE=log.cc");
        }
        SECTION("full name") {
            set_format("FILE=%1f");
            panda_log_alert();
            REGCHECK(c.fstr, "FILE=t[\\/]log.cc");
        }
    }

    SECTION("line") {
        set_format("LINE=%l");
        panda_log_alert();
        CHECK(c.fstr == "LINE=230");
    }

    SECTION("message") {
        set_format("MSG=%m");
        panda_log_alert("mymsg");
        CHECK(c.fstr == "MSG=mymsg");
    }

    SECTION("current time") {
        string pat = "TIME=";
        string re;
        SECTION("Y4 date") {
            re = "\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}";
            SECTION("low-res") {
                pat += "%t";
            }
            SECTION("hi-res") {
                pat += "%.1t";
                re += "\\.\\d{1}";
            }
        }
        SECTION("Y2 date") {
            re = "\\d{2}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}";
            SECTION("low-res") {
                pat += "%1t";
            }
            SECTION("hi-res") {
                pat += "%1.2t";
                re += "\\.\\d{2}";
            }
        }
        SECTION("hms") {
            re = "\\d{2}:\\d{2}:\\d{2}";
            SECTION("low-res") {
                pat += "%2t";
            }
            SECTION("hi-res") {
                pat += "%2.3t";
                re += "\\.\\d{3}";
            }
        }
        SECTION("unix ts") {
            re = "\\d+";
            SECTION("low-res") {
                pat += "%3t";
            }
            SECTION("hi-res") {
                pat += "%3.9t";
                re += "\\.\\d{9}";
            }
        }
        set_format(pat);
        panda_log_alert();
        REGCHECK(c.fstr, re + '$');
    }

    SECTION("thread id") {
        set_format("THREAD=%T");
        panda_log_alert();
        REGCHECK(c.fstr, "THREAD=\\d+");
    }

    SECTION("process id") {
        set_format("PID=%p");
        panda_log_alert();
        REGCHECK(c.fstr, "PID=\\d+");
    }

    SECTION("process title") {
        set_format("TITLE=%P");
        panda_log_alert();
        REGCHECK(c.fstr, "TITLE=.+");
    }
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
