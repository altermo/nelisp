#include "lisp.h"

void
syms_of_display (void)
{
  DEFSYM (Qdisplay_table, "display-table");

  DEFVAR_LISP ("standard-display-table", Vstandard_display_table,
        doc: /* Display table to use for buffers that specify none.
It is also used for standard output and error streams.
See `buffer-display-table' for more information.  */);
  Vstandard_display_table = Qnil;

#if TODO_NELISP_LATER_AND
  DEFVAR_KBOARD ("window-system", Vwindow_system,
    doc: /* Name of window system through which the selected frame is displayed.
The value is a symbol:
 nil for a termcap frame (a character-only terminal),
 `x' for an Emacs frame that is really an X window,
 `w32' for an Emacs frame that is a window on MS-Windows display,
 `ns' for an Emacs frame on a GNUstep or Macintosh Cocoa display,
 `pc' for a direct-write MS-DOS frame.
 `pgtk' for an Emacs frame using pure GTK facilities.
 `haiku' for an Emacs frame running in Haiku.
 `android' for an Emacs frame running in Android.

Use of this variable as a boolean is deprecated.  Instead,
use `display-graphic-p' or any of the other `display-*-p'
predicates which report frame's specific UI-related capabilities.  */);
#else
  DEFVAR_LISP("window-system", Vwindow_system,
    doc: /* */);
  Vwindow_system = Qnil;
#endif
}
