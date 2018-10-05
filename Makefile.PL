use 5.012;
use Config;
use XS::Install;

my %params = (
    NAME          => 'CPP::panda::lib',
    VERSION_FROM  => 'lib/CPP/panda/lib.pm',
    ABSTRACT_FROM => 'lib/CPP/panda/lib.pod',
    CPLUS         => 11,
    SRC           => ['src'],
    INC           => '-Isrc',
    BIN_SHARE     => {
        CPLUS    => 11,
        INCLUDE  => {'src' => '/'},
    },
    BIN_DEPS         => ['CPP::catch'],
    CCFLAGS          => '-Wall -Wextra',
    OPTIMIZE         => '-O2 -g',
    MIN_PERL_VERSION => '5.12.0',
    XS               => $ENV{TEST_FULL} ? {'t/xs.xs' => 't/xs.cc'} : {'lib.xs' => 'lib.cc'},
);

if ($ENV{TEST_FULL}) {
    ($params{DEFINE}||='') .= ' -DTEST_FULL';
    push @{$params{SRC}||=[]}, 't';
    ($params{CCFLAGS}||='') .= ' -fsanitize=undefined';
    push @{$params{LIBS}||=[]}, '-lubsan'; #sanitizer
}

write_makefile(%params);
