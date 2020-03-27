use strict;
use warnings;
use lib 't';
use MyTest;
use Config;
use Test::More;
use Test::Catch;

# windows 32bit: gdbeng is n/a for mingw32
#    *bsd 32bit: libunwind: EHHeaderParser::decodeTableEntry: bad fde: CIE ID is not zero
# linux: should be ok
plan skip_all => 'backtrace is not supported on 32bit systems' if ($Config{ptrsize} == 4) && ($^O =~ /(MSWin32)|(bsd)/);

catch_run('[exception]');

done_testing;
