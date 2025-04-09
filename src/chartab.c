#include "lisp.h"
#include "character.h"

const int chartab_size[4]
  = { (1 << CHARTAB_SIZE_BITS_0), (1 << CHARTAB_SIZE_BITS_1),
      (1 << CHARTAB_SIZE_BITS_2), (1 << CHARTAB_SIZE_BITS_3) };

static const int chartab_chars[4]
  = { (1 << (CHARTAB_SIZE_BITS_1 + CHARTAB_SIZE_BITS_2 + CHARTAB_SIZE_BITS_3)),
      (1 << (CHARTAB_SIZE_BITS_2 + CHARTAB_SIZE_BITS_3)),
      (1 << CHARTAB_SIZE_BITS_3), 1 };

static const int chartab_bits[4]
  = { (CHARTAB_SIZE_BITS_1 + CHARTAB_SIZE_BITS_2 + CHARTAB_SIZE_BITS_3),
      (CHARTAB_SIZE_BITS_2 + CHARTAB_SIZE_BITS_3), CHARTAB_SIZE_BITS_3, 0 };

#define CHARTAB_IDX(c, depth, min_char) \
  (((c) - (min_char)) >> chartab_bits[(depth)])

#define UNIPROP_TABLE_P(TABLE)                                  \
  (EQ (XCHAR_TABLE (TABLE)->purpose, Qchar_code_property_table) \
   && CHAR_TABLE_EXTRA_SLOTS (XCHAR_TABLE (TABLE)) == 5)

#define UNIPROP_COMPRESSED_FORM_P(OBJ) \
  (STRINGP (OBJ) && SCHARS (OBJ) > 0   \
   && ((SREF (OBJ, 0) == 1 || (SREF (OBJ, 0) == 2))))

static void
CHECK_CHAR_TABLE (Lisp_Object x)
{
  CHECK_TYPE (CHAR_TABLE_P (x), Qchar_table_p, x);
}
static void
set_char_table_ascii (Lisp_Object table, Lisp_Object val)
{
  XCHAR_TABLE (table)->ascii = val;
}
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

static Lisp_Object
make_sub_char_table (int depth, int min_char, Lisp_Object defalt)
{
  int i;
  Lisp_Object table = make_uninit_sub_char_table (depth, min_char);

  for (i = 0; i < chartab_size[depth]; i++)
    XSUB_CHAR_TABLE (table)->contents[i] = defalt;
  return table;
}
static Lisp_Object
char_table_ascii (Lisp_Object table)
{
  Lisp_Object sub, val;

  sub = XCHAR_TABLE (table)->contents[0];
  if (!SUB_CHAR_TABLE_P (sub))
    return sub;
  sub = XSUB_CHAR_TABLE (sub)->contents[0];
  if (!SUB_CHAR_TABLE_P (sub))
    return sub;
  val = XSUB_CHAR_TABLE (sub)->contents[0];
  if (UNIPROP_TABLE_P (table) && UNIPROP_COMPRESSED_FORM_P (val))
    TODO; // val = uniprop_table_uncompress (sub, 0);
  return val;
}

static void
sub_char_table_set (Lisp_Object table, int c, Lisp_Object val, bool is_uniprop)
{
  struct Lisp_Sub_Char_Table *tbl = XSUB_CHAR_TABLE (table);
  int depth = tbl->depth, min_char = tbl->min_char;
  int i = CHARTAB_IDX (c, depth, min_char);
  Lisp_Object sub;

  if (depth == 3)
    set_sub_char_table_contents (table, i, val);
  else
    {
      sub = tbl->contents[i];
      if (!SUB_CHAR_TABLE_P (sub))
        {
          if (is_uniprop && UNIPROP_COMPRESSED_FORM_P (sub))
            TODO; // sub = uniprop_table_uncompress (table, i);
          else
            {
              sub = make_sub_char_table (depth + 1,
                                         min_char + i * chartab_chars[depth],
                                         sub);
              set_sub_char_table_contents (table, i, sub);
            }
        }
      sub_char_table_set (sub, c, val, is_uniprop);
    }
}

void
char_table_set (Lisp_Object table, int c, Lisp_Object val)
{
  struct Lisp_Char_Table *tbl = XCHAR_TABLE (table);

  if (ASCII_CHAR_P (c) && SUB_CHAR_TABLE_P (tbl->ascii))
    set_sub_char_table_contents (tbl->ascii, c, val);
  else
    {
      int i = CHARTAB_IDX (c, 0, 0);
      Lisp_Object sub;

      sub = tbl->contents[i];
      if (!SUB_CHAR_TABLE_P (sub))
        {
          sub = make_sub_char_table (1, i * chartab_chars[0], sub);
          set_char_table_contents (table, i, sub);
        }
      sub_char_table_set (sub, c, val, UNIPROP_TABLE_P (table));
      if (ASCII_CHAR_P (c))
        set_char_table_ascii (table, char_table_ascii (table));
    }
}

static void
sub_char_table_set_range (Lisp_Object table, int from, int to, Lisp_Object val,
                          bool is_uniprop)
{
  struct Lisp_Sub_Char_Table *tbl = XSUB_CHAR_TABLE (table);
  int depth = tbl->depth, min_char = tbl->min_char;
  int chars_in_block = chartab_chars[depth];
  int i, c, lim = chartab_size[depth];

  if (from < min_char)
    from = min_char;
  i = CHARTAB_IDX (from, depth, min_char);
  c = min_char + chars_in_block * i;
  for (; i < lim; i++, c += chars_in_block)
    {
      if (c > to)
        break;
      if (from <= c && c + chars_in_block - 1 <= to)
        set_sub_char_table_contents (table, i, val);
      else
        {
          Lisp_Object sub = tbl->contents[i];
          if (!SUB_CHAR_TABLE_P (sub))
            {
              if (is_uniprop && UNIPROP_COMPRESSED_FORM_P (sub))
                TODO; // sub = uniprop_table_uncompress (table, i);
              else
                {
                  sub = make_sub_char_table (depth + 1, c, sub);
                  set_sub_char_table_contents (table, i, sub);
                }
            }
          sub_char_table_set_range (sub, from, to, val, is_uniprop);
        }
    }
}

void
char_table_set_range (Lisp_Object table, int from, int to, Lisp_Object val)
{
  struct Lisp_Char_Table *tbl = XCHAR_TABLE (table);

  if (from == to)
    char_table_set (table, from, val);
  else
    {
      bool is_uniprop = UNIPROP_TABLE_P (table);
      int lim = CHARTAB_IDX (to, 0, 0);
      int i, c;

      for (i = CHARTAB_IDX (from, 0, 0), c = i * chartab_chars[0]; i <= lim;
           i++, c += chartab_chars[0])
        {
          if (c > to)
            break;
          if (from <= c && c + chartab_chars[0] - 1 <= to)
            set_char_table_contents (table, i, val);
          else
            {
              Lisp_Object sub = tbl->contents[i];
              if (!SUB_CHAR_TABLE_P (sub))
                {
                  sub = make_sub_char_table (1, i * chartab_chars[0], sub);
                  set_char_table_contents (table, i, sub);
                }
              sub_char_table_set_range (sub, from, to, val, is_uniprop);
            }
        }
      if (ASCII_CHAR_P (from))
        set_char_table_ascii (table, char_table_ascii (table));
    }
}

DEFUN ("set-char-table-range", Fset_char_table_range, Sset_char_table_range,
       3, 3, 0,
       doc: /* Set the value in CHAR-TABLE for a range of characters RANGE to VALUE.
RANGE should be t (for all characters), nil (for the default value),
a cons of character codes (for characters in the range),
or a character code.  Return VALUE.  */)
(Lisp_Object char_table, Lisp_Object range, Lisp_Object value)
{
  CHECK_CHAR_TABLE (char_table);
  if (EQ (range, Qt))
    {
      int i;

      set_char_table_ascii (char_table, value);
      for (i = 0; i < chartab_size[0]; i++)
        set_char_table_contents (char_table, i, value);
    }
  else if (NILP (range))
    set_char_table_defalt (char_table, value);
  else if (CHARACTERP (range))
    char_table_set (char_table, XFIXNUM (range), value);
  else if (CONSP (range))
    {
      CHECK_CHARACTER_CAR (range);
      CHECK_CHARACTER_CDR (range);
      char_table_set_range (char_table, XFIXNUM (XCAR (range)),
                            XFIXNUM (XCDR (range)), value);
    }
  else
    error ("Invalid RANGE argument to `set-char-table-range'");

  return value;
}

void
syms_of_chartab (void)
{
  DEFSYM (Qchar_code_property_table, "char-code-property-table");

  defsubr (&Smake_char_table);
  defsubr (&Sset_char_table_range);
}
