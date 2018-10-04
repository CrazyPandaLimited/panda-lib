// THIS XS IS ONLY NEEDED BECAUSE TESTS FOR STRING, CONTAINERS, ETC ARE WRITTEN IN PERL, SO WE NEED ADAPTERS TO TEST IT
// IT IS ONLY COMPILED WITH TEST_FULL=1, OTHERWISE IT IS EMPTY XS
// THIS FILE AND EVERYTHING IN t/* CAN BE REMOVED WHEN PERL TESTS ARE REPLACED WITH C++ TESTS
extern "C" {
#  include "EXTERN.h"
#  include "perl.h"
#  include "XSUB.h"
#  undef do_open
#  undef do_close
}
#include <iostream>
#include <string>
#include <panda/log.h>
#undef seed
#define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_DEFAULT_REPORTER "tap"
#include <catch.hpp>
#include <catch_reporter_tap.hpp>
   
MODULE = CPP::panda::lib                PACKAGE = CPP::panda::lib::Test
PROTOTYPES: DISABLE

bool test_run_all_cpp_tests() {
    struct CatchLogger : panda::logger::ILogger {
        virtual void log (panda::logger::Level l, panda::logger::CodePoint cp, const std::string& s) override {
            if (int(l) > int(panda::logger::WARNING)) {
                FAIL(cp.to_string() << "\t" << s);
            } else {
                INFO(cp << "\t" << s);
            }
        }
    };
    panda::Log::logger().reset(new CatchLogger);

    std::vector<const char*> argv = {"test", "-s"};
    Catch::Session session;    
    RETVAL = session.applyCommandLine(argv.size(), argv.data()) == 0 && session.run() == 0;
}
