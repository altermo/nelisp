#include "lisp.h"

static void
set_char_table_parent (Lisp_Object table, Lisp_Object val)
{
  XCHAR_TABLE (table)->parent = val;
}

DEFUN ("make-char-table", Fmake_char_table, Smake_char_table, 1, 2, 0,
       doc: /* Return a newly created char-table, with purpose PURPOSE.
Each element is initialized to INIT, which defaults to nil.

PURPOSE should be a symbol.  If it has a `char-table-extra-slots'
property, the property's value should be an integer between 0 and 10
that specifies how many extra slots the char-table has.  Otherwise,
the char-table has no extra slot.  */)
(register Lisp_Object purpose, Lisp_Object init)
{
  Lisp_Object vector;
  Lisp_Object n;
  int n_extras;
  int size;

  CHECK_SYMBOL (purpose);
  n = Fget (purpose, Qchar_table_extra_slots);
  if (NILP (n))
    n_extras = 0;
  else
    {
      CHECK_FIXNAT (n);
      if (XFIXNUM (n) > 10)
        args_out_of_range (n, Qnil);
      n_extras = XFIXNUM (n);
    }

  size = CHAR_TABLE_STANDARD_SLOTS + n_extras;
  vector = make_vector (size, init);
  XSETPVECTYPE (XVECTOR (vector), PVEC_CHAR_TABLE);
  set_char_table_parent (vector, Qnil);
  set_char_table_purpose (vector, purpose);
  XSETCHAR_TABLE (vector, XCHAR_TABLE (vector));
  return vector;
}

void
syms_of_chartab (void)
{
  defsubr (&Smake_char_table);
}
