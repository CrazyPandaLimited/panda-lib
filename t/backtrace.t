use strict;
use warnings;

BEGIN {
    return printf("1..0 # available for linux only") unless $^O eq 'linux';
    require lib; lib->import('t');
    require MyTest;
    require Test::Catch; Test::Catch->import('[backtrace]');
}
