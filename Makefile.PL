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
        FILE         => 'libpanda-lib.a',
        DIR          => '.',
    },
    test             => {
        CPLUS    => 14,
        BIN_DEPS => 'Test::Catch',
        CLIB => {
            CMAKE_TARGET => 'panda-lib-tests',
            FILE => 'libpanda-lib-tests.a',
            DIR  => '.',
        },
        RECURSIVE_TEST_FILES => 1,
    },
    LIBS               => [$libs{$^O} // $libs{default}],
    CONFIGURE_REQUIRES => {'Alien::cmake3' => 0},
);

write_makefile(%params);
