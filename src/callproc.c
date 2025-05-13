#include "lisp.h"

void
syms_of_callproc (void)
{
  DEFVAR_LISP ("data-directory", Vdata_directory,
        doc: /* Directory of machine-independent files that come with GNU Emacs.
These are files intended for Emacs to use while it runs.  */);
}
