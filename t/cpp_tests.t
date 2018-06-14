use strict;
use warnings;
use Test::More;
use CPP::panda::lib;

plan skip_all => 'rebuild Makefile.PL adding TEST_FULL=1 to enable all tests' unless CPP::panda::lib::Test->can('test_run_all_cpp_tests');

ok (CPP::panda::lib::Test::test_run_all_cpp_tests());
