extern "C" {
#  include "EXTERN.h"
#  include "perl.h"
#  include "XSUB.h"
#  undef do_open
#  undef do_close
}
   
MODULE = CPP::panda::lib                PACKAGE = CPP::panda::lib
PROTOTYPES: DISABLE
