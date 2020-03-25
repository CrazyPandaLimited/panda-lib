use strict;
use warnings;
use lib 't';
use MyTest;
use Config;
use Test::More;
use Test::Catch;

plan skip_all => 'backtrace is not supported on 32bit systems' if $Config{ptrsize} == 4;
plan skip_all => 'not available for your system' if $^O eq 'MSWin32';

catch_run('[exception]');

done_testing;
