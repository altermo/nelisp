#include "lisp.h"

void
syms_of_window (void)
{
  DEFSYM (Qclone_of, "clone-of");

  DEFVAR_LISP ("window-persistent-parameters", Vwindow_persistent_parameters,
               doc: /* Alist of persistent window parameters.
This alist specifies which window parameters shall get saved by
`current-window-configuration' and `window-state-get' and subsequently
restored to their previous values by `set-window-configuration' and
`window-state-put'.

The car of each entry of this alist is the symbol specifying the
parameter.  The cdr is one of the following:

nil means the parameter is neither saved by `window-state-get' nor by
`current-window-configuration'.

t means the parameter is saved by `current-window-configuration' and,
provided its WRITABLE argument is nil, by `window-state-get'.

The symbol `writable' means the parameter is saved unconditionally by
both `current-window-configuration' and `window-state-get'.  Do not use
this value for parameters without read syntax (like windows or frames).

Parameters not saved by `current-window-configuration' or
`window-state-get' are left alone by `set-window-configuration'
respectively are not installed by `window-state-put'.  */);
  Vwindow_persistent_parameters = list1 (Fcons (Qclone_of, Qt));
}
