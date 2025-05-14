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
}
