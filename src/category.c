#include "category.h"
#include "lisp.h"
#include "buffer.h"

DEFUN ("category-table-p", Fcategory_table_p, Scategory_table_p, 1, 1, 0,
       doc: /* Return t if ARG is a category table.  */)
(Lisp_Object arg)
{
  if (CHAR_TABLE_P (arg) && EQ (XCHAR_TABLE (arg)->purpose, Qcategory_table))
    return Qt;
  return Qnil;
}

static Lisp_Object
check_category_table (Lisp_Object table)
{
  if (NILP (table))
    return BVAR (current_buffer, category_table);
  CHECK_TYPE (!NILP (Fcategory_table_p (table)), Qcategory_table_p, table);
  return table;
}

DEFUN ("define-category", Fdefine_category, Sdefine_category, 2, 3, 0,
       doc: /* Define CATEGORY as a category which is described by DOCSTRING.
CATEGORY should be an ASCII printing character in the range ` ' to `~'.
DOCSTRING is the documentation string of the category.  The first line
should be a terse text (preferably less than 16 characters),
and the rest lines should be the full description.
The category is defined only in category table TABLE, which defaults to
the current buffer's category table.  */)
(Lisp_Object category, Lisp_Object docstring, Lisp_Object table)
{
  CHECK_CATEGORY (category);
  CHECK_STRING (docstring);
  table = check_category_table (table);

  if (!NILP (CATEGORY_DOCSTRING (table, XFIXNAT (category))))
    error ("Category `%c' is already defined", (int) XFIXNAT (category));
  if (!NILP (Vpurify_flag))
    docstring = Fpurecopy (docstring);
  SET_CATEGORY_DOCSTRING (table, XFIXNAT (category), docstring);

  return Qnil;
}

void
init_category_once (void)
{
  DEFSYM (Qcategory_table, "category-table");
  Fput (Qcategory_table, Qchar_table_extra_slots, make_fixnum (2));

  Vstandard_category_table = Fmake_char_table (Qcategory_table, Qnil);
  set_char_table_defalt (Vstandard_category_table, MAKE_CATEGORY_SET);
  Fset_char_table_extra_slot (Vstandard_category_table, make_fixnum (0),
                              make_nil_vector (95));
}

void
syms_of_category (void)
{
  DEFSYM (Qcategoryp, "categoryp");
  DEFSYM (Qcategorysetp, "categorysetp");
  DEFSYM (Qcategory_table_p, "category-table-p");

  defsubr (&Scategory_table_p);
  defsubr (&Sdefine_category);
}
