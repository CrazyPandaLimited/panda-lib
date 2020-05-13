#include "logtest.h"

#define TEST(name) TEST_CASE("log-module: " name, "[log-module]")

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

TEST("panda_rlog_*") {
    Ctx c;
    static Module panda_log_module("non-root");
    panda_rlog_error("");
    CHECK(c.cp.module == &::panda_log_module);

    //panda_
}
