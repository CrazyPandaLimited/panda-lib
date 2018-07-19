use strict;
use warnings;
use Test::More qw/no_plan/;
use CPP::panda::lib;

my $full_tests = CPP::panda::lib::Test->can('test_run_all_cpp_tests');

if ($full_tests) {
    ok (CPP::panda::lib::Test::test_run_all_cpp_tests());
} else {
    warn "rebuild Makefile.PL adding TEST_FULL=1 to enable all tests'" unless $full_tests;
    ok 1;
}
