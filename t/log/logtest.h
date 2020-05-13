#include <catch.hpp>
#include <panda/log.h>

using namespace panda;
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
