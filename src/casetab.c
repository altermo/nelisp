#include "lisp.h"

Lisp_Object Vascii_downcase_table;
static Lisp_Object Vascii_upcase_table;
Lisp_Object Vascii_canon_table;
static Lisp_Object Vascii_eqv_table;

DEFUN ("case-table-p", Fcase_table_p, Scase_table_p, 1, 1, 0,
       doc: /* Return t if OBJECT is a case table.
See `set-case-table' for more information on these data structures.  */)
(Lisp_Object object)
{
  Lisp_Object up, canon, eqv;

  if (!CHAR_TABLE_P (object))
    return Qnil;
  if (!EQ (XCHAR_TABLE (object)->purpose, Qcase_table))
    return Qnil;

  up = XCHAR_TABLE (object)->extras[0];
  canon = XCHAR_TABLE (object)->extras[1];
  eqv = XCHAR_TABLE (object)->extras[2];

  return (
    (NILP (up) || CHAR_TABLE_P (up))
        && ((NILP (canon) && NILP (eqv))
            || (CHAR_TABLE_P (canon) && (NILP (eqv) || CHAR_TABLE_P (eqv))))
      ? Qt
      : Qnil);
}

DEFUN ("standard-case-table", Fstandard_case_table, Sstandard_case_table, 0, 0, 0,
       doc: /* Return the standard case table.
This is the one used for new buffers.  */)
(void) { return Vascii_downcase_table; }

static Lisp_Object
check_case_table (Lisp_Object obj)
{
  CHECK_TYPE (!NILP (Fcase_table_p (obj)), Qcase_table_p, obj);
  return (obj);
}

static void
set_canon (Lisp_Object case_table, Lisp_Object range, Lisp_Object elt)
{
  Lisp_Object up = XCHAR_TABLE (case_table)->extras[0];
  Lisp_Object canon = XCHAR_TABLE (case_table)->extras[1];

  if (FIXNATP (elt))
    Fset_char_table_range (canon, range, Faref (case_table, Faref (up, elt)));
}

static void
set_identity (Lisp_Object table, Lisp_Object c, Lisp_Object elt)
{
  if (FIXNATP (elt))
    {
      int from, to;

      if (CONSP (c))
        {
          from = XFIXNUM (XCAR (c));
          to = XFIXNUM (XCDR (c));
        }
      else
        from = to = XFIXNUM (c);

      to++;
      for (; from < to; from++)
        CHAR_TABLE_SET (table, from, make_fixnum (from));
    }
}

static void
shuffle (Lisp_Object table, Lisp_Object c, Lisp_Object elt)
{
  if (FIXNATP (elt))
    {
      int from, to;

      if (CONSP (c))
        {
          from = XFIXNUM (XCAR (c));
          to = XFIXNUM (XCDR (c));
        }
      else
        from = to = XFIXNUM (c);

      to++;
      for (; from < to; from++)
        {
          Lisp_Object tem = Faref (table, elt);
          Faset (table, elt, make_fixnum (from));
          Faset (table, make_fixnum (from), tem);
        }
    }
}

static Lisp_Object
set_case_table (Lisp_Object table, bool standard)
{
  Lisp_Object up, canon, eqv;

  check_case_table (table);

  up = XCHAR_TABLE (table)->extras[0];
  canon = XCHAR_TABLE (table)->extras[1];
  eqv = XCHAR_TABLE (table)->extras[2];

  if (NILP (up))
    {
      up = Fmake_char_table (Qcase_table, Qnil);
      map_char_table (set_identity, Qnil, table, up);
      map_char_table (shuffle, Qnil, table, up);
      set_char_table_extras (table, 0, up);
    }

  if (NILP (canon))
    {
      canon = Fmake_char_table (Qcase_table, Qnil);
      set_char_table_extras (table, 1, canon);
      map_char_table (set_canon, Qnil, table, table);
    }

  if (NILP (eqv))
    {
      eqv = Fmake_char_table (Qcase_table, Qnil);
      map_char_table (set_identity, Qnil, canon, eqv);
      map_char_table (shuffle, Qnil, canon, eqv);
      set_char_table_extras (table, 2, eqv);
    }

  set_char_table_extras (canon, 2, eqv);

  if (standard)
    {
      Vascii_downcase_table = table;
      Vascii_upcase_table = up;
      Vascii_canon_table = canon;
      Vascii_eqv_table = eqv;
    }
  else
    {
      TODO;
    }

  return table;
}

void
init_casetab_once (void)
{
  register int i;
  Lisp_Object down, up, eqv;

  DEFSYM (Qcase_table, "case-table");
  Fput (Qcase_table, Qchar_table_extra_slots, make_fixnum (3));

  down = Fmake_char_table (Qcase_table, Qnil);
  Vascii_downcase_table = down;
  set_char_table_purpose (down, Qcase_table);

  for (i = 0; i < 128; i++)
    {
      int c = (i >= 'A' && i <= 'Z') ? i + ('a' - 'A') : i;
      CHAR_TABLE_SET (down, i, make_fixnum (c));
    }

  set_char_table_extras (down, 1, Fcopy_sequence (down));

  up = Fmake_char_table (Qcase_table, Qnil);
  set_char_table_extras (down, 0, up);

  for (i = 0; i < 128; i++)
    {
      int c = (i >= 'a' && i <= 'z') ? i + ('A' - 'a') : i;
      CHAR_TABLE_SET (up, i, make_fixnum (c));
    }

  eqv = Fmake_char_table (Qcase_table, Qnil);

  for (i = 0; i < 128; i++)
    {
      int c = ((i >= 'A' && i <= 'Z')
                 ? i + ('a' - 'A')
                 : ((i >= 'a' && i <= 'z') ? i + ('A' - 'a') : i));
      CHAR_TABLE_SET (eqv, i, make_fixnum (c));
    }

  set_char_table_extras (down, 2, eqv);

  set_case_table (down, 1);
}

void
syms_of_casetab (void)
{
  DEFSYM (Qcase_table_p, "case-table-p");

  staticpro (&Vascii_canon_table);
  staticpro (&Vascii_downcase_table);
  staticpro (&Vascii_eqv_table);
  staticpro (&Vascii_upcase_table);

  defsubr (&Scase_table_p);
  defsubr (&Sstandard_case_table);
}
