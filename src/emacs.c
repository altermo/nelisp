#ifndef EMACS_C
#define EMACS_C

#include "lisp.h"

void
syms_of_emacs (void)
{
    DEFVAR_LISP ("dump-mode", Vdump_mode,
                 doc: /* Non-nil when Emacs is dumping itself.  */);
}

#endif
