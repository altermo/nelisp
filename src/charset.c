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
Lisp_Object Vcharset_non_preferred_head;

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

#define INDEX_TO_CODE_POINT(charset, idx)                                      \
  ((charset)->code_linear_p                                                    \
     ? (idx) + (charset)->min_code                                             \
     : (idx += (charset)->char_index_offset,                                   \
        (((charset)->code_space[0] + (idx) % (charset)->code_space[2])         \
         | (((charset)->code_space[4]                                          \
             + ((idx) / (charset)->code_space[3] % (charset)->code_space[6]))  \
            << 8)                                                              \
         | (((charset)->code_space[8]                                          \
             + ((idx) / (charset)->code_space[7] % (charset)->code_space[10])) \
            << 16)                                                             \
         | (((charset)->code_space[12] + ((idx) / (charset)->code_space[11]))  \
            << 24))))

static struct
{
  struct charset *current;

  short for_encoder;

  int min_char, max_char;

  int zero_index_char;

  union
  {
    int decoder[0x10000];
    unsigned short encoder[0x20000];
  } table;
} *temp_charset_work = NULL;

#define SET_TEMP_CHARSET_WORK_ENCODER(C, CODE)                    \
  do                                                              \
    {                                                             \
      if ((CODE) == 0)                                            \
        temp_charset_work->zero_index_char = (C);                 \
      else if ((C) < 0x20000)                                     \
        temp_charset_work->table.encoder[(C)] = (CODE);           \
      else                                                        \
        temp_charset_work->table.encoder[(C) - 0x10000] = (CODE); \
    }                                                             \
  while (0)

#define GET_TEMP_CHARSET_WORK_ENCODER(C)                              \
  ((C) == temp_charset_work->zero_index_char ? 0                      \
   : (C) < 0x20000 ? (temp_charset_work->table.encoder[(C)]           \
                        ? (int) temp_charset_work->table.encoder[(C)] \
                        : -1)                                         \
   : temp_charset_work->table.encoder[(C) - 0x10000]                  \
     ? temp_charset_work->table.encoder[(C) - 0x10000]                \
     : -1)

#define SET_TEMP_CHARSET_WORK_DECODER(C, CODE) \
  (temp_charset_work->table.decoder[(CODE)] = (C))

#define GET_TEMP_CHARSET_WORK_DECODER(CODE) \
  (temp_charset_work->table.decoder[(CODE)])

bool charset_map_loaded = false;

struct charset_map_entries
{
  struct
  {
    unsigned from, to;
    int c;
  } entry[0x10000];
  struct charset_map_entries *next;
};

static void
load_charset_map (struct charset *charset, struct charset_map_entries *entries,
                  int n_entries, int control_flag)
{
  Lisp_Object vec UNINIT;
  Lisp_Object table UNINIT;
  unsigned max_code = CHARSET_MAX_CODE (charset);
  bool ascii_compatible_p = charset->ascii_compatible_p;
  int min_char, max_char, nonascii_min_char;
  int i;
  unsigned char *fast_map = charset->fast_map;

  if (n_entries <= 0)
    return;

  if (control_flag)
    {
      if (!inhibit_load_charset_map)
        {
          if (control_flag == 1)
            {
              if (charset->method == CHARSET_METHOD_MAP)
                {
                  int n = CODE_POINT_TO_INDEX (charset, max_code) + 1;

                  vec = make_vector (n, make_fixnum (-1));
                  set_charset_attr (charset, charset_decoder, vec);
                }
              else
                {
                  char_table_set_range (Vchar_unify_table, charset->min_char,
                                        charset->max_char, Qnil);
                }
            }
          else
            {
              table = Fmake_char_table (Qnil, Qnil);
              set_charset_attr (charset,
                                (charset->method == CHARSET_METHOD_MAP
                                   ? charset_encoder
                                   : charset_deunifier),
                                table);
            }
        }
      else
        {
          if (!temp_charset_work)
            temp_charset_work = xmalloc (sizeof *temp_charset_work);
          if (control_flag == 1)
            {
              memset (temp_charset_work->table.decoder, -1,
                      sizeof (int) * 0x10000);
            }
          else
            {
              memset (temp_charset_work->table.encoder, 0,
                      sizeof (unsigned short) * 0x20000);
              temp_charset_work->zero_index_char = -1;
            }
          temp_charset_work->current = charset;
          temp_charset_work->for_encoder = (control_flag == 2);
          control_flag += 2;
        }
      charset_map_loaded = 1;
    }

  min_char = max_char = entries->entry[0].c;
  nonascii_min_char = MAX_CHAR;
  for (i = 0; i < n_entries; i++)
    {
      unsigned from, to;
      int from_index, to_index, lim_index;
      int from_c, to_c;
      int idx = i % 0x10000;

      if (i > 0 && idx == 0)
        entries = entries->next;
      from = entries->entry[idx].from;
      to = entries->entry[idx].to;
      from_c = entries->entry[idx].c;
      from_index = CODE_POINT_TO_INDEX (charset, from);
      if (from == to)
        {
          to_index = from_index;
          to_c = from_c;
        }
      else
        {
          to_index = CODE_POINT_TO_INDEX (charset, to);
          to_c = from_c + (to_index - from_index);
        }
      if (from_index < 0 || to_index < 0)
        continue;
      lim_index = to_index + 1;

      if (to_c > max_char)
        max_char = to_c;
      else if (from_c < min_char)
        min_char = from_c;

      if (control_flag == 1)
        {
          if (charset->method == CHARSET_METHOD_MAP)
            for (; from_index < lim_index; from_index++, from_c++)
              ASET (vec, from_index, make_fixnum (from_c));
          else
            for (; from_index < lim_index; from_index++, from_c++)
              CHAR_TABLE_SET (Vchar_unify_table,
                              CHARSET_CODE_OFFSET (charset) + from_index,
                              make_fixnum (from_c));
        }
      else if (control_flag == 2)
        {
          if (charset->method == CHARSET_METHOD_MAP
              && CHARSET_COMPACT_CODES_P (charset))
            for (; from_index < lim_index; from_index++, from_c++)
              {
                unsigned code = from_index;
                code = INDEX_TO_CODE_POINT (charset, code);

                if (NILP (CHAR_TABLE_REF (table, from_c)))
                  CHAR_TABLE_SET (table, from_c, make_fixnum (code));
              }
          else
            for (; from_index < lim_index; from_index++, from_c++)
              {
                if (NILP (CHAR_TABLE_REF (table, from_c)))
                  CHAR_TABLE_SET (table, from_c, make_fixnum (from_index));
              }
        }
      else if (control_flag == 3)
        for (; from_index < lim_index; from_index++, from_c++)
          SET_TEMP_CHARSET_WORK_DECODER (from_c, from_index);
      else if (control_flag == 4)
        for (; from_index < lim_index; from_index++, from_c++)
          SET_TEMP_CHARSET_WORK_ENCODER (from_c, from_index);
      else
        {
          if (ascii_compatible_p)
            {
              if (!ASCII_CHAR_P (from_c))
                {
                  if (from_c < nonascii_min_char)
                    nonascii_min_char = from_c;
                }
              else if (!ASCII_CHAR_P (to_c))
                {
                  nonascii_min_char = 0x80;
                }
            }

          for (; from_c <= to_c; from_c++)
            CHARSET_FAST_MAP_SET (from_c, fast_map);
        }
    }

  if (control_flag == 0)
    {
      CHARSET_MIN_CHAR (charset)
        = (ascii_compatible_p ? nonascii_min_char : min_char);
      CHARSET_MAX_CHAR (charset) = max_char;
    }
  else if (control_flag == 4)
    {
      temp_charset_work->min_char = min_char;
      temp_charset_work->max_char = max_char;
    }
}

static unsigned
read_hex (FILE *fp, int lookahead, int *terminator, bool *overflow)
{
  int c = lookahead < 0 ? getc (fp) : lookahead;

  while (true)
    {
      if (c == '#')
        do
          c = getc (fp);
        while (0 <= c && c != '\n');
      else if (c == '0')
        {
          c = getc (fp);
          if (c < 0 || c == 'x')
            break;
        }
      if (c < 0)
        break;
      c = getc (fp);
    }

  int n = 0;
  bool v = false;

  if (0 <= c)
    while (true)
      {
        c = getc (fp);
        int digit = char_hexdigit (c);
        if (digit < 0)
          break;
        v |= INT_LEFT_SHIFT_OVERFLOW (n, (unsigned) 4);
        n = (n << 4) + digit;
      }

  *terminator = c;
  *overflow |= v;
  return (unsigned) n;
}

static void
load_charset_map_from_file (struct charset *charset, Lisp_Object mapfile,
                            int control_flag)
{
  unsigned min_code = CHARSET_MIN_CODE (charset);
  unsigned max_code = CHARSET_MAX_CODE (charset);
  int fd;
  FILE *fp;
  struct charset_map_entries *head, *entries;
  int n_entries;
  AUTO_STRING (map, ".map");
  AUTO_STRING (txt, ".txt");
  AUTO_LIST2 (suffixes, map, txt);
  specpdl_ref count = SPECPDL_INDEX ();
  record_unwind_protect_nothing ();
  specbind (Qfile_name_handler_alist, Qnil);
  fd = openp (Vcharset_map_path, mapfile, suffixes, NULL, Qnil, false, false,
              NULL);
  fp = fd < 0 ? 0 : emacs_fdopen (fd, "r");
  if (!fp)
    {
      // int open_errno = errno;
      emacs_close (fd);
      TODO; // report_file_errno ("Loading charset map", mapfile, open_errno);
    }
  set_unwind_protect_ptr (count, fclose_unwind, fp);
  unbind_to (specpdl_ref_add (count, 1), Qnil);

  head = record_xmalloc (sizeof *head);
  entries = head;
  memset (entries, 0, sizeof (struct charset_map_entries));

  n_entries = 0;
  int ch = -1;
  while (true)
    {
      bool overflow = false;
      unsigned from = read_hex (fp, ch, &ch, &overflow), to;
      if (ch < 0)
        break;
      if (ch == '-')
        {
          to = read_hex (fp, -1, &ch, &overflow);
          if (ch < 0)
            break;
        }
      else
        {
          to = from;
          ch = -1;
        }
      unsigned c = read_hex (fp, ch, &ch, &overflow);
      if (ch < 0)
        break;

      if (overflow)
        continue;
      if (from < min_code || to > max_code || from > to || c > MAX_CHAR)
        continue;

      if (n_entries == 0x10000)
        {
          entries->next = record_xmalloc (sizeof *entries->next);
          entries = entries->next;
          memset (entries, 0, sizeof (struct charset_map_entries));
          n_entries = 0;
        }
      int idx = n_entries;
      entries->entry[idx].from = from;
      entries->entry[idx].to = to;
      entries->entry[idx].c = c;
      n_entries++;
    }
  emacs_fclose (fp);
  clear_unwind_protect (count);

  load_charset_map (charset, head, n_entries, control_flag);
  unbind_to (count, Qnil);
}

static void
load_charset (struct charset *charset, int control_flag)
{
  Lisp_Object map;

  if (inhibit_load_charset_map && temp_charset_work
      && charset == temp_charset_work->current
      && ((control_flag == 2) == temp_charset_work->for_encoder))
    return;

  if (CHARSET_METHOD (charset) == CHARSET_METHOD_MAP)
    map = CHARSET_MAP (charset);
  else
    {
      if (!CHARSET_UNIFIED_P (charset))
        emacs_abort ();
      map = CHARSET_UNIFY_MAP (charset);
    }
  if (STRINGP (map))
    load_charset_map_from_file (charset, map, control_flag);
  else
    TODO; // load_charset_map_from_vector (charset, map, control_flag);
}

DEFUN ("set-charset-priority", Fset_charset_priority, Sset_charset_priority,
       1, MANY, 0,
       doc: /* Assign higher priority to the charsets given as arguments.
usage: (set-charset-priority &rest charsets)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  Lisp_Object new_head, old_list;
  Lisp_Object list_2022, list_emacs_mule;
  ptrdiff_t i;
  int id;

  old_list = Fcopy_sequence (Vcharset_ordered_list);
  new_head = Qnil;
  for (i = 0; i < nargs; i++)
    {
      CHECK_CHARSET_GET_ID (args[i], id);
      if (!NILP (Fmemq (make_fixnum (id), old_list)))
        {
          old_list = Fdelq (make_fixnum (id), old_list);
          new_head = Fcons (make_fixnum (id), new_head);
        }
    }
  Vcharset_non_preferred_head = old_list;
  Vcharset_ordered_list = nconc2 (Fnreverse (new_head), old_list);

  charset_ordered_list_tick++;

  charset_unibyte = -1;
  for (old_list = Vcharset_ordered_list, list_2022 = list_emacs_mule = Qnil;
       CONSP (old_list); old_list = XCDR (old_list))
    {
      if (!NILP (Fmemq (XCAR (old_list), Viso_2022_charset_list)))
        list_2022 = Fcons (XCAR (old_list), list_2022);
      if (!NILP (Fmemq (XCAR (old_list), Vemacs_mule_charset_list)))
        list_emacs_mule = Fcons (XCAR (old_list), list_emacs_mule);
      if (charset_unibyte < 0)
        {
          struct charset *charset = CHARSET_FROM_ID (XFIXNUM (XCAR (old_list)));

          if (CHARSET_DIMENSION (charset) == 1
              && CHARSET_ASCII_COMPATIBLE_P (charset)
              && CHARSET_MAX_CHAR (charset) >= 0x80)
            charset_unibyte = CHARSET_ID (charset);
        }
    }
  Viso_2022_charset_list = Fnreverse (list_2022);
  Vemacs_mule_charset_list = Fnreverse (list_emacs_mule);
  if (charset_unibyte < 0)
    charset_unibyte = charset_iso_8859_1;

  return Qnil;
}

DEFUN ("charsetp", Fcharsetp, Scharsetp, 1, 1, 0,
       doc: /* Return non-nil if and only if OBJECT is a charset.*/)
(Lisp_Object object) { return (CHARSETP (object) ? Qt : Qnil); }

static void
map_charset_for_dump (void (*c_function) (Lisp_Object, Lisp_Object),
                      Lisp_Object function, Lisp_Object arg, unsigned int from,
                      unsigned int to)
{
  int from_idx = CODE_POINT_TO_INDEX (temp_charset_work->current, from);
  int to_idx = CODE_POINT_TO_INDEX (temp_charset_work->current, to);
  Lisp_Object range = Fcons (Qnil, Qnil);
  int c, stop;

  c = temp_charset_work->min_char;
  stop = (temp_charset_work->max_char < 0x20000 ? temp_charset_work->max_char
                                                : 0xFFFF);

  while (1)
    {
      int idx = GET_TEMP_CHARSET_WORK_ENCODER (c);

      if (idx >= from_idx && idx <= to_idx)
        {
          if (NILP (XCAR (range)))
            XSETCAR (range, make_fixnum (c));
        }
      else if (!NILP (XCAR (range)))
        {
          XSETCDR (range, make_fixnum (c - 1));
          if (c_function)
            (*c_function) (arg, range);
          else
            call2 (function, range, arg);
          XSETCAR (range, Qnil);
        }
      if (c == stop)
        {
          if (c == temp_charset_work->max_char)
            {
              if (!NILP (XCAR (range)))
                {
                  XSETCDR (range, make_fixnum (c));
                  if (c_function)
                    (*c_function) (arg, range);
                  else
                    call2 (function, range, arg);
                }
              break;
            }
          c = 0x1FFFF;
          stop = temp_charset_work->max_char;
        }
      c++;
    }
}

void
map_charset_chars (void (*c_function) (Lisp_Object, Lisp_Object),
                   Lisp_Object function, Lisp_Object arg,
                   struct charset *charset, unsigned from, unsigned to)
{
  Lisp_Object range;
  bool partial
    = (from > CHARSET_MIN_CODE (charset) || to < CHARSET_MAX_CODE (charset));

  if (CHARSET_METHOD (charset) == CHARSET_METHOD_OFFSET)
    {
      int from_idx = CODE_POINT_TO_INDEX (charset, from);
      int to_idx = CODE_POINT_TO_INDEX (charset, to);
      int from_c = from_idx + CHARSET_CODE_OFFSET (charset);
      int to_c = to_idx + CHARSET_CODE_OFFSET (charset);

      if (CHARSET_UNIFIED_P (charset))
        {
          if (!CHAR_TABLE_P (CHARSET_DEUNIFIER (charset)))
            load_charset (charset, 2);
          if (CHAR_TABLE_P (CHARSET_DEUNIFIER (charset)))
            map_char_table_for_charset (c_function, function,
                                        CHARSET_DEUNIFIER (charset), arg,
                                        partial ? charset : NULL, from, to);
          else
            map_charset_for_dump (c_function, function, arg, from, to);
        }

      range = Fcons (make_fixnum (from_c), make_fixnum (to_c));
      if (NILP (function))
        (*c_function) (arg, range);
      else
        call2 (function, range, arg);
    }
  else if (CHARSET_METHOD (charset) == CHARSET_METHOD_MAP)
    {
      if (!CHAR_TABLE_P (CHARSET_ENCODER (charset)))
        load_charset (charset, 2);
      if (CHAR_TABLE_P (CHARSET_ENCODER (charset)))
        map_char_table_for_charset (c_function, function,
                                    CHARSET_ENCODER (charset), arg,
                                    partial ? charset : NULL, from, to);
      else
        map_charset_for_dump (c_function, function, arg, from, to);
    }
  else if (CHARSET_METHOD (charset) == CHARSET_METHOD_SUBSET)
    {
      Lisp_Object subset_info;
      int offset;

      subset_info = CHARSET_SUBSET (charset);
      charset = CHARSET_FROM_ID (XFIXNAT (AREF (subset_info, 0)));
      offset = XFIXNUM (AREF (subset_info, 3));
      from -= offset;
      if (from < XFIXNAT (AREF (subset_info, 1)))
        from = XFIXNAT (AREF (subset_info, 1));
      to -= offset;
      if (to > XFIXNAT (AREF (subset_info, 2)))
        to = XFIXNAT (AREF (subset_info, 2));
      map_charset_chars (c_function, function, arg, charset, from, to);
    }
  else
    {
      Lisp_Object parents;

      for (parents = CHARSET_SUPERSET (charset); CONSP (parents);
           parents = XCDR (parents))
        {
          int offset;
          unsigned this_from, this_to;

          charset = CHARSET_FROM_ID (XFIXNAT (XCAR (XCAR (parents))));
          offset = XFIXNUM (XCDR (XCAR (parents)));
          this_from = from > offset ? from - offset : 0;
          this_to = to > offset ? to - offset : 0;
          if (this_from < CHARSET_MIN_CODE (charset))
            this_from = CHARSET_MIN_CODE (charset);
          if (this_to > CHARSET_MAX_CODE (charset))
            this_to = CHARSET_MAX_CODE (charset);
          map_charset_chars (c_function, function, arg, charset, this_from,
                             this_to);
        }
    }
}

DEFUN ("map-charset-chars", Fmap_charset_chars, Smap_charset_chars, 2, 5, 0,
       doc: /* Call FUNCTION for all characters in CHARSET.
Optional 3rd argument ARG is an additional argument to be passed
to FUNCTION, see below.
Optional 4th and 5th arguments FROM-CODE and TO-CODE specify the
range of code points (in CHARSET) of target characters on which to
map the FUNCTION.  Note that these are not character codes, but code
points of CHARSET; for the difference see `decode-char' and
`list-charset-chars'.  If FROM-CODE is nil or imitted, it stands for
the first code point of CHARSET; if TO-CODE is nil or omitted, it
stands for the last code point of CHARSET.

FUNCTION will be called with two arguments: RANGE and ARG.
RANGE is a cons (FROM .  TO), where FROM and TO specify a range of
characters that belong to CHARSET on which FUNCTION should do its
job.  FROM and TO are Emacs character codes, unlike FROM-CODE and
TO-CODE, which are CHARSET code points.  */)
(Lisp_Object function, Lisp_Object charset, Lisp_Object arg,
 Lisp_Object from_code, Lisp_Object to_code)
{
  struct charset *cs;
  unsigned from, to;

  CHECK_CHARSET_GET_CHARSET (charset, cs);
  if (NILP (from_code))
    from = CHARSET_MIN_CODE (cs);
  else
    {
      from = XFIXNUM (from_code);
      if (from < CHARSET_MIN_CODE (cs))
        from = CHARSET_MIN_CODE (cs);
    }
  if (NILP (to_code))
    to = CHARSET_MAX_CODE (cs);
  else
    {
      to = XFIXNUM (to_code);
      if (to > CHARSET_MAX_CODE (cs))
        to = CHARSET_MAX_CODE (cs);
    }
  map_charset_chars (NULL, function, arg, cs, from, to);
  return Qnil;
}

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
      CHECK_CHARSET_GET_CHARSET (parent, parent_charset);
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
      val = Fcopy_sequence (val);
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
              CHECK_CHARSET_GET_ID (car_part, this_id);
              offset = check_integer_range (cdr_part, INT_MIN, INT_MAX);
            }
          else
            {
              CHECK_CHARSET_GET_ID (elt, this_id);
              offset = 0;
            }
          XSETCAR (val, Fcons (make_fixnum (this_id), make_fixnum (offset)));

          this_charset = CHARSET_FROM_ID (this_id);
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
    {
      load_charset (&charset, 0);
      charset_table[id] = charset;
    }

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

static int
maybe_unify_char (int c, Lisp_Object val)
{
  struct charset *charset;

  if (FIXNUMP (val))
    return XFIXNAT (val);
  if (NILP (val))
    return c;

  CHECK_CHARSET_GET_CHARSET (val, charset);
  load_charset (charset, 1);
  if (!inhibit_load_charset_map)
    {
      val = CHAR_TABLE_REF (Vchar_unify_table, c);
      if (!NILP (val))
        c = XFIXNAT (val);
    }
  else
    {
      int code_index = c - CHARSET_CODE_OFFSET (charset);
      int unified = GET_TEMP_CHARSET_WORK_DECODER (code_index);

      if (unified > 0)
        c = unified;
    }
  return c;
}

int
decode_char (struct charset *charset, unsigned int code)
{
  int c, char_index;
  enum charset_method method = CHARSET_METHOD (charset);

  if (code < CHARSET_MIN_CODE (charset) || code > CHARSET_MAX_CODE (charset))
    return -1;

  if (method == CHARSET_METHOD_SUBSET)
    {
      Lisp_Object subset_info;

      subset_info = CHARSET_SUBSET (charset);
      charset = CHARSET_FROM_ID (XFIXNAT (AREF (subset_info, 0)));
      code -= XFIXNUM (AREF (subset_info, 3));
      if (code < XFIXNAT (AREF (subset_info, 1))
          || code > XFIXNAT (AREF (subset_info, 2)))
        c = -1;
      else
        c = DECODE_CHAR (charset, code);
    }
  else if (method == CHARSET_METHOD_SUPERSET)
    {
      Lisp_Object parents;

      parents = CHARSET_SUPERSET (charset);
      c = -1;
      for (; CONSP (parents); parents = XCDR (parents))
        {
          int id = XFIXNUM (XCAR (XCAR (parents)));
          int code_offset = XFIXNUM (XCDR (XCAR (parents)));
          unsigned this_code = code - code_offset;

          charset = CHARSET_FROM_ID (id);
          if ((c = DECODE_CHAR (charset, this_code)) >= 0)
            break;
        }
    }
  else
    {
      char_index = CODE_POINT_TO_INDEX (charset, code);
      if (char_index < 0)
        return -1;

      if (method == CHARSET_METHOD_MAP)
        {
          Lisp_Object decoder;

          decoder = CHARSET_DECODER (charset);
          if (!VECTORP (decoder))
            {
              load_charset (charset, 1);
              decoder = CHARSET_DECODER (charset);
            }
          if (VECTORP (decoder))
            c = XFIXNUM (AREF (decoder, char_index));
          else
            c = GET_TEMP_CHARSET_WORK_DECODER (char_index);
        }
      else
        {
          c = char_index + CHARSET_CODE_OFFSET (charset);
          if (CHARSET_UNIFIED_P (charset) && MAX_UNICODE_CHAR < c
              && c <= MAX_5_BYTE_CHAR)
            {
              Lisp_Object val = CHAR_TABLE_REF (Vchar_unify_table, c);
              c = maybe_unify_char (c, val);
            }
        }
    }

  return c;
}

Lisp_Object charset_work;

unsigned
encode_char (struct charset *charset, int c)
{
  unsigned code;
  enum charset_method method = CHARSET_METHOD (charset);

  if (CHARSET_UNIFIED_P (charset))
    {
      Lisp_Object deunifier;
      int code_index = -1;

      deunifier = CHARSET_DEUNIFIER (charset);
      if (!CHAR_TABLE_P (deunifier))
        {
          load_charset (charset, 2);
          deunifier = CHARSET_DEUNIFIER (charset);
        }
      if (CHAR_TABLE_P (deunifier))
        {
          Lisp_Object deunified = CHAR_TABLE_REF (deunifier, c);

          if (FIXNUMP (deunified))
            code_index = XFIXNUM (deunified);
        }
      else
        {
          code_index = GET_TEMP_CHARSET_WORK_ENCODER (c);
        }
      if (code_index >= 0)
        c = CHARSET_CODE_OFFSET (charset) + code_index;
    }

  if (method == CHARSET_METHOD_SUBSET)
    {
      Lisp_Object subset_info;
      struct charset *this_charset;

      subset_info = CHARSET_SUBSET (charset);
      this_charset = CHARSET_FROM_ID (XFIXNAT (AREF (subset_info, 0)));
      code = ENCODE_CHAR (this_charset, c);
      if (code == CHARSET_INVALID_CODE (this_charset)
          || code < XFIXNAT (AREF (subset_info, 1))
          || code > XFIXNAT (AREF (subset_info, 2)))
        return CHARSET_INVALID_CODE (charset);
      code += XFIXNUM (AREF (subset_info, 3));
      return code;
    }

  if (method == CHARSET_METHOD_SUPERSET)
    {
      Lisp_Object parents;

      parents = CHARSET_SUPERSET (charset);
      for (; CONSP (parents); parents = XCDR (parents))
        {
          int id = XFIXNUM (XCAR (XCAR (parents)));
          int code_offset = XFIXNUM (XCDR (XCAR (parents)));
          struct charset *this_charset = CHARSET_FROM_ID (id);

          code = ENCODE_CHAR (this_charset, c);
          if (code != CHARSET_INVALID_CODE (this_charset))
            return code + code_offset;
        }
      return CHARSET_INVALID_CODE (charset);
    }

  if (!CHARSET_FAST_MAP_REF (c, charset->fast_map)
      || c < CHARSET_MIN_CHAR (charset) || c > CHARSET_MAX_CHAR (charset))
    return CHARSET_INVALID_CODE (charset);

  if (method == CHARSET_METHOD_MAP)
    {
      Lisp_Object encoder;
      Lisp_Object val;

      encoder = CHARSET_ENCODER (charset);
      if (!CHAR_TABLE_P (CHARSET_ENCODER (charset)))
        {
          load_charset (charset, 2);
          encoder = CHARSET_ENCODER (charset);
        }
      if (CHAR_TABLE_P (encoder))
        {
          val = CHAR_TABLE_REF (encoder, c);
          if (NILP (val))
            return CHARSET_INVALID_CODE (charset);
          code = XFIXNUM (val);
          if (!CHARSET_COMPACT_CODES_P (charset))
            code = INDEX_TO_CODE_POINT (charset, code);
        }
      else
        {
          code = GET_TEMP_CHARSET_WORK_ENCODER (c);
          code = INDEX_TO_CODE_POINT (charset, code);
        }
    }
  else
    {
      unsigned code_index = c - CHARSET_CODE_OFFSET (charset);

      code = INDEX_TO_CODE_POINT (charset, code_index);
    }

  return code;
}

DEFUN ("unify-charset", Funify_charset, Sunify_charset, 1, 3, 0,
       doc: /* Unify characters of CHARSET with Unicode.
This means reading the relevant file and installing the table defined
by CHARSET's `:unify-map' property.

Optional second arg UNIFY-MAP is a file name string or a vector.  It has
the same meaning as the `:unify-map' attribute in the function
`define-charset' (which see).

Optional third argument DEUNIFY, if non-nil, means to de-unify CHARSET.  */)
(Lisp_Object charset, Lisp_Object unify_map, Lisp_Object deunify)
{
  int id;
  struct charset *cs;

  CHECK_CHARSET_GET_ID (charset, id);
  cs = CHARSET_FROM_ID (id);
  if (NILP (deunify) ? CHARSET_UNIFIED_P (cs) && !NILP (CHARSET_DEUNIFIER (cs))
                     : !CHARSET_UNIFIED_P (cs))
    return Qnil;

  CHARSET_UNIFIED_P (cs) = 0;
  if (NILP (deunify))
    {
      if (CHARSET_METHOD (cs) != CHARSET_METHOD_OFFSET
          || CHARSET_CODE_OFFSET (cs) < 0x110000)
        error ("Can't unify charset: %s", SDATA (SYMBOL_NAME (charset)));
      if (NILP (unify_map))
        unify_map = CHARSET_UNIFY_MAP (cs);
      else
        {
          if (!STRINGP (unify_map) && !VECTORP (unify_map))
            signal_error ("Bad unify-map", unify_map);
          set_charset_attr (cs, charset_unify_map, unify_map);
        }
      if (NILP (Vchar_unify_table))
        Vchar_unify_table = Fmake_char_table (Qnil, Qnil);
      char_table_set_range (Vchar_unify_table, cs->min_char, cs->max_char,
                            charset);
      CHARSET_UNIFIED_P (cs) = 1;
    }
  else if (CHAR_TABLE_P (Vchar_unify_table))
    {
      unsigned min_code = CHARSET_MIN_CODE (cs);
      unsigned max_code = CHARSET_MAX_CODE (cs);
      int min_char = DECODE_CHAR (cs, min_code);
      int max_char = DECODE_CHAR (cs, max_code);

      char_table_set_range (Vchar_unify_table, min_char, max_char, Qnil);
    }

  return Qnil;
}

DEFUN ("decode-char", Fdecode_char, Sdecode_char, 2, 2, 0,
       doc: /* Decode the pair of CHARSET and CODE-POINT into a character.
Return nil if CODE-POINT is not valid in CHARSET.

CODE-POINT may be a cons (HIGHER-16-BIT-VALUE . LOWER-16-BIT-VALUE),
although this usage is obsolescent.  */)
(Lisp_Object charset, Lisp_Object code_point)
{
  int c, id;
  unsigned code;
  struct charset *charsetp;

  CHECK_CHARSET_GET_ID (charset, id);
  code = cons_to_unsigned (code_point, UINT_MAX);
  charsetp = CHARSET_FROM_ID (id);
  c = DECODE_CHAR (charsetp, code);
  return (c >= 0 ? make_fixnum (c) : Qnil);
}

DEFUN ("encode-char", Fencode_char, Sencode_char, 2, 2, 0,
       doc: /* Encode the character CH into a code-point of CHARSET.
Return the encoded code-point as an integer,
or nil if CHARSET doesn't support CH.  */)
(Lisp_Object ch, Lisp_Object charset)
{
  int c, id;
  unsigned code;
  struct charset *charsetp;

  CHECK_CHARSET_GET_ID (charset, id);
  CHECK_CHARACTER (ch);
  c = XFIXNAT (ch);
  charsetp = CHARSET_FROM_ID (id);
  code = ENCODE_CHAR (charsetp, c);
  if (code == CHARSET_INVALID_CODE (charsetp))
    return Qnil;
  return INT_TO_INTEGER (code);
}

void
mark_charset (void)
{
  for (int i = 0; i < charset_table_used; i++)
    mark_object (charset_table[i].attributes);
}

void
init_charset (void)
{
  Lisp_Object tempdir;
  tempdir = Fexpand_file_name (build_string ("charsets"), Vdata_directory);
  if (!file_accessible_directory_p (tempdir))
    {
      TODO; // fprintf (stderr,
      //          ("Error: %s: %s\n"
      //           "Emacs will not function correctly "
      //           "without the character map files.\n"
      //           "%s"
      //           "Please check your installation!\n"),
      //          SDATA (tempdir), strerror (errno),
      //          (egetenv ("EMACSDATA")
      //             ? ("The EMACSDATA environment variable is set.  "
      //                "Maybe it has the wrong value?\n")
      //             : ""));
      // exit (1);
    }

  Vcharset_map_path = list1 (tempdir);
}

void
init_charset_once (void)
{
  TODO_NELISP_LATER;

  int i, j, k;

  for (i = 0; i < ISO_MAX_DIMENSION; i++)
    for (j = 0; j < ISO_MAX_CHARS; j++)
      for (k = 0; k < ISO_MAX_FINAL; k++)
        iso_charset_table[i][j][k] = -1;

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

  defsubr (&Sset_charset_priority);
  defsubr (&Scharsetp);
  defsubr (&Smap_charset_chars);
  defsubr (&Sdefine_charset_internal);
  defsubr (&Sdefine_charset_alias);
  defsubr (&Scharset_plist);
  defsubr (&Sset_charset_plist);
  defsubr (&Sunify_charset);
  defsubr (&Sdecode_char);
  defsubr (&Sencode_char);

  DEFVAR_LISP ("charset-map-path", Vcharset_map_path,
        doc: /* List of directories to search for charset map files.  */);
  Vcharset_map_path = Qnil;

  DEFVAR_BOOL ("inhibit-load-charset-map", inhibit_load_charset_map,
        doc: /* Inhibit loading of charset maps.  Used when dumping Emacs.  */);
  inhibit_load_charset_map = 0;

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
