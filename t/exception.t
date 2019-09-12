use strict;
use warnings;
use lib 't';
use MyTest;
use Test::More;
use Test::Catch;

plan skip_all => 'not available for windows' if $^O eq 'MSWin32';

catch_run('[exception]');
