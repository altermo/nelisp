#include "category.h"
#include "lisp.h"
#include "buffer.h"
#include "character.h"

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

static Lisp_Object
hash_get_category_set (Lisp_Object table, Lisp_Object category_set)
{
  if (NILP (XCHAR_TABLE (table)->extras[1]))
    set_char_table_extras (table, 1,
                           make_hash_table (&hashtest_equal, DEFAULT_HASH_SIZE,
                                            Weak_None, false));
  struct Lisp_Hash_Table *h = XHASH_TABLE (XCHAR_TABLE (table)->extras[1]);
  hash_hash_t hash;
  ptrdiff_t i = hash_lookup_get_hash (h, category_set, &hash);
  if (i >= 0)
    return HASH_KEY (h, i);
  hash_put (h, category_set, Qnil, hash);
  return category_set;
}

static void
set_category_set (Lisp_Object category_set, EMACS_INT category, bool val)
{
  bool_vector_set (category_set, category, val);
}

Lisp_Object
char_category_set (int c)
{
  return CHAR_TABLE_REF (BVAR (current_buffer, category_table), c);
}

DEFUN ("modify-category-entry", Fmodify_category_entry,
       Smodify_category_entry, 2, 4, 0,
       doc: /* Modify the category set of CHARACTER by adding CATEGORY to it.
The category is changed only for table TABLE, which defaults to
the current buffer's category table.
CHARACTER can be either a single character or a cons representing the
lower and upper ends of an inclusive character range to modify.
CATEGORY must be a category name (a character between ` ' and `~').
Use `describe-categories' to see existing category names.
If optional fourth argument RESET is non-nil,
then delete CATEGORY from the category set instead of adding it.  */)
(Lisp_Object character, Lisp_Object category, Lisp_Object table,
 Lisp_Object reset)
{
  bool set_value; /* Actual value to be set in category sets.  */
  Lisp_Object category_set;
  int start, end;
  int from, to;

  if (FIXNUMP (character))
    {
      CHECK_CHARACTER (character);
      start = end = XFIXNAT (character);
    }
  else
    {
      CHECK_CONS (character);
      CHECK_CHARACTER_CAR (character);
      CHECK_CHARACTER_CDR (character);
      start = XFIXNAT (XCAR (character));
      end = XFIXNAT (XCDR (character));
    }

  CHECK_CATEGORY (category);
  table = check_category_table (table);

  if (NILP (CATEGORY_DOCSTRING (table, XFIXNAT (category))))
    error ("Undefined category: %c", (int) XFIXNAT (category));

  set_value = NILP (reset);

  while (start <= end)
    {
      from = start, to = end;
      category_set = char_table_ref_and_range (table, start, &from, &to);
      if (CATEGORY_MEMBER (XFIXNAT (category), category_set) != NILP (reset))
        {
          category_set = Fcopy_sequence (category_set);
          set_category_set (category_set, XFIXNAT (category), set_value);
          category_set = hash_get_category_set (table, category_set);
          char_table_set_range (table, start, to, category_set);
        }
      start = to + 1;
    }

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
  defsubr (&Smodify_category_entry);
}
