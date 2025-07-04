#include "lisp.h"

void
init_macros (void)
{
  Vexecuting_kbd_macro = Qnil;
}

void
syms_of_macros (void)
{
  DEFVAR_LISP ("executing-kbd-macro", Vexecuting_kbd_macro,
        doc: /* Currently executing keyboard macro (string or vector).
This is nil when not executing a keyboard macro.  */);
}
