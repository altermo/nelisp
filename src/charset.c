#include "charset.h"
#include "lisp.h"
#include "character.h"
#include "coding.h"

Lisp_Object Vcharset_hash_table;

struct charset *charset_table;
int charset_table_size;
int charset_table_used;

int charset_ascii;
int charset_eight_bit;
static int charset_iso_8859_1;
int charset_unicode;
static int charset_emacs;

int charset_jisx0201_roman;
int charset_jisx0208_1978;
int charset_jisx0208;
int charset_ksc5601;

int charset_unibyte;

Lisp_Object Vcharset_ordered_list;

Lisp_Object Viso_2022_charset_list;
EMACS_UINT charset_ordered_list_tick;

Lisp_Object Vemacs_mule_charset_list;
int emacs_mule_charset[256];

int iso_charset_table[ISO_MAX_DIMENSION][ISO_MAX_CHARS][ISO_MAX_FINAL];

#define CODE_POINT_TO_INDEX(charset, code)                            \
  ((charset)->code_linear_p ? (int) ((code) - (charset)->min_code)    \
   : (((charset)->code_space_mask[(code) >> 24] & 0x8)                \
      && ((charset)->code_space_mask[((code) >> 16) & 0xFF] & 0x4)    \
      && ((charset)->code_space_mask[((code) >> 8) & 0xFF] & 0x2)     \
      && ((charset)->code_space_mask[(code) & 0xFF] & 0x1))           \
     ? (int) (((((code) >> 24) - (charset)->code_space[12])           \
               * (charset)->code_space[11])                           \
              + (((((code) >> 16) & 0xFF) - (charset)->code_space[8]) \
                 * (charset)->code_space[7])                          \
              + (((((code) >> 8) & 0xFF) - (charset)->code_space[4])  \
                 * (charset)->code_space[3])                          \
              + (((code) & 0xFF) - (charset)->code_space[0])          \
              - ((charset)->char_index_offset))                       \
     : -1)

DEFUN ("define-charset-internal", Fdefine_charset_internal,
       Sdefine_charset_internal, charset_arg_max, MANY, 0,
       doc: /* For internal use only.
usage: (define-charset-internal ...)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  /* Charset attr vector.  */
  Lisp_Object attrs;
  Lisp_Object val;
  struct Lisp_Hash_Table *hash_table = XHASH_TABLE (Vcharset_hash_table);
  int i, j;
  struct charset charset;
  int id;
  int dimension;
  bool new_definition_p;
  int nchars;

  memset (&charset, 0, sizeof (charset));

  if (nargs != charset_arg_max)
    Fsignal (Qwrong_number_of_arguments,
             Fcons (Qdefine_charset_internal, make_fixnum (nargs)));

  attrs = make_nil_vector (charset_attr_max);

  CHECK_SYMBOL (args[charset_arg_name]);
  ASET (attrs, charset_name, args[charset_arg_name]);

  val = args[charset_arg_code_space];
  for (i = 0, dimension = 0, nchars = 1;; i++)
    {
      Lisp_Object min_byte_obj = Faref (val, make_fixnum (i * 2));
      Lisp_Object max_byte_obj = Faref (val, make_fixnum (i * 2 + 1));
      int min_byte = check_integer_range (min_byte_obj, 0, 255);
      int max_byte = check_integer_range (max_byte_obj, min_byte, 255);
      charset.code_space[i * 4] = min_byte;
      charset.code_space[i * 4 + 1] = max_byte;
      charset.code_space[i * 4 + 2] = max_byte - min_byte + 1;
      if (max_byte > 0)
        dimension = i + 1;
      if (i == 3)
        break;
      nchars *= charset.code_space[i * 4 + 2];
      charset.code_space[i * 4 + 3] = nchars;
    }

  val = args[charset_arg_dimension];
  charset.dimension = !NILP (val) ? check_integer_range (val, 1, 4) : dimension;

  charset.code_linear_p
    = (charset.dimension == 1
       || (charset.code_space[2] == 256
           && (charset.dimension == 2
               || (charset.code_space[6] == 256
                   && (charset.dimension == 3
                       || charset.code_space[10] == 256)))));

  if (!charset.code_linear_p)
    {
      charset.code_space_mask = xzalloc (256);
      for (i = 0; i < 4; i++)
        for (j = charset.code_space[i * 4]; j <= charset.code_space[i * 4 + 1];
             j++)
          charset.code_space_mask[j] |= (1 << i);
    }

  charset.iso_chars_96 = charset.code_space[2] == 96;

  charset.min_code = (charset.code_space[0] | (charset.code_space[4] << 8)
                      | (charset.code_space[8] << 16)
                      | ((unsigned) charset.code_space[12] << 24));
  charset.max_code = (charset.code_space[1] | (charset.code_space[5] << 8)
                      | (charset.code_space[9] << 16)
                      | ((unsigned) charset.code_space[13] << 24));
  charset.char_index_offset = 0;

  val = args[charset_arg_min_code];
  if (!NILP (val))
    {
      unsigned code = cons_to_unsigned (val, UINT_MAX);

      if (code < charset.min_code || code > charset.max_code)
        args_out_of_range_3 (INT_TO_INTEGER (charset.min_code),
                             INT_TO_INTEGER (charset.max_code), val);
      charset.char_index_offset = CODE_POINT_TO_INDEX (&charset, code);
      charset.min_code = code;
    }

  val = args[charset_arg_max_code];
  if (!NILP (val))
    {
      unsigned code = cons_to_unsigned (val, UINT_MAX);

      if (code < charset.min_code || code > charset.max_code)
        args_out_of_range_3 (INT_TO_INTEGER (charset.min_code),
                             INT_TO_INTEGER (charset.max_code), val);
      charset.max_code = code;
    }

  charset.compact_codes_p = charset.max_code < 0x10000;

  val = args[charset_arg_invalid_code];
  if (NILP (val))
    {
      if (charset.min_code > 0)
        charset.invalid_code = 0;
      else
        {
          if (charset.max_code < UINT_MAX)
            charset.invalid_code = charset.max_code + 1;
          else
            error ("Attribute :invalid-code must be specified");
        }
    }
  else
    charset.invalid_code = cons_to_unsigned (val, UINT_MAX);

  val = args[charset_arg_iso_final];
  if (NILP (val))
    charset.iso_final = -1;
  else
    {
      CHECK_FIXNUM (val);
      if (XFIXNUM (val) < '0' || XFIXNUM (val) > 127)
        error ("Invalid iso-final-char: %" pI "d", XFIXNUM (val));
      charset.iso_final = XFIXNUM (val);
    }

  val = args[charset_arg_iso_revision];
  charset.iso_revision = !NILP (val) ? check_integer_range (val, -1, 63) : -1;

  val = args[charset_arg_emacs_mule_id];
  if (NILP (val))
    charset.emacs_mule_id = -1;
  else
    {
      CHECK_FIXNAT (val);
      if ((XFIXNUM (val) > 0 && XFIXNUM (val) <= 128) || XFIXNUM (val) >= 256)
        error ("Invalid emacs-mule-id: %" pI "d", XFIXNUM (val));
      charset.emacs_mule_id = XFIXNUM (val);
    }

  charset.ascii_compatible_p = !NILP (args[charset_arg_ascii_compatible_p]);

  charset.supplementary_p = !NILP (args[charset_arg_supplementary_p]);

  charset.unified_p = 0;

  memset (charset.fast_map, 0, sizeof (charset.fast_map));

  if (!NILP (args[charset_arg_code_offset]))
    {
      val = args[charset_arg_code_offset];
      CHECK_CHARACTER (val);

      charset.method = CHARSET_METHOD_OFFSET;
      charset.code_offset = XFIXNUM (val);

      i = CODE_POINT_TO_INDEX (&charset, charset.max_code);
      if (MAX_CHAR - charset.code_offset < i)
        error ("Unsupported max char: %d", charset.max_char);
      charset.max_char = i + charset.code_offset;
      i = CODE_POINT_TO_INDEX (&charset, charset.min_code);
      charset.min_char = i + charset.code_offset;

      i = (charset.min_char >> 7) << 7;
      for (; i < 0x10000 && i <= charset.max_char; i += 128)
        CHARSET_FAST_MAP_SET (i, charset.fast_map);
      i = (i >> 12) << 12;
      for (; i <= charset.max_char; i += 0x1000)
        CHARSET_FAST_MAP_SET (i, charset.fast_map);
      if (charset.code_offset == 0 && charset.max_char >= 0x80)
        charset.ascii_compatible_p = 1;
    }
  else if (!NILP (args[charset_arg_map]))
    {
      val = args[charset_arg_map];
      ASET (attrs, charset_map, val);
      charset.method = CHARSET_METHOD_MAP;
    }
  else if (!NILP (args[charset_arg_subset]))
    {
      Lisp_Object parent;
      Lisp_Object parent_min_code, parent_max_code, parent_code_offset;
      struct charset *parent_charset;

      val = args[charset_arg_subset];
      parent = Fcar (val);
      TODO;
      UNUSED (parent); // CHECK_CHARSET_GET_CHARSET (parent, parent_charset);
      parent_min_code = Fnth (make_fixnum (1), val);
      CHECK_FIXNAT (parent_min_code);
      parent_max_code = Fnth (make_fixnum (2), val);
      CHECK_FIXNAT (parent_max_code);
      parent_code_offset = Fnth (make_fixnum (3), val);
      CHECK_FIXNUM (parent_code_offset);
      ASET (attrs, charset_subset,
            CALLN (Fvector, make_fixnum (parent_charset->id), parent_min_code,
                   parent_max_code, parent_code_offset));

      charset.method = CHARSET_METHOD_SUBSET;
      memcpy (charset.fast_map, parent_charset->fast_map,
              sizeof charset.fast_map);

      charset.min_char = parent_charset->min_char;
      charset.max_char = parent_charset->max_char;
    }
  else if (!NILP (args[charset_arg_superset]))
    {
      val = args[charset_arg_superset];
      charset.method = CHARSET_METHOD_SUPERSET;
      TODO; // val = Fcopy_sequence (val);
      ASET (attrs, charset_superset, val);

      charset.min_char = MAX_CHAR;
      charset.max_char = 0;
      for (; !NILP (val); val = Fcdr (val))
        {
          Lisp_Object elt, car_part, cdr_part;
          int this_id, offset;
          struct charset *this_charset;

          elt = Fcar (val);
          if (CONSP (elt))
            {
              car_part = XCAR (elt);
              cdr_part = XCDR (elt);
              TODO;
              UNUSED (car_part); // CHECK_CHARSET_GET_ID (car_part, this_id);
              offset = check_integer_range (cdr_part, INT_MIN, INT_MAX);
            }
          else
            {
              TODO; // CHECK_CHARSET_GET_ID (elt, this_id);
              offset = 0;
            }
          XSETCAR (val, Fcons (make_fixnum (this_id), make_fixnum (offset)));

          TODO; // this_charset = CHARSET_FROM_ID (this_id);
          if (charset.min_char > this_charset->min_char)
            charset.min_char = this_charset->min_char;
          if (charset.max_char < this_charset->max_char)
            charset.max_char = this_charset->max_char;
          for (i = 0; i < 190; i++)
            charset.fast_map[i] |= this_charset->fast_map[i];
        }
    }
  else
    error ("None of :code-offset, :map, :parents are specified");

  val = args[charset_arg_unify_map];
  if (!NILP (val) && !STRINGP (val))
    TODO; // CHECK_VECTOR (val);
  ASET (attrs, charset_unify_map, val);

  CHECK_LIST (args[charset_arg_plist]);
  ASET (attrs, charset_plist, args[charset_arg_plist]);

  hash_hash_t hash_code;
  ptrdiff_t hash_index
    = hash_lookup_get_hash (hash_table, args[charset_arg_name], &hash_code);
  if (hash_index >= 0)
    {
      new_definition_p = false;
      id = XFIXNAT (CHARSET_SYMBOL_ID (args[charset_arg_name]));
      set_hash_value_slot (hash_table, hash_index, attrs);
    }
  else
    {
      hash_put (hash_table, args[charset_arg_name], attrs, hash_code);
      if (charset_table_used == charset_table_size)
        {
          int old_size = charset_table_size;
          ptrdiff_t new_size = old_size;
          struct charset *new_table
            = xpalloc (0, &new_size, 1, min (INT_MAX, MOST_POSITIVE_FIXNUM),
                       sizeof *charset_table);
          memcpy (new_table, charset_table, old_size * sizeof *new_table);
          charset_table = new_table;
          charset_table_size = new_size;
        }
      id = charset_table_used++;
      new_definition_p = 1;
    }

  ASET (attrs, charset_id, make_fixnum (id));
  charset.id = id;
  charset.attributes = attrs;
  charset_table[id] = charset;

  if (charset.method == CHARSET_METHOD_MAP)
    TODO;

  if (charset.iso_final >= 0)
    {
      ISO_CHARSET_TABLE (charset.dimension, charset.iso_chars_96,
                         charset.iso_final)
        = id;
      if (new_definition_p)
        Viso_2022_charset_list = nconc2 (Viso_2022_charset_list, list1i (id));
      if (ISO_CHARSET_TABLE (1, 0, 'J') == id)
        charset_jisx0201_roman = id;
      else if (ISO_CHARSET_TABLE (2, 0, '@') == id)
        charset_jisx0208_1978 = id;
      else if (ISO_CHARSET_TABLE (2, 0, 'B') == id)
        charset_jisx0208 = id;
      else if (ISO_CHARSET_TABLE (2, 0, 'C') == id)
        charset_ksc5601 = id;
    }

  if (charset.emacs_mule_id >= 0)
    {
      emacs_mule_charset[charset.emacs_mule_id] = id;
      if (charset.emacs_mule_id < 0xA0)
        emacs_mule_bytes[charset.emacs_mule_id] = charset.dimension + 1;
      else
        emacs_mule_bytes[charset.emacs_mule_id] = charset.dimension + 2;
      if (new_definition_p)
        Vemacs_mule_charset_list
          = nconc2 (Vemacs_mule_charset_list, list1i (id));
    }

  if (new_definition_p)
    {
      Vcharset_list = Fcons (args[charset_arg_name], Vcharset_list);
      if (charset.supplementary_p)
        Vcharset_ordered_list = nconc2 (Vcharset_ordered_list, list1i (id));
      else
        {
          Lisp_Object tail;

          for (tail = Vcharset_ordered_list; CONSP (tail); tail = XCDR (tail))
            {
              struct charset *cs = CHARSET_FROM_ID (XFIXNUM (XCAR (tail)));

              if (cs->supplementary_p)
                break;
            }
          if (EQ (tail, Vcharset_ordered_list))
            Vcharset_ordered_list
              = Fcons (make_fixnum (id), Vcharset_ordered_list);
          else if (NILP (tail))
            Vcharset_ordered_list = nconc2 (Vcharset_ordered_list, list1i (id));
          else
            {
              val = Fcons (XCAR (tail), XCDR (tail));
              XSETCDR (tail, val);
              XSETCAR (tail, make_fixnum (id));
            }
        }
      charset_ordered_list_tick++;
    }

  return Qnil;
}

static int
define_charset_internal (Lisp_Object name, int dimension,
                         const char *code_space_chars, unsigned min_code,
                         unsigned max_code, int iso_final, int iso_revision,
                         int emacs_mule_id, bool ascii_compatible,
                         bool supplementary, int code_offset)
{
  const unsigned char *code_space = (const unsigned char *) code_space_chars;
  Lisp_Object args[charset_arg_max];
  Lisp_Object val;
  int i;

  args[charset_arg_name] = name;
  args[charset_arg_dimension] = make_fixnum (dimension);
  val = make_uninit_vector (8);
  for (i = 0; i < 8; i++)
    ASET (val, i, make_fixnum (code_space[i]));
  args[charset_arg_code_space] = val;
  args[charset_arg_min_code] = make_fixnum (min_code);
  args[charset_arg_max_code] = make_fixnum (max_code);
  args[charset_arg_iso_final]
    = (iso_final < 0 ? Qnil : make_fixnum (iso_final));
  args[charset_arg_iso_revision] = make_fixnum (iso_revision);
  args[charset_arg_emacs_mule_id]
    = (emacs_mule_id < 0 ? Qnil : make_fixnum (emacs_mule_id));
  args[charset_arg_ascii_compatible_p] = ascii_compatible ? Qt : Qnil;
  args[charset_arg_supplementary_p] = supplementary ? Qt : Qnil;
  args[charset_arg_invalid_code] = Qnil;
  args[charset_arg_code_offset] = make_fixnum (code_offset);
  args[charset_arg_map] = Qnil;
  args[charset_arg_subset] = Qnil;
  args[charset_arg_superset] = Qnil;
  args[charset_arg_unify_map] = Qnil;

  args[charset_arg_plist]
    = list (QCname, args[charset_arg_name], intern_c_string (":dimension"),
            args[charset_arg_dimension], intern_c_string (":code-space"),
            args[charset_arg_code_space], intern_c_string (":iso-final-char"),
            args[charset_arg_iso_final], intern_c_string (":emacs-mule-id"),
            args[charset_arg_emacs_mule_id], QCascii_compatible_p,
            args[charset_arg_ascii_compatible_p],
            intern_c_string (":code-offset"), args[charset_arg_code_offset]);
  Fdefine_charset_internal (charset_arg_max, args);

  return XFIXNUM (CHARSET_SYMBOL_ID (name));
}

DEFUN ("define-charset-alias", Fdefine_charset_alias,
       Sdefine_charset_alias, 2, 2, 0,
       doc: /* Define ALIAS as an alias for charset CHARSET.  */)
(Lisp_Object alias, Lisp_Object charset)
{
  Lisp_Object attr;

  CHECK_CHARSET_GET_ATTR (charset, attr);
  Fputhash (alias, attr, Vcharset_hash_table);
  Vcharset_list = Fcons (alias, Vcharset_list);
  return Qnil;
}

DEFUN ("charset-plist", Fcharset_plist, Scharset_plist, 1, 1, 0,
       doc: /* Return the property list of CHARSET.  */)
(Lisp_Object charset)
{
  Lisp_Object attrs;

  CHECK_CHARSET_GET_ATTR (charset, attrs);
  return CHARSET_ATTR_PLIST (attrs);
}

DEFUN ("set-charset-plist", Fset_charset_plist, Sset_charset_plist, 2, 2, 0,
       doc: /* Set CHARSET's property list to PLIST.  */)
(Lisp_Object charset, Lisp_Object plist)
{
  Lisp_Object attrs;

  CHECK_CHARSET_GET_ATTR (charset, attrs);
  ASET (attrs, charset_plist, plist);
  return plist;
}

void
init_charset_once (void)
{
  TODO_NELISP_LATER;

  int i;
  for (i = 0; i < 256; i++)
    emacs_mule_charset[i] = -1;

  charset_jisx0201_roman = -1;

  charset_jisx0208_1978 = -1;

  charset_jisx0208 = -1;

  charset_ksc5601 = -1;
}

static struct charset charset_table_init[180];

void
syms_of_charset (void)
{
  DEFSYM (Qcharsetp, "charsetp");
  DEFSYM (Qdefine_charset_internal, "define-charset-internal");

  DEFSYM (Qascii, "ascii");
  DEFSYM (Qunicode, "unicode");
  DEFSYM (Qiso_8859_1, "iso-8859-1");
  DEFSYM (Qemacs, "emacs");
  DEFSYM (Qeight_bit, "eight-bit");

  staticpro (&Vcharset_ordered_list);
  Vcharset_ordered_list = Qnil;

  staticpro (&Viso_2022_charset_list);
  Viso_2022_charset_list = Qnil;

  staticpro (&Vemacs_mule_charset_list);
  Vemacs_mule_charset_list = Qnil;

  staticpro (&Vcharset_hash_table);
  Vcharset_hash_table = CALLN (Fmake_hash_table, QCtest, Qeq);

  charset_table = charset_table_init;
  charset_table_size = ARRAYELTS (charset_table_init);
  charset_table_used = 0;

  defsubr (&Sdefine_charset_internal);
  defsubr (&Sdefine_charset_alias);
  defsubr (&Scharset_plist);
  defsubr (&Sset_charset_plist);

  DEFVAR_LISP ("charset-list", Vcharset_list,
	       doc: /* List of all charsets ever defined.  */);
  Vcharset_list = Qnil;

  charset_ascii = define_charset_internal (Qascii, 1, "\x00\x7F\0\0\0\0\0", 0,
                                           127, 'B', -1, 0, 1, 0, 0);

  charset_iso_8859_1
    = define_charset_internal (Qiso_8859_1, 1, "\x00\xFF\0\0\0\0\0", 0, 255, -1,
                               -1, -1, 1, 0, 0);

  charset_unicode
    = define_charset_internal (Qunicode, 3, "\x00\xFF\x00\xFF\x00\x10\0", 0,
                               MAX_UNICODE_CHAR, -1, 0, -1, 1, 0, 0);

  charset_emacs
    = define_charset_internal (Qemacs, 3, "\x00\xFF\x00\xFF\x00\x3F\0", 0,
                               MAX_5_BYTE_CHAR, -1, 0, -1, 1, 1, 0);

  charset_eight_bit
    = define_charset_internal (Qeight_bit, 1, "\x80\xFF\0\0\0\0\0", 128, 255,
                               -1, 0, -1, 0, 1, MAX_5_BYTE_CHAR + 1);

  charset_unibyte = charset_iso_8859_1;
}
