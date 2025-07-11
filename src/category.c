#include "lisp.h"

DEFUN ("category-table-p", Fcategory_table_p, Scategory_table_p, 1, 1, 0,
       doc: /* Return t if ARG is a category table.  */)
(Lisp_Object arg)
{
  if (CHAR_TABLE_P (arg) && EQ (XCHAR_TABLE (arg)->purpose, Qcategory_table))
    return Qt;
  return Qnil;
}

void
init_category_once (void)
{
  DEFSYM (Qcategory_table, "category-table");
  Fput (Qcategory_table, Qchar_table_extra_slots, make_fixnum (2));
}

void
syms_of_category (void)
{
  defsubr (&Scategory_table_p);
}
