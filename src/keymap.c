#include "lisp.h"

DEFUN ("make-keymap", Fmake_keymap, Smake_keymap, 0, 1, 0,
       doc: /* Construct and return a new keymap, of the form (keymap CHARTABLE . ALIST).
CHARTABLE is a char-table that holds the bindings for all characters
without modifiers.  All entries in it are initially nil, meaning
"command undefined".  ALIST is an assoc-list which holds bindings for
function keys, mouse events, and any other things that appear in the
input stream.  Initially, ALIST is nil.

The optional arg STRING supplies a menu name for the keymap
in case you use it as a menu with `x-popup-menu'.  */)
(Lisp_Object string)
{
  Lisp_Object tail = !NILP (string) ? list1 (string) : Qnil;
  return Fcons (Qkeymap, Fcons (Fmake_char_table (Qkeymap, Qnil), tail));
}

void
syms_of_keymap (void)
{
  DEFSYM (Qkeymap, "keymap");

  Fput (Qkeymap, Qchar_table_extra_slots, make_fixnum (0));

  defsubr (&Smake_keymap);
}
