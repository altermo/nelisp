#include "lisp.h"

void
syms_of_emacs (void)
{
  DEFSYM (Qrisky_local_variable, "risky-local-variable");

  DEFVAR_LISP ("command-line-args", Vcommand_line_args,
                 doc: /* Args passed by shell to Emacs, as a list of strings.
Many arguments are deleted from the list as they are processed.  */);

  DEFVAR_LISP ("dump-mode", Vdump_mode,
                 doc: /* Non-nil when Emacs is dumping itself.  */);
}
