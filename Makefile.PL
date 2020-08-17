use 5.012;
use Config;
use XS::Install;

my %libs = (
    MSWin32 => '-lexecinfo -ldbgeng -lole32',
    linux   => '',
    freebsd => '-lexecinfo',
    default => '-lexecinfo',
);


my %params = (
    NAME             => 'XS::libpanda',
    CPLUS            => 14,
    MIN_PERL_VERSION => '5.12.0',
    CLIB => {
        CMAKE_TARGET => 'panda-lib',
        CMAKE_TEST   => 'panda-lib-tests',
        CMAKE_TEST_FILE   => 'libpanda-lib-tests.a',
        FILE         => 'libpanda-lib.a',
        DIR          => '.',
    },
    test             => {
        INC => '-Isrc',
        SRC => 't',
        BIN_DEPS => 'Test::Catch',
        RECURSIVE_TEST_FILES => 1,
        CLIB => {
            CMAKE_TARGET => 'panda-lib-tests',
            FILE   => 'libpanda-lib-tests.a',
            DIR => '.',
        },
    },
    LIBS             => [$libs{$^O} // $libs{default}],
);

if ($ENV{SANITIZE}) {
    ($params{CCFLAGS}||='') .= ' -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer';
    $params{LIBS}[1] .= ' -lubsan -lasan'; #sanitizer
    $params{LIBS}[0] .= ' -lubsan -lasan'; #sanitizer
}

write_makefile(%params);
