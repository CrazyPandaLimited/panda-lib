use strict;
use warnings;
use CPP::panda::lib;

my $full_tests = CPP::panda::lib::Test->can('test_run_all_cpp_tests');
if ($full_tests) {
    CPP::panda::lib::Test::test_run_all_cpp_tests();
} else {
    require Test::More;
    Test::More->import('no_plan');
    warn "rebuild Makefile.PL adding TEST_FULL=1 to enable all tests'";
    ok(1);
}
