#include <math.h>
#include <stdlib.h>
#include "lisp.h"
#include "character.h"
#include "coding.h"
#include "termhooks.h"

#include <errno.h>
#include <fcntl.h>

static Lisp_Object read_objects_map;
static Lisp_Object read_objects_completed;

struct Lisp_Symbol lispsym[];

static Lisp_Object initial_obarray;
static size_t oblookup_last_bucket_number;

static struct infile
{
  FILE *stream;

  signed char lookahead;

  unsigned char buf[MAX_MULTIBYTE_LENGTH - 1];
} *infile;

static int unread_char = -1;

static void readevalloop (Lisp_Object, struct infile *, Lisp_Object, bool,
                          Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);

Lisp_Object
check_obarray_slow (Lisp_Object obarray)
{
  UNUSED (obarray);
  TODO;
  return NULL;
}

INLINE Lisp_Object
check_obarray (Lisp_Object obarray)
{
  return OBARRAYP (obarray) ? obarray : check_obarray_slow (obarray);
}

static ptrdiff_t
obarray_index (struct Lisp_Obarray *oa, const char *str, ptrdiff_t size_byte)
{
  EMACS_UINT hash = hash_string (str, size_byte);
  return knuth_hash (reduce_emacs_uint_to_hash_hash (hash), oa->size_bits);
}

Lisp_Object
oblookup (Lisp_Object obarray, register const char *ptr, ptrdiff_t size,
          ptrdiff_t size_byte)
{
  struct Lisp_Obarray *o = XOBARRAY (obarray);
  ptrdiff_t idx = obarray_index (o, ptr, size_byte);
  Lisp_Object bucket = o->buckets[idx];

  oblookup_last_bucket_number = idx;
  if (!BASE_EQ (bucket, make_fixnum (0)))
    {
      Lisp_Object sym = bucket;
      while (1)
        {
          struct Lisp_Symbol *s = XBARE_SYMBOL (sym);
          Lisp_Object name = s->u.s.name;
          if (SBYTES (name) == size_byte && SCHARS (name) == size
              && memcmp (SDATA (name), ptr, size_byte) == 0)
            return sym;
          if (s->u.s.next == NULL)
            break;
          sym = make_lisp_symbol (s->u.s.next);
        }
    }
  return make_fixnum (idx);
}

enum
{
  obarray_default_bits = 3,
  word_size_log2 = word_size < 8 ? 5 : 6,
  obarray_max_bits
  = min (8 * sizeof (int), 8 * sizeof (ptrdiff_t) - word_size_log2) - 1,
};

static void
grow_obarray (struct Lisp_Obarray *o)
{
  ptrdiff_t old_size = obarray_size (o);
  eassert (o->count > old_size);
  Lisp_Object *old_buckets = o->buckets;

  int new_bits = o->size_bits + 1;
  if (new_bits > obarray_max_bits)
    error ("Obarray too big");
  ptrdiff_t new_size = (ptrdiff_t) 1 << new_bits;
  o->buckets = hash_table_alloc_bytes (new_size * sizeof *o->buckets);
  for (ptrdiff_t i = 0; i < new_size; i++)
    o->buckets[i] = make_fixnum (0);
  o->size_bits = new_bits;

  for (ptrdiff_t i = 0; i < old_size; i++)
    {
      Lisp_Object obj = old_buckets[i];
      if (BARE_SYMBOL_P (obj))
        {
          struct Lisp_Symbol *s = XBARE_SYMBOL (obj);
          while (1)
            {
              Lisp_Object name = s->u.s.name;
              ptrdiff_t idx = obarray_index (o, SSDATA (name), SBYTES (name));
              Lisp_Object *loc = o->buckets + idx;
              struct Lisp_Symbol *next = s->u.s.next;
              s->u.s.next = BARE_SYMBOL_P (*loc) ? XBARE_SYMBOL (*loc) : NULL;
              *loc = make_lisp_symbol (s);
              if (next == NULL)
                break;
              s = next;
            }
        }
    }

  hash_table_free_bytes (old_buckets, old_size * sizeof *old_buckets);
}

static Lisp_Object
intern_sym (Lisp_Object sym, Lisp_Object obarray, Lisp_Object index)
{
  eassert (BARE_SYMBOL_P (sym) && OBARRAYP (obarray) && FIXNUMP (index));
  struct Lisp_Symbol *s = XBARE_SYMBOL (sym);
  s->u.s.interned
    = (BASE_EQ (obarray, initial_obarray) ? SYMBOL_INTERNED_IN_INITIAL_OBARRAY
                                          : SYMBOL_INTERNED);

  if (SREF (s->u.s.name, 0) == ':' && BASE_EQ (obarray, initial_obarray))
    {
      s->u.s.trapped_write = SYMBOL_NOWRITE;
      s->u.s.redirect = SYMBOL_PLAINVAL;
      s->u.s.declared_special = true;
      SET_SYMBOL_VAL (s, sym);
    }

  struct Lisp_Obarray *o = XOBARRAY (obarray);
  Lisp_Object *ptr = o->buckets + XFIXNUM (index);
  s->u.s.next = BARE_SYMBOL_P (*ptr) ? XBARE_SYMBOL (*ptr) : NULL;
  *ptr = sym;
  o->count++;
  if (o->count > obarray_size (o))
    grow_obarray (o);
  return sym;
}

static struct Lisp_Obarray *
allocate_obarray (void)
{
  return ALLOCATE_PLAIN_PSEUDOVECTOR (struct Lisp_Obarray, PVEC_OBARRAY);
}

static Lisp_Object
make_obarray (unsigned bits)
{
  struct Lisp_Obarray *o = allocate_obarray ();
  o->count = 0;
  o->size_bits = bits;
  ptrdiff_t size = (ptrdiff_t) 1 << bits;
  o->buckets = hash_table_alloc_bytes (size * sizeof *o->buckets);
  for (ptrdiff_t i = 0; i < size; i++)
    o->buckets[i] = make_fixnum (0);
  return make_lisp_obarray (o);
}

DEFUN ("obarray-make", Fobarray_make, Sobarray_make, 0, 1, 0,
       doc: /* Return a new obarray of size SIZE.
The obarray will grow to accommodate any number of symbols; the size, if
given, is only a hint for the expected number.  */)
(Lisp_Object size)
{
  int bits;
  if (NILP (size))
    bits = obarray_default_bits;
  else
    {
      CHECK_FIXNAT (size);
      EMACS_UINT n = XFIXNUM (size);
      bits = elogb (n) + 1;
      if (bits > obarray_max_bits)
        xsignal (Qargs_out_of_range, size);
    }
  return make_obarray (bits);
}

static void
define_symbol (Lisp_Object sym, char const *str)
{
  ptrdiff_t len = strlen (str);
  Lisp_Object string = make_pure_c_string (str, len);
  init_symbol (sym, string);

  if (!BASE_EQ (sym, Qunbound))
    {
      Lisp_Object bucket = oblookup (initial_obarray, str, len, len);
      eassert (FIXNUMP (bucket));
      intern_sym (sym, initial_obarray, bucket);
    }
}

void
init_obarray_once (void)
{
  Vobarray = make_obarray (15);
  initial_obarray = Vobarray;
  staticpro (&initial_obarray);

  for (unsigned long i = 0; i < ARRAYELTS (lispsym); i++)
    define_symbol (builtin_lisp_symbol (i), defsym_name[i]);

  DEFSYM (Qunbound, "unbound");

  DEFSYM (Qnil, "nil");
  SET_SYMBOL_VAL (XBARE_SYMBOL (Qnil), Qnil);
  make_symbol_constant (Qnil);
  XBARE_SYMBOL (Qnil)->u.s.declared_special = true;

  DEFSYM (Qt, "t");
  SET_SYMBOL_VAL (XBARE_SYMBOL (Qt), Qt);
  make_symbol_constant (Qt);
  XBARE_SYMBOL (Qt)->u.s.declared_special = true;

#if TODO_NELISP_LATER_AND
  Vpurify_flag = Qt;
#endif

  DEFSYM (Qvariable_documentation, "variable-documentation");
}

Lisp_Object
intern_driver (Lisp_Object string, Lisp_Object obarray, Lisp_Object index)
{
  SET_SYMBOL_VAL (XBARE_SYMBOL (Qobarray_cache), Qnil);
  return intern_sym (Fmake_symbol (string), obarray, index);
}

Lisp_Object
intern_1 (const char *str, ptrdiff_t len)
{
  Lisp_Object obarray = check_obarray (Vobarray);
  Lisp_Object tem = oblookup (obarray, str, len, len);

  return (BARE_SYMBOL_P (tem)
            ? tem
            : intern_driver (make_unibyte_string (str, len), obarray, tem));
}

Lisp_Object
intern_c_string_1 (const char *str, ptrdiff_t len)
{
  Lisp_Object obarray = check_obarray (Vobarray);
  Lisp_Object tem = oblookup (obarray, str, len, len);

  if (!BARE_SYMBOL_P (tem))
    {
      Lisp_Object string;

#if TODO_NELISP_LATER_AND
      if (NILP (Vpurify_flag))
        string = make_string (str, len);
      else
        string = make_pure_c_string (str, len);
#else
      string = make_pure_c_string (str, len);
#endif

      tem = intern_driver (string, obarray, tem);
    }
  return tem;
}

DEFUN ("get-load-suffixes", Fget_load_suffixes, Sget_load_suffixes, 0, 0, 0,
       doc: /* Return the suffixes that `load' should try if a suffix is \
required.
This uses the variables `load-suffixes' and `load-file-rep-suffixes'.  */)
(void)
{
  Lisp_Object lst = Qnil, suffixes = Vload_suffixes;
  FOR_EACH_TAIL (suffixes)
    {
      Lisp_Object exts = Vload_file_rep_suffixes;
      Lisp_Object suffix = XCAR (suffixes);
      FOR_EACH_TAIL (exts)
        lst = Fcons (concat2 (suffix, XCAR (exts)), lst);
    }
  return Fnreverse (lst);
}

bool
suffix_p (Lisp_Object string, const char *suffix)
{
  ptrdiff_t suffix_len = strlen (suffix);
  ptrdiff_t string_len = SBYTES (string);

  return (suffix_len <= string_len
          && strcmp (SSDATA (string) + string_len - suffix_len, suffix) == 0);
}

static bool
complete_filename_p (Lisp_Object pathname)
{
  const unsigned char *s = SDATA (pathname);
  return (IS_DIRECTORY_SEP (s[0])
          || (SCHARS (pathname) > 2 && IS_DEVICE_SEP (s[1])
              && IS_DIRECTORY_SEP (s[2])));
}

int
openp (Lisp_Object path, Lisp_Object str, Lisp_Object suffixes,
       Lisp_Object *storeptr, Lisp_Object predicate, bool newer, bool no_native,
       void **platform)
{
  UNUSED (platform);
  UNUSED (no_native);
  TODO_NELISP_LATER;

  ptrdiff_t fn_size = 100;
  char buf[100];
  char *fn = buf;
  Lisp_Object filename;
  Lisp_Object tail, string, encoded_fn;
  ptrdiff_t max_suffix_len = 0;
  ptrdiff_t want_length;
  bool absolute;
  int last_errno = ENOENT;
  USE_SAFE_ALLOCA;

  CHECK_STRING (str);

  tail = suffixes;
  FOR_EACH_TAIL_SAFE (tail)
    {
      CHECK_STRING_CAR (tail);
      max_suffix_len = max (max_suffix_len, SBYTES (XCAR (tail)));
    }

  AUTO_LIST1 (just_use_str, Qnil);
  if (NILP (path))
    path = just_use_str;

  absolute = complete_filename_p (str);

  FOR_EACH_TAIL_SAFE (path)
    {
      ptrdiff_t baselen, prefixlen;

      if (EQ (path, just_use_str))
        filename = str;
      else
        filename = Fexpand_file_name (str, XCAR (path));

      if (!complete_filename_p (filename))
        TODO;

      want_length = max_suffix_len + SBYTES (filename);
      if (fn_size <= want_length)
        {
          fn_size = 100 + want_length;
          fn = SAFE_ALLOCA (fn_size);
        }

      prefixlen = ((SCHARS (filename) > 2 && SREF (filename, 0) == '/'
                    && SREF (filename, 1) == ':')
                     ? 2
                     : 0);
      baselen = SBYTES (filename) - prefixlen;
      memcpy (fn, SDATA (filename) + prefixlen, baselen);
      AUTO_LIST1 (empty_string_only, empty_unibyte_string);
      tail = NILP (suffixes) ? empty_string_only : suffixes;
      FOR_EACH_TAIL_SAFE (tail)
        {
          Lisp_Object suffix = XCAR (tail);
          ptrdiff_t fnlen, lsuffix = SBYTES (suffix);
          memcpy (fn + baselen, SDATA (suffix), lsuffix + 1);
          fnlen = baselen + lsuffix;
          if (!STRING_MULTIBYTE (filename) && !STRING_MULTIBYTE (suffix))
            string = make_unibyte_string (fn, fnlen);
          else
            string = make_string (fn, fnlen);
          if (((!NILP (predicate) && !EQ (predicate, Qt)))
              && !FIXNATP (predicate))
            TODO;
          else
            {
              int fd;
              const char *pfn;

              encoded_fn = ENCODE_FILE (string);
              pfn = SSDATA (encoded_fn);
              struct stat st;

              if (FIXNATP (predicate))
                TODO;
              else
                {
                  fd = emacs_open (pfn, O_RDONLY, 0);

                  if (fd < 0)
                    {
                      if (!(errno == ENOENT || errno == ENOTDIR))
                        last_errno = errno;
                    }
                  else
                    {
                      int err = (fstat (fd, &st) != 0   ? errno
                                 : S_ISDIR (st.st_mode) ? EISDIR
                                                        : 0);
                      if (err)
                        {
                          last_errno = err;
                          emacs_close (fd);
                          fd = -1;
                        }
                    }
                }
              if (fd >= 0)
                {
                  if (newer && !FIXNATP (predicate))
                    TODO;
                  else
                    {
                      if (storeptr)
                        *storeptr = string;
                      SAFE_FREE ();
                      return fd;
                    }
                }
            }
        }
      if (absolute)
        break;
    }
  SAFE_FREE ();
  errno = last_errno;
  return -1;
}

void
close_file_unwind (int fd)
{
  emacs_close (fd);
}

static void
close_infile_unwind (void *arg)
{
  struct infile *prev_infile = arg;
  eassert (infile && infile != prev_infile);
  fclose (infile->stream);
  infile = prev_infile;
}

DEFUN ("load", Fload, Sload, 1, 5, 0,
       doc: /* Execute a file of Lisp code named FILE.
First try FILE with `.elc' appended, then try with `.el', then try
with a system-dependent suffix of dynamic modules (see `load-suffixes'),
then try FILE unmodified (the exact suffixes in the exact order are
determined by `load-suffixes').  Environment variable references in
FILE are replaced with their values by calling `substitute-in-file-name'.
This function searches the directories in `load-path'.

If optional second arg NOERROR is non-nil,
report no error if FILE doesn't exist.
Print messages at start and end of loading unless
optional third arg NOMESSAGE is non-nil (but `force-load-messages'
overrides that).
If optional fourth arg NOSUFFIX is non-nil, don't try adding
suffixes to the specified name FILE.
If optional fifth arg MUST-SUFFIX is non-nil, insist on
the suffix `.elc' or `.el' or the module suffix; don't accept just
FILE unless it ends in one of those suffixes or includes a directory name.

If NOSUFFIX is nil, then if a file could not be found, try looking for
a different representation of the file by adding non-empty suffixes to
its name, before trying another file.  Emacs uses this feature to find
compressed versions of files when Auto Compression mode is enabled.
If NOSUFFIX is non-nil, disable this feature.

The suffixes that this function tries out, when NOSUFFIX is nil, are
given by the return value of `get-load-suffixes' and the values listed
in `load-file-rep-suffixes'.  If MUST-SUFFIX is non-nil, only the
return value of `get-load-suffixes' is used, i.e. the file name is
required to have a non-empty suffix.

When searching suffixes, this function normally stops at the first
one that exists.  If the option `load-prefer-newer' is non-nil,
however, it tries all suffixes, and uses whichever file is the newest.

Loading a file records its definitions, and its `provide' and
`require' calls, in an element of `load-history' whose
car is the file name loaded.  See `load-history'.

While the file is in the process of being loaded, the variable
`load-in-progress' is non-nil and the variable `load-file-name'
is bound to the file's name.

Return t if the file exists and loads successfully.  */)
(Lisp_Object file, Lisp_Object noerror, Lisp_Object nomessage,
 Lisp_Object nosuffix, Lisp_Object must_suffix)
{
  UNUSED (noerror);
  UNUSED (nomessage);
  TODO_NELISP_LATER;

  FILE *stream UNINIT;
  specpdl_ref fd_index UNINIT;
  int fd;
  Lisp_Object found;
  const char *fmode = "r";
  specpdl_ref count = SPECPDL_INDEX ();

  CHECK_STRING (file);

  bool no_native = suffix_p (file, ".elc");

  if (SCHARS (file) == 0)
    {
      fd = -1;
      errno = ENOENT;
    }
  else
    {
      Lisp_Object suffixes;
      found = Qnil;

      if (!NILP (must_suffix))
        TODO;

      if (!NILP (nosuffix))
        suffixes = Qnil;
      else
        suffixes
          = list2 (build_pure_c_string (".elc"), build_pure_c_string (".el"));
      fd = openp (Vload_path, file, suffixes, &found, Qnil, false, no_native,
                  NULL);
    }

  if (fd == -1)
    {
      if (NILP (noerror))
        TODO;
      return Qnil;
    }

  if (fd == 2)
    TODO;

  if (0 <= fd)
    {
      fd_index = SPECPDL_INDEX ();
      record_unwind_protect_int (close_file_unwind, fd);
    }

  specbind (Qlexical_binding, Qnil);

  specbind (Qlread_unescaped_character_literals, Qnil);

  if (!(fd >= 0))
    {
      stream = NULL;
      errno = EINVAL;
    }
  else
    {
      stream = emacs_fdopen (fd, fmode);
    }

  struct infile input;

  if (!stream)
    TODO;
  set_unwind_protect_ptr (fd_index, close_infile_unwind, infile);
  input.stream = stream;
  input.lookahead = 0;
  infile = &input;
  unread_char = -1;

  if (lisp_file_lexical_cookie (Qget_file_char) == Cookie_Lex)
    Fset (Qlexical_binding, Qt);

  Lisp_Object hist_file_name = Qnil;

  readevalloop (Qget_file_char, &input, hist_file_name, 0, Qnil, Qnil, Qnil,
                Qnil);

  unbind_to (count, Qnil);
  return Qt;
}

DEFUN ("locate-file-internal", Flocate_file_internal, Slocate_file_internal, 2, 4, 0,
       doc: /* Search for FILENAME through PATH.
Returns the file's name in absolute form, or nil if not found.
If SUFFIXES is non-nil, it should be a list of suffixes to append to
file name when searching.
If non-nil, PREDICATE is used instead of `file-readable-p'.
PREDICATE can also be an integer to pass to the faccessat(2) function,
in which case file-name-handlers are ignored.
This function will normally skip directories, so if you want it to find
directories, make sure the PREDICATE function returns `dir-ok' for them.  */)
(Lisp_Object filename, Lisp_Object path, Lisp_Object suffixes,
 Lisp_Object predicate)
{
  Lisp_Object file;
  int fd
    = openp (path, filename, suffixes, &file, predicate, false, true, NULL);
  if (NILP (predicate) && fd >= 0)
    emacs_close (fd);
  return file;
}

static Lisp_Object read_internal_start (Lisp_Object, Lisp_Object, Lisp_Object,
                                        bool);
DEFUN ("read", Fread, Sread, 0, 1, 0,
       doc: /* Read one Lisp expression as text from STREAM, return as Lisp object.
If STREAM is nil, use the value of `standard-input' (which see).
STREAM or the value of `standard-input' may be:
 a buffer (read from point and advance it)
 a marker (read from where it points and advance it)
 a function (call it with no arguments for each character,
     call it with a char as argument to push a char back)
 a string (takes text from string, starting at the beginning)
 t (read text line using minibuffer and use it, or read from
    standard input in batch mode).  */)
(Lisp_Object stream)
{
  if (NILP (stream))
    TODO; // stream = Vstandard_input;
  if (EQ (stream, Qt))
    TODO; // stream = Qread_char;
#if TODO_NELISP_LATER_AND
  if (EQ (stream, Qread_char))
    return call1 (Qread_minibuffer, build_string ("Lisp expression: "));
#endif

  return read_internal_start (stream, Qnil, Qnil, false);
}

DEFUN ("intern", Fintern, Sintern, 1, 2, 0,
       doc: /* Return the canonical symbol whose name is STRING.
If there is none, one is created by this function and returned.
A second optional argument specifies the obarray to use;
it defaults to the value of `obarray'.  */)
(Lisp_Object string, Lisp_Object obarray)
{
  Lisp_Object tem;

  obarray = check_obarray (NILP (obarray) ? Vobarray : obarray);
  CHECK_STRING (string);

#if TODO_NELISP_LATER_AND
  char *longhand = NULL;
  ptrdiff_t longhand_chars = 0;
  ptrdiff_t longhand_bytes = 0;
  tem
    = oblookup_considering_shorthand (obarray, SSDATA (string), SCHARS (string),
                                      SBYTES (string), &longhand,
                                      &longhand_chars, &longhand_bytes);

  if (!BARE_SYMBOL_P (tem))
    {
      if (longhand)
        {
          tem = intern_driver (make_specified_string (longhand, longhand_chars,
                                                      longhand_bytes, true),
                               obarray, tem);
          xfree (longhand);
        }
      else
        tem = intern_driver (NILP (Vpurify_flag) ? string : Fpurecopy (string),
                             obarray, tem);
    }
#else
  tem = oblookup (obarray, SSDATA (string), SCHARS (string), SBYTES (string));
  if (!BARE_SYMBOL_P (tem))
    tem = intern_driver (string, obarray, tem);
#endif
  return tem;
}
DEFUN ("unintern", Funintern, Sunintern, 1, 2, 0,
       doc: /* Delete the symbol named NAME, if any, from OBARRAY.
The value is t if a symbol was found and deleted, nil otherwise.
NAME may be a string or a symbol.  If it is a symbol, that symbol
is deleted, if it belongs to OBARRAY--no other symbol is deleted.
OBARRAY, if nil, defaults to the value of the variable `obarray'.
usage: (unintern NAME OBARRAY)  */)
(Lisp_Object name, Lisp_Object obarray)
{
  register Lisp_Object tem;
  Lisp_Object string;

  if (NILP (obarray))
    obarray = Vobarray;
  obarray = check_obarray (obarray);

  if (SYMBOLP (name))
    {
      if (!BARE_SYMBOL_P (name))
        name = XSYMBOL_WITH_POS (name)->sym;
      string = SYMBOL_NAME (name);
    }
  else
    {
      CHECK_STRING (name);
      string = name;
    }

#if TODO_NELISP_LATER_AND
  char *longhand = NULL;
  ptrdiff_t longhand_chars = 0;
  ptrdiff_t longhand_bytes = 0;
  tem
    = oblookup_considering_shorthand (obarray, SSDATA (string), SCHARS (string),
                                      SBYTES (string), &longhand,
                                      &longhand_chars, &longhand_bytes);
  if (longhand)
    xfree (longhand);
#endif
  tem = oblookup (obarray, SSDATA (string), SCHARS (string), SBYTES (string));

  if (FIXNUMP (tem))
    return Qnil;
  if (BARE_SYMBOL_P (name) && !BASE_EQ (name, tem))
    return Qnil;

  struct Lisp_Symbol *sym = XBARE_SYMBOL (tem);
  sym->u.s.interned = SYMBOL_UNINTERNED;

  ptrdiff_t idx = oblookup_last_bucket_number;
  Lisp_Object *loc = &XOBARRAY (obarray)->buckets[idx];

  eassert (BARE_SYMBOL_P (*loc));
  struct Lisp_Symbol *prev = XBARE_SYMBOL (*loc);
  if (sym == prev)
    *loc = sym->u.s.next ? make_lisp_symbol (sym->u.s.next) : make_fixnum (0);
  else
    {
      while (1)
        {
          struct Lisp_Symbol *next = prev->u.s.next;
          if (next == sym)
            {
              prev->u.s.next = next->u.s.next;
              break;
            }
          prev = next;
        }
    }

  XOBARRAY (obarray)->count--;

  return Qt;
}

void
defvar_lisp_nopro (struct Lisp_Objfwd const *o_fwd, char const *namestring)
{
  Lisp_Object sym = intern_c_string (namestring);
  XBARE_SYMBOL (sym)->u.s.declared_special = true;
  XBARE_SYMBOL (sym)->u.s.redirect = SYMBOL_FORWARDED;
  SET_SYMBOL_FWD (XBARE_SYMBOL (sym), o_fwd);
}
void
defvar_lisp (struct Lisp_Objfwd const *o_fwd, char const *namestring)
{
  defvar_lisp_nopro (o_fwd, namestring);
  staticpro (o_fwd->objvar);
}
void
defvar_int (struct Lisp_Intfwd const *i_fwd, char const *namestring)
{
  Lisp_Object sym = intern_c_string (namestring);
  XBARE_SYMBOL (sym)->u.s.declared_special = true;
  XBARE_SYMBOL (sym)->u.s.redirect = SYMBOL_FORWARDED;
  SET_SYMBOL_FWD (XBARE_SYMBOL (sym), i_fwd);
}
void
defvar_bool (struct Lisp_Boolfwd const *b_fwd, char const *namestring)
{
  Lisp_Object sym = intern_c_string (namestring);
  XBARE_SYMBOL (sym)->u.s.declared_special = true;
  XBARE_SYMBOL (sym)->u.s.redirect = SYMBOL_FORWARDED;
  SET_SYMBOL_FWD (XBARE_SYMBOL (sym), b_fwd);
  Vbyte_boolean_vars = Fcons (sym, Vbyte_boolean_vars);
}

void
defsubr (union Aligned_Lisp_Subr *aname)
{
  struct Lisp_Subr *sname = &aname->s;
  Lisp_Object sym, tem;
  sym = intern_c_string (sname->symbol_name);
  XSETPVECTYPE (sname, PVEC_SUBR);
  XSETSUBR (tem, sname);
  set_symbol_function (sym, tem);
}

static int
readbyte_from_stdio (void)
{
  if (infile->lookahead)
    return infile->buf[--infile->lookahead];

  int c;
  FILE *instream = infile->stream;

  while ((c = getc (instream)) == EOF && errno == EINTR && ferror (instream))
    {
      maybe_quit ();
      clearerr (instream);
    }

  return (c == EOF ? -1 : c);
}

static int
readbyte_from_file (int c, Lisp_Object readcharfun)
{
  UNUSED (readcharfun);
  eassert (infile);
  if (c >= 0)
    {
      eassert ((unsigned long) infile->lookahead < sizeof infile->buf);
      infile->buf[infile->lookahead++] = c;
      return 0;
    }

  return readbyte_from_stdio ();
}

ptrdiff_t read_from_string_index;
ptrdiff_t read_from_string_index_byte;
ptrdiff_t read_from_string_limit;
#define READCHAR readchar (readcharfun, NULL)
#define UNREAD(c) unreadchar (readcharfun, c)
#define READCHAR_REPORT_MULTIBYTE(multibyte) readchar (readcharfun, multibyte)
int
readchar (Lisp_Object readcharfun, bool *multibyte)
{
  register int c;
  int (*readbyte) (int, Lisp_Object);
  int i, len;
  unsigned char buf[MAX_MULTIBYTE_LENGTH];

  if (multibyte)
    *multibyte = 0;

  if (STRINGP (readcharfun))
    {
      if (read_from_string_index >= read_from_string_limit)
        c = -1;
      else if (STRING_MULTIBYTE (readcharfun))
        {
          TODO;
        }
      else
        {
          c = SREF (readcharfun, read_from_string_index_byte);
          read_from_string_index++;
          read_from_string_index_byte++;
        }
      return c;
    }
  if (EQ (readcharfun, Qget_file_char))
    {
      eassert (infile);
      readbyte = readbyte_from_file;
      goto read_multibyte;
    }
  TODO;
read_multibyte:
  if (unread_char >= 0)
    {
      c = unread_char;
      unread_char = -1;
      return c;
    }
  c = (*readbyte) (-1, readcharfun);
  if (c < 0)
    return c;
  if (multibyte)
    *multibyte = 1;
  if (ASCII_CHAR_P (c))
    return c;
  i = 0;
  buf[i++] = c;
  len = BYTES_BY_CHAR_HEAD (c);
  while (i < len)
    {
      buf[i++] = c = (*readbyte) (-1, readcharfun);
      if (c < 0 || !TRAILING_CODE_P (c))
        {
          for (i -= c < 0; 0 < --i;)
            (*readbyte) (buf[i], readcharfun);
          return BYTE8_TO_CHAR (buf[0]);
        }
    }
  return STRING_CHAR (buf);
}
void
unreadchar (Lisp_Object readcharfun, int c)
{
  if (c == -1)
    ;
  else if (STRINGP (readcharfun))
    {
      read_from_string_index--;
      read_from_string_index_byte
        = string_char_to_byte (readcharfun, read_from_string_index);
    }
  else if (EQ (readcharfun, Qget_file_char))
    {
      unread_char = c;
    }
  else
    TODO;
}
enum read_entry_type
{
  RE_list_start,
  RE_list,
  RE_list_dot,
  RE_vector,
  RE_record,
  RE_char_table,
  RE_sub_char_table,
  RE_byte_code,
  RE_string_props,
  RE_special,
  RE_numbered,
};
struct read_stack_entry
{
  enum read_entry_type type;
  union
  {
    struct
    {
      Lisp_Object head;
      Lisp_Object tail;
    } list;
    struct
    {
      Lisp_Object elems;
      bool old_locate_syms;
    } vector;
    struct
    {
      Lisp_Object symbol;
    } special;
    struct
    {
      Lisp_Object number;
      Lisp_Object placeholder;
    } numbered;
  } u;
};
struct read_stack
{
  struct read_stack_entry *stack;
  ptrdiff_t size;
  ptrdiff_t sp;
};
static struct read_stack rdstack = { NULL, 0, 0 };
static inline struct read_stack_entry *
read_stack_top (void)
{
  eassume (rdstack.sp > 0);
  return &rdstack.stack[rdstack.sp - 1];
}
static inline struct read_stack_entry *
read_stack_pop (void)
{
  eassume (rdstack.sp > 0);
  return &rdstack.stack[--rdstack.sp];
}
static inline bool
read_stack_empty_p (ptrdiff_t base_sp)
{
  return rdstack.sp <= base_sp;
}
NO_INLINE static void
grow_read_stack (void)
{
  struct read_stack *rs = &rdstack;
  eassert (rs->sp == rs->size);
  rs->stack = xpalloc (rs->stack, &rs->size, 1, -1, sizeof *rs->stack);
  eassert (rs->sp < rs->size);
}
static inline void
read_stack_push (struct read_stack_entry e)
{
  if (rdstack.sp >= rdstack.size)
    grow_read_stack ();
  rdstack.stack[rdstack.sp++] = e;
}
static void
read_stack_reset (intmax_t sp)
{
  eassert (sp <= rdstack.sp);
  rdstack.sp = sp;
}
#define READ_AND_BUFFER(c)                     \
  c = READCHAR;                                \
  if (c < 0)                                   \
    TODO;                                      \
  if (multibyte)                               \
    p += CHAR_STRING (c, (unsigned char *) p); \
  else                                         \
    *p++ = c;                                  \
  if (end - p < MAX_MULTIBYTE_LENGTH + 1)      \
    {                                          \
      TODO;                                    \
    }
static AVOID
end_of_file_error (void)
{
  TODO;
}
static AVOID
invalid_syntax (const char *s, Lisp_Object readcharfun)
{
  UNUSED (s);
  UNUSED (readcharfun);
  TODO;
}
#define INVALID_SYNTAX_WITH_BUFFER()           \
  {                                            \
    *p = 0;                                    \
    invalid_syntax (read_buffer, readcharfun); \
  }
static void
skip_space_and_comments (Lisp_Object readcharfun)
{
  int c;
  do
    {
      c = READCHAR;
      if (c == ';')
        do
          c = READCHAR;
        while (c >= 0 && c != '\n');
      if (c < 0)
        end_of_file_error ();
    }
  while (c <= 32 || c == NO_BREAK_SPACE);
  UNREAD (c);
}
static int
read_char_escape (Lisp_Object readcharfun, int next_char)
{
  int modifiers = 0;
  ptrdiff_t ncontrol = 0;
  int chr;

again:;
  int c = next_char;
  int mod;

  switch (c)
    {
    case -1:
      end_of_file_error ();

    case 'a':
      chr = '\a';
      break;
    case 'b':
      chr = '\b';
      break;
    case 'd':
      chr = 127;
      break;
    case 'e':
      chr = 27;
      break;
    case 'f':
      chr = '\f';
      break;
    case 'n':
      chr = '\n';
      break;
    case 'r':
      chr = '\r';
      break;
    case 't':
      chr = '\t';
      break;
    case 'v':
      chr = '\v';
      break;

    case '\n':
      TODO;

    case 'M':
      mod = meta_modifier;
      goto mod_key;
    case 'S':
      mod = shift_modifier;
      goto mod_key;
    case 'H':
      mod = hyper_modifier;
      goto mod_key;
    case 'A':
      mod = alt_modifier;
      goto mod_key;
    case 's':
      mod = super_modifier;
      goto mod_key;

    mod_key:
      {
        int c1 = READCHAR;
        if (c1 != '-')
          {
            if (c == 's')
              {
                UNREAD (c1);
                chr = ' ';
                break;
              }
            else
              error ("Invalid escape char syntax: \\%c not followed by -", c);
          }
        modifiers |= mod;
        c1 = READCHAR;
        if (c1 == '\\')
          {
            next_char = READCHAR;
            goto again;
          }
        chr = c1;
        break;
      }

    case 'C':
      {
        int c1 = READCHAR;
        if (c1 != '-')
          TODO;
      }
      FALLTHROUGH;
    case '^':
      {
        ncontrol++;
        int c1 = READCHAR;
        if (c1 == '\\')
          {
            next_char = READCHAR;
            goto again;
          }
        chr = c1;
        break;
      }

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
      {
        int i = c - '0';
        int count = 0;
        while (count < 2)
          {
            int c = READCHAR;
            if (c < '0' || c > '7')
              {
                UNREAD (c);
                break;
              }
            i = (i << 3) + (c - '0');
            count++;
          }

        if (i >= 0x80 && i < 0x100)
          i = BYTE8_TO_CHAR (i);
        chr = i;
        break;
      }

    case 'x':
      TODO;

    case 'U':
      TODO;

    case 'u':
      TODO;

    case 'N':
      TODO;

    default:
      chr = c;
      break;
    }
  eassert (chr >= 0 && chr < (1 << CHARACTERBITS));

  while (ncontrol > 0)
    {
      if ((chr >= '@' && chr <= '_') || (chr >= 'a' && chr <= 'z'))
        chr &= 0x1f;
      else if (chr == '?')
        chr = 127;
      else
        modifiers |= ctrl_modifier;
      ncontrol--;
    }

  return chr | modifiers;
}
static char *
grow_read_buffer (char *buf, ptrdiff_t offset, char **buf_addr,
                  ptrdiff_t *buf_size, specpdl_ref count)
{
  char *p = xpalloc (*buf_addr, buf_size, MAX_MULTIBYTE_LENGTH, -1, 1);
  if (!*buf_addr)
    {
      memcpy (p, buf, offset);
      record_unwind_protect_ptr (xfree, p);
    }
  else
    set_unwind_protect_ptr (count, xfree, p);
  *buf_addr = p;
  return p;
}
static Lisp_Object
read_char_literal (Lisp_Object readcharfun)
{
  int ch = READCHAR;
  if (ch < 0)
    end_of_file_error ();

  if (ch == ' ' || ch == '\t')
    return make_fixnum (ch);

  if (ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '"'
      || ch == ';')
    {
      CHECK_LIST (Vlread_unescaped_character_literals);
      Lisp_Object char_obj = make_fixed_natnum (ch);
      if (NILP (Fmemq (char_obj, Vlread_unescaped_character_literals)))
        Vlread_unescaped_character_literals
          = Fcons (char_obj, Vlread_unescaped_character_literals);
    }

  if (ch == '\\')
    ch = read_char_escape (readcharfun, READCHAR);

  int modifiers = ch & CHAR_MODIFIER_MASK;
  ch &= ~CHAR_MODIFIER_MASK;
  if (CHAR_BYTE8_P (ch))
    ch = CHAR_TO_BYTE8 (ch);
  ch |= modifiers;

  int nch = READCHAR;
  UNREAD (nch);
  if (nch <= 32 || nch == '"' || nch == '\'' || nch == ';' || nch == '('
      || nch == ')' || nch == '[' || nch == ']' || nch == '#' || nch == '?'
      || nch == '`' || nch == ',' || nch == '.')
    return make_fixnum (ch);

  invalid_syntax ("?", readcharfun);
}

static Lisp_Object
read_string_literal (Lisp_Object readcharfun)
{
  char stackbuf[1024];
  char *read_buffer = stackbuf;
  ptrdiff_t read_buffer_size = sizeof stackbuf;
  specpdl_ref count = SPECPDL_INDEX ();
  char *heapbuf = NULL;
  char *p = read_buffer;
  char *end = read_buffer + read_buffer_size;
  bool force_multibyte = false;
  bool force_singlebyte = false;
  ptrdiff_t nchars = 0;
  int ch;
  while ((ch = READCHAR) >= 0 && ch != '\"')
    {
      if (end - p < MAX_MULTIBYTE_LENGTH)
        {
          ptrdiff_t offset = p - read_buffer;
          read_buffer = grow_read_buffer (read_buffer, offset, &heapbuf,
                                          &read_buffer_size, count);
          p = read_buffer + offset;
          end = read_buffer + read_buffer_size;
        }

      if (ch == '\\')
        {
          ch = READCHAR;
          switch (ch)
            {
            case 's':
              ch = ' ';
              break;
            case ' ':
            case '\n':
              continue;
            default:
              ch = read_char_escape (readcharfun, ch);
              break;
            }

          int modifiers = ch & CHAR_MODIFIER_MASK;
          ch &= ~CHAR_MODIFIER_MASK;

          if (CHAR_BYTE8_P (ch))
            force_singlebyte = true;
          else if (!ASCII_CHAR_P (ch))
            force_multibyte = true;
          else
            {
              if (modifiers == CHAR_CTL && ch == ' ')
                {
                  ch = 0;
                  modifiers = 0;
                }
              if (modifiers & CHAR_SHIFT)
                {
                  if (ch >= 'A' && ch <= 'Z')
                    modifiers &= ~CHAR_SHIFT;
                  else if (ch >= 'a' && ch <= 'z')
                    {
                      ch -= ('a' - 'A');
                      modifiers &= ~CHAR_SHIFT;
                    }
                }

              if (modifiers & CHAR_META)
                {
                  modifiers &= ~CHAR_META;
                  ch = BYTE8_TO_CHAR (ch | 0x80);
                  force_singlebyte = true;
                }
            }

          if (modifiers)
            invalid_syntax ("Invalid modifier in string", readcharfun);
          p += CHAR_STRING (ch, (unsigned char *) p);
        }
      else
        {
          p += CHAR_STRING (ch, (unsigned char *) p);
          if (CHAR_BYTE8_P (ch))
            force_singlebyte = true;
          else if (!ASCII_CHAR_P (ch))
            force_multibyte = true;
        }
      nchars++;
    }

  if (ch < 0)
    end_of_file_error ();

  if (!force_multibyte && force_singlebyte)
    {
      nchars = str_as_unibyte ((unsigned char *) read_buffer, p - read_buffer);
      p = read_buffer + nchars;
    }

  Lisp_Object obj
    = make_specified_string (read_buffer, nchars, p - read_buffer,
                             (force_multibyte || (p - read_buffer != nchars)));
  return unbind_to (count, obj);
}
static int
digit_to_number (int character, int base)
{
  int digit;

  if ('0' <= character && character <= '9')
    digit = character - '0';
  else if ('a' <= character && character <= 'z')
    digit = character - 'a' + 10;
  else if ('A' <= character && character <= 'Z')
    digit = character - 'A' + 10;
  else
    return -2;

  return digit < base ? digit : -1;
}
Lisp_Object
string_to_number (char const *string, int base, ptrdiff_t *plen)
{
  char const *cp = string;
  bool float_syntax = false;
  double value = 0;

  bool negative = *cp == '-';
  bool positive = *cp == '+';

  bool signedp = negative | positive;
  cp += signedp;

  enum
  {
    INTOVERFLOW = 1,
    LEAD_INT = 2,
    TRAIL_INT = 4,
    E_EXP = 16
  };
  int state = 0;
  int leading_digit = digit_to_number (*cp, base);
  uintmax_t n = leading_digit;
  if (leading_digit >= 0)
    {
      state |= LEAD_INT;
      for (int digit; 0 <= (digit = digit_to_number (*++cp, base));)
        {
#if TODO_NELISP_LATER_AND
          if (INT_MULTIPLY_OVERFLOW (n, base))
            state |= INTOVERFLOW;
#endif
          n *= base;
#if TODO_NELISP_LATER_AND
          if (INT_ADD_OVERFLOW (n, digit))
            state |= INTOVERFLOW;
#endif
          n += digit;
        }
    }
  // char const *after_digits = cp;
  if (*cp == '.')
    {
      cp++;
    }

  if (base == 10)
    {
      if ('0' <= *cp && *cp <= '9')
        {
          state |= TRAIL_INT;
          do
            cp++;
          while ('0' <= *cp && *cp <= '9');
        }
      if (*cp == 'e' || *cp == 'E')
        {
          char const *ecp = cp;
          cp++;
          if (*cp == '+' || *cp == '-')
            cp++;
          if ('0' <= *cp && *cp <= '9')
            {
              state |= E_EXP;
              do
                cp++;
              while ('0' <= *cp && *cp <= '9');
            }
          else if (cp[-1] == '+' && cp[0] == 'I' && cp[1] == 'N'
                   && cp[2] == 'F')
            {
              state |= E_EXP;
              cp += 3;
              value = INFINITY;
            }
          else if (cp[-1] == '+' && cp[0] == 'N' && cp[1] == 'a'
                   && cp[2] == 'N')
            {
              state |= E_EXP;
              cp += 3;
              union ieee754_double u
                = { .ieee_nan = { .exponent = 0x7ff,
                                  .quiet_nan = 1,
                                  .mantissa0 = n >> 31 >> 1,
                                  .mantissa1 = n } };
              value = u.d;
            }
          else
            cp = ecp;
        }

      float_syntax
        = ((state & TRAIL_INT) || ((state & LEAD_INT) && (state & E_EXP)));
    }

  if (plen)
    *plen = cp - string;

  if (float_syntax)
    {
      if (!value)
        value = atof (string + signedp);
      return make_float (negative ? -value : value);
    }

  if (!(state & LEAD_INT))
    return Qnil;

  if (!(state & INTOVERFLOW))
    {
      if (!negative)
        return make_uint (n);
      if (-MOST_NEGATIVE_FIXNUM < n)
        TODO;
      EMACS_INT signed_n = n;
      return make_fixnum (-signed_n);
    }

  TODO;
}
static Lisp_Object
vector_from_rev_list (Lisp_Object elems)
{
  ptrdiff_t size = list_length (elems);
  Lisp_Object obj = make_nil_vector (size);
  Lisp_Object *vec = XVECTOR (obj)->contents;
  for (ptrdiff_t i = size - 1; i >= 0; i--)
    {
      vec[i] = XCAR (elems);
      Lisp_Object next = XCDR (elems);
      free_cons (XCONS (elems));
      elems = next;
    }
  return obj;
}
static Lisp_Object
bytecode_from_rev_list (Lisp_Object elems, Lisp_Object readcharfun)
{
  Lisp_Object obj = vector_from_rev_list (elems);
  Lisp_Object *vec = XVECTOR (obj)->contents;
  ptrdiff_t size = ASIZE (obj);

  if (infile && size >= CLOSURE_CONSTANTS)
    {
      if (CONSP (vec[CLOSURE_CODE]) && FIXNUMP (XCDR (vec[CLOSURE_CODE])))
        TODO;

      if (NILP (vec[CLOSURE_CONSTANTS]) && STRINGP (vec[CLOSURE_CODE]))
        TODO;
    }

  if (!(size >= CLOSURE_STACK_DEPTH && size <= CLOSURE_INTERACTIVE + 1
        && (FIXNUMP (vec[CLOSURE_ARGLIST]) || CONSP (vec[CLOSURE_ARGLIST])
            || NILP (vec[CLOSURE_ARGLIST]))
        && ((STRINGP (vec[CLOSURE_CODE]) && VECTORP (vec[CLOSURE_CONSTANTS])
             && size > CLOSURE_STACK_DEPTH
             && (FIXNATP (vec[CLOSURE_STACK_DEPTH])))
            || (CONSP (vec[CLOSURE_CODE])
                && (CONSP (vec[CLOSURE_CONSTANTS])
                    || NILP (vec[CLOSURE_CONSTANTS]))))))
    invalid_syntax ("Invalid byte-code object", readcharfun);

  if (STRINGP (vec[CLOSURE_CODE]))
    {
      if (STRING_MULTIBYTE (vec[CLOSURE_CODE]))
        TODO;

      pin_string (vec[CLOSURE_CODE]);
    }

  XSETPVECTYPE (XVECTOR (obj), PVEC_CLOSURE);
  return obj;
}
static Lisp_Object
hash_table_from_plist (Lisp_Object plist)
{
  Lisp_Object params[4 * 2];
  Lisp_Object *par = params;

#define ADDPARAM(name)                              \
  do                                                \
    {                                               \
      Lisp_Object val = plist_get (plist, Q##name); \
      if (!NILP (val))                              \
        {                                           \
          *par++ = QC##name;                        \
          *par++ = val;                             \
        }                                           \
    }                                               \
  while (0)

  ADDPARAM (test);
  ADDPARAM (weakness);
  ADDPARAM (purecopy);

  Lisp_Object data = plist_get (plist, Qdata);
  if (!(NILP (data) || CONSP (data)))
    error ("Hash table data is not a list");
  ptrdiff_t data_len = list_length (data);
  if (data_len & 1)
    error ("Hash table data length is odd");
  *par++ = QCsize;
  *par++ = make_fixnum (data_len / 2);

  Lisp_Object ht = Fmake_hash_table (par - params, params);

  while (!NILP (data))
    {
      Lisp_Object key = XCAR (data);
      data = XCDR (data);
      Lisp_Object val = XCAR (data);
      Fputhash (key, val, ht);
      data = XCDR (data);
    }

  return ht;
}
Lisp_Object
read0 (Lisp_Object readcharfun, bool locate_syms)
{
  TODO_NELISP_LATER;
  char stackbuf[64];
  char *read_buffer = stackbuf;
  ptrdiff_t read_buffer_size = sizeof stackbuf;
  char *heapbuf = NULL;
  specpdl_ref base_pdl = SPECPDL_INDEX ();
  ptrdiff_t base_sp = rdstack.sp;
  record_unwind_protect_intmax (read_stack_reset, base_sp);
  specpdl_ref count = SPECPDL_INDEX ();
  bool uninterned_symbol;
  bool skip_shorthand;
read_obj:;
  Lisp_Object obj;
  bool multibyte;
  int c = READCHAR_REPORT_MULTIBYTE (&multibyte);
  switch (c)
    {
    case '(':
      read_stack_push ((struct read_stack_entry) { .type = RE_list_start });
      goto read_obj;
    case ')':
      if (read_stack_empty_p (base_sp))
        invalid_syntax (")", readcharfun);
      switch (read_stack_top ()->type)
        {
        case RE_list_start:
          read_stack_pop ();
          obj = Qnil;
          break;
        case RE_list:
          obj = read_stack_pop ()->u.list.head;
          break;
        case RE_record:
          {
            locate_syms = read_stack_top ()->u.vector.old_locate_syms;
            Lisp_Object elems = Fnreverse (read_stack_pop ()->u.vector.elems);
            if (NILP (elems))
              invalid_syntax ("#s", readcharfun);

            if (BASE_EQ (XCAR (elems), Qhash_table))
              obj = hash_table_from_plist (XCDR (elems));
            else
              TODO;
            break;
          }
        case RE_string_props:
          TODO;
        default:
          invalid_syntax (")", readcharfun);
        }
      break;
    case '[':
      read_stack_push ((struct read_stack_entry) {
        .type = RE_vector,
        .u.vector.elems = Qnil,
        .u.vector.old_locate_syms = locate_syms,
      });
      goto read_obj;
    case ']':
      if (read_stack_empty_p (base_sp))
        invalid_syntax ("]", readcharfun);
      switch (read_stack_top ()->type)
        {
        case RE_vector:
          locate_syms = read_stack_top ()->u.vector.old_locate_syms;
          obj = vector_from_rev_list (read_stack_pop ()->u.vector.elems);
          break;
        case RE_byte_code:
          locate_syms = read_stack_top ()->u.vector.old_locate_syms;
          obj = bytecode_from_rev_list (read_stack_pop ()->u.vector.elems,
                                        readcharfun);
          break;
        case RE_char_table:
          TODO;
        case RE_sub_char_table:
          TODO;
        default:
          invalid_syntax ("]", readcharfun);
          break;
        }
      break;
    case '#':
      {
        char *p = read_buffer;
        char *end = read_buffer + read_buffer_size;

        *p++ = '#';
        int ch;
        READ_AND_BUFFER (ch);
        switch (ch)
          {
          case '\'':
            read_stack_push ((struct read_stack_entry) {
              .type = RE_special,
              .u.special.symbol = Qfunction,
            });
            goto read_obj;
          case '#':
            TODO;
          case 's':
            READ_AND_BUFFER (ch);
            if (ch != '(')
              {
                UNREAD (ch);
                INVALID_SYNTAX_WITH_BUFFER ();
              }
            read_stack_push ((struct read_stack_entry) {
              .type = RE_record,
              .u.vector.elems = Qnil,
              .u.vector.old_locate_syms = locate_syms,
            });
            locate_syms = false;
            goto read_obj;
          case '^':
            TODO;
          case '(':
            TODO;
          case '[':
            read_stack_push ((struct read_stack_entry) {
              .type = RE_byte_code,
              .u.vector.elems = Qnil,
              .u.vector.old_locate_syms = locate_syms,
            });
            locate_syms = false;
            goto read_obj;
          case '&':
            TODO;
          case '!':
            TODO;
          case 'x':
          case 'X':
            TODO;
          case 'o':
          case 'O':
            TODO;
          case 'b':
          case 'B':
            TODO;
          case '@':
            TODO;
          case '$':
            TODO;
          case ':':
            TODO;
          case '_':
            TODO;
          default:
            if (ch >= '0' && ch <= '9')
              {
                EMACS_INT n = ch - '0';
                int c;
                for (;;)
                  {
                    READ_AND_BUFFER (c);
                    if (c < '0' || c > '9')
                      break;
                    if (ckd_mul (&n, n, 10) || ckd_add (&n, n, c - '0'))
                      INVALID_SYNTAX_WITH_BUFFER ();
                  }
                if (c == 'r' || c == 'R')
                  TODO;
                else if (n <= MOST_POSITIVE_FIXNUM && !NILP (Vread_circle))
                  {
                    if (c == '=')
                      {
                        Lisp_Object placeholder = Fcons (Qnil, Qnil);

                        struct Lisp_Hash_Table *h
                          = XHASH_TABLE (read_objects_map);
                        Lisp_Object number = make_fixnum (n);
                        hash_hash_t hash;
                        ptrdiff_t i = hash_lookup_get_hash (h, number, &hash);
                        if (i >= 0)
                          set_hash_value_slot (h, i, placeholder);
                        else
                          hash_put (h, number, placeholder, hash);
                        read_stack_push ((struct read_stack_entry) {
                          .type = RE_numbered,
                          .u.numbered.number = number,
                          .u.numbered.placeholder = placeholder,
                        });
                        goto read_obj;
                      }
                    else if (c == '#')
                      {
                        struct Lisp_Hash_Table *h
                          = XHASH_TABLE (read_objects_map);
                        ptrdiff_t i = hash_lookup (h, make_fixnum (n));
                        if (i < 0)
                          INVALID_SYNTAX_WITH_BUFFER ();
                        obj = HASH_VALUE (h, i);
                        break;
                      }
                    else
                      INVALID_SYNTAX_WITH_BUFFER ();
                  }
                else
                  INVALID_SYNTAX_WITH_BUFFER ();
              }
            else
              INVALID_SYNTAX_WITH_BUFFER ();
          }
        break;
      }
    case '?':
      obj = read_char_literal (readcharfun);
      break;
    case '"':
      obj = read_string_literal (readcharfun);
      break;
    case '\'':
      read_stack_push ((struct read_stack_entry) {
        .type = RE_special,
        .u.special.symbol = Qquote,
      });
      goto read_obj;
    case '`':
      read_stack_push ((struct read_stack_entry) {
        .type = RE_special,
        .u.special.symbol = Qbackquote,
      });
      goto read_obj;
    case ',':
      {
        int ch = READCHAR;
        Lisp_Object sym;
        if (ch == '@')
          sym = Qcomma_at;
        else
          {
            if (ch >= 0)
              UNREAD (ch);
            sym = Qcomma;
          }
        read_stack_push ((struct read_stack_entry) {
          .type = RE_special,
          .u.special.symbol = sym,
        });
        goto read_obj;
      }
    case ';':
      {
        int c;
        do
          c = READCHAR;
        while (c >= 0 && c != '\n');
        goto read_obj;
      }
    case '.':
      {
        int nch = READCHAR;
        UNREAD (nch);
        if (nch <= 32 || nch == NO_BREAK_SPACE || nch == '"' || nch == '\''
            || nch == ';' || nch == '(' || nch == '[' || nch == '#'
            || nch == '?' || nch == '`' || nch == ',')
          {
            if (!read_stack_empty_p (base_sp)
                && read_stack_top ()->type == RE_list)
              {
                read_stack_top ()->type = RE_list_dot;
                goto read_obj;
              }
            invalid_syntax (".", readcharfun);
          }
      }
      FALLTHROUGH;
    default:
      if (c <= 32 || c == NO_BREAK_SPACE)
        goto read_obj;

      uninterned_symbol = false;
      skip_shorthand = false;

      char *p = read_buffer;
      char *end = read_buffer + read_buffer_size;
      bool quoted = false;
      do
        {
          if (end - p < MAX_MULTIBYTE_LENGTH + 1)
            {
              ptrdiff_t offset = p - read_buffer;
              read_buffer = grow_read_buffer (read_buffer, offset, &heapbuf,
                                              &read_buffer_size, count);
              p = read_buffer + offset;
              end = read_buffer + read_buffer_size;
            }

          if (c == '\\')
            {
              c = READCHAR;
              if (c < 0)
                end_of_file_error ();
              quoted = true;
            }

          if (multibyte)
            p += CHAR_STRING (c, (unsigned char *) p);
          else
            *p++ = c;
          c = READCHAR;
        }
      while (
        c > 32 && c != NO_BREAK_SPACE
        && (c >= 128
            || !(c == '"' || c == '\'' || c == ';' || c == '#' || c == '('
                 || c == ')' || c == '[' || c == ']' || c == '`' || c == ',')));

      *p = 0;
      ptrdiff_t nbytes = p - read_buffer;
      UNREAD (c);

      char c0 = read_buffer[0];
      if (((c0 >= '0' && c0 <= '9') || c0 == '.' || c0 == '-' || c0 == '+')
          && !quoted && !uninterned_symbol && !skip_shorthand)
        {
          ptrdiff_t len;
          Lisp_Object result = string_to_number (read_buffer, 10, &len);
          if (!NILP (result) && len == nbytes)
            {
              obj = result;
              break;
            }
        }

#if TODO_NELISP_LATER_AND
      ptrdiff_t nchars
        = (multibyte
             ? multibyte_chars_in_text ((unsigned char *) read_buffer, nbytes)
             : nbytes);
#else
      ptrdiff_t nchars = nbytes;
#endif
      Lisp_Object result;
      if (uninterned_symbol)
        {
          TODO;
        }
      else
        {
          Lisp_Object obarray = check_obarray (Vobarray);

          char *longhand = NULL;
          Lisp_Object found;
#if TODO_NELISP_LATER_AND
          ptrdiff_t longhand_chars = 0;
          ptrdiff_t longhand_bytes = 0;

          if (skip_shorthand || symbol_char_span (read_buffer) >= nbytes)
            found = oblookup (obarray, read_buffer, nchars, nbytes);
          else
            found = oblookup_considering_shorthand (obarray, read_buffer,
                                                    nchars, nbytes, &longhand,
                                                    &longhand_chars,
                                                    &longhand_bytes);
#else
          found = oblookup (obarray, read_buffer, nchars, nbytes);
#endif

          if (BARE_SYMBOL_P (found))
            result = found;
          else if (longhand)
            {
              TODO;
            }
          else
            {
              Lisp_Object name = make_specified_string (read_buffer, nchars,
                                                        nbytes, multibyte);
              result = intern_driver (name, obarray, found);
            }
        }
      if (locate_syms && !NILP (result))
        TODO;

      obj = result;
      break;
    }

  while (rdstack.sp > base_sp)
    {
      struct read_stack_entry *e = read_stack_top ();
      switch (e->type)
        {
        case RE_list_start:
          e->type = RE_list;
          e->u.list.head = e->u.list.tail = Fcons (obj, Qnil);
          goto read_obj;
        case RE_list:
          {
            Lisp_Object tl = Fcons (obj, Qnil);
            XSETCDR (e->u.list.tail, tl);
            e->u.list.tail = tl;
            goto read_obj;
          }
        case RE_list_dot:
          {
            skip_space_and_comments (readcharfun);
            int ch = READCHAR;
            if (ch != ')')
              invalid_syntax ("expected )", readcharfun);
            XSETCDR (e->u.list.tail, obj);
            read_stack_pop ();
            obj = e->u.list.head;

#if TODO_NELISP_LATER_AND
            if (load_force_doc_strings && BASE_EQ (XCAR (obj), Vload_file_name)
                && !NILP (XCAR (obj)) && FIXNUMP (XCDR (obj)))
              obj = get_lazy_string (obj);
#endif

            break;
          }
        case RE_vector:
        case RE_record:
        case RE_char_table:
        case RE_sub_char_table:
        case RE_byte_code:
        case RE_string_props:
          e->u.vector.elems = Fcons (obj, e->u.vector.elems);
          goto read_obj;
        case RE_special:
          read_stack_pop ();
          obj = list2 (e->u.special.symbol, obj);
          break;
        case RE_numbered:
          {
            read_stack_pop ();
            Lisp_Object placeholder = e->u.numbered.placeholder;
            if (CONSP (obj))
              {
                if (BASE_EQ (obj, placeholder))
                  invalid_syntax ("nonsensical self-reference", readcharfun);

                Fsetcar (placeholder, XCAR (obj));
                Fsetcdr (placeholder, XCDR (obj));

                struct Lisp_Hash_Table *h2
                  = XHASH_TABLE (read_objects_completed);
                hash_hash_t hash;
                ptrdiff_t i = hash_lookup_get_hash (h2, placeholder, &hash);
                eassert (i < 0);
                hash_put (h2, placeholder, Qnil, hash);
                obj = placeholder;
              }
            else
              {
                if (!SYMBOLP (obj) && !NUMBERP (obj)
                    && !(STRINGP (obj) && !string_intervals (obj)))
                  {
                    struct Lisp_Hash_Table *h2
                      = XHASH_TABLE (read_objects_completed);
                    hash_hash_t hash;
                    ptrdiff_t i = hash_lookup_get_hash (h2, obj, &hash);
                    eassert (i < 0);
                    hash_put (h2, obj, Qnil, hash);
                  }

                Flread__substitute_object_in_subtree (obj, placeholder,
                                                      read_objects_completed);

                struct Lisp_Hash_Table *h = XHASH_TABLE (read_objects_map);
                hash_hash_t hash;
                ptrdiff_t i
                  = hash_lookup_get_hash (h, e->u.numbered.number, &hash);
                eassert (i >= 0);
                set_hash_value_slot (h, i, obj);
              }
            break;
          }
        }
    }
  return unbind_to (base_pdl, obj);
}

struct subst
{
  Lisp_Object object;
  Lisp_Object placeholder;

  Lisp_Object completed;

  Lisp_Object seen;
};

static Lisp_Object
substitute_object_recurse (struct subst *subst, Lisp_Object subtree)
{
  if (EQ (subst->placeholder, subtree))
    return subst->object;

  if (SYMBOLP (subtree) || (STRINGP (subtree) && !string_intervals (subtree))
      || NUMBERP (subtree))
    return subtree;

  if (!NILP (Fmemq (subtree, subst->seen)))
    return subtree;

  if (EQ (subst->completed, Qt)
      || hash_lookup (XHASH_TABLE (subst->completed), subtree) >= 0)
    subst->seen = Fcons (subtree, subst->seen);

  switch (XTYPE (subtree))
    {
    case Lisp_Vectorlike:
      {
        ptrdiff_t i = 0, length = 0;
        if (BOOL_VECTOR_P (subtree))
          return subtree;
        else if (CHAR_TABLE_P (subtree) || SUB_CHAR_TABLE_P (subtree)
                 || CLOSUREP (subtree) || HASH_TABLE_P (subtree)
                 || RECORDP (subtree))
          length = PVSIZE (subtree);
        else if (VECTORP (subtree))
          length = ASIZE (subtree);
        else
          wrong_type_argument (Qsequencep, subtree);

        if (SUB_CHAR_TABLE_P (subtree))
          i = 2;
        for (; i < length; i++)
          ASET (subtree, i,
                substitute_object_recurse (subst, AREF (subtree, i)));
        return subtree;
      }

    case Lisp_Cons:
      XSETCAR (subtree, substitute_object_recurse (subst, XCAR (subtree)));
      XSETCDR (subtree, substitute_object_recurse (subst, XCDR (subtree)));
      return subtree;

    case Lisp_String:
      {
        TODO; // INTERVAL root_interval = string_intervals (subtree);
              // traverse_intervals_noorder (root_interval,
        // 			    substitute_in_interval, subst);
        return subtree;
      }

    default:
      return subtree;
    }
}
DEFUN ("lread--substitute-object-in-subtree",
       Flread__substitute_object_in_subtree,
       Slread__substitute_object_in_subtree, 3, 3, 0,
       doc: /* In OBJECT, replace every occurrence of PLACEHOLDER with OBJECT.
COMPLETED is a hash table of objects that might be circular, or is t
if any object might be circular.  */)
(Lisp_Object object, Lisp_Object placeholder, Lisp_Object completed)
{
  struct subst subst = { object, placeholder, completed, Qnil };
  Lisp_Object check_object = substitute_object_recurse (&subst, object);

  if (!EQ (check_object, object))
    error ("Unexpected mutation error in reader");
  return Qnil;
}

void
mark_lread (void)
{
  for (ptrdiff_t i = 0; i < rdstack.sp; i++)
    {
      struct read_stack_entry *e = &rdstack.stack[i];
      switch (e->type)
        {
        case RE_list_start:
          break;
        case RE_list:
        case RE_list_dot:
          mark_object (e->u.list.head);
          mark_object (e->u.list.tail);
          break;
        case RE_vector:
        case RE_record:
        case RE_char_table:
        case RE_sub_char_table:
        case RE_byte_code:
        case RE_string_props:
          mark_object (e->u.vector.elems);
          break;
        case RE_special:
          mark_object (e->u.special.symbol);
          break;
        case RE_numbered:
          mark_object (e->u.numbered.number);
          mark_object (e->u.numbered.placeholder);
          break;
        }
    }
}

lexical_cookie_t
lisp_file_lexical_cookie (Lisp_Object readcharfun)
{
  int ch = READCHAR;

  if (ch == '#')
    {
      ch = READCHAR;
      if (ch != '!')
        {
          UNREAD (ch);
          UNREAD ('#');
          return Cookie_None;
        }
      while (ch != '\n' && ch != EOF)
        ch = READCHAR;
      if (ch == '\n')
        ch = READCHAR;
    }

  if (ch != ';')
    {
      UNREAD (ch);
      return Cookie_None;
    }
  else
    {
      lexical_cookie_t rv = Cookie_None;
      enum
      {
        NOMINAL,
        AFTER_FIRST_DASH,
        AFTER_ASTERIX
      } beg_end_state
        = NOMINAL;
      bool in_file_vars = 0;

#define UPDATE_BEG_END_STATE(ch)                              \
  if (beg_end_state == NOMINAL)                               \
    beg_end_state = (ch == '-' ? AFTER_FIRST_DASH : NOMINAL); \
  else if (beg_end_state == AFTER_FIRST_DASH)                 \
    beg_end_state = (ch == '*' ? AFTER_ASTERIX : NOMINAL);    \
  else if (beg_end_state == AFTER_ASTERIX)                    \
    {                                                         \
      if (ch == '-')                                          \
        in_file_vars = !in_file_vars;                         \
      beg_end_state = NOMINAL;                                \
    }

      do
        {
          ch = READCHAR;
          UPDATE_BEG_END_STATE (ch);
        }
      while (!in_file_vars && ch != '\n' && ch != EOF);

      while (in_file_vars)
        {
          char var[100], val[100];
          unsigned i;

          ch = READCHAR;

          while (ch == ' ' || ch == '\t')
            ch = READCHAR;

          i = 0;
          beg_end_state = NOMINAL;
          while (ch != ':' && ch != '\n' && ch != EOF && in_file_vars)
            {
              if (i < sizeof var - 1)
                var[i++] = ch;
              UPDATE_BEG_END_STATE (ch);
              ch = READCHAR;
            }

          if (!in_file_vars || ch == '\n' || ch == EOF)
            break;

          while (i > 0 && (var[i - 1] == ' ' || var[i - 1] == '\t'))
            i--;
          var[i] = '\0';

          if (ch == ':')
            {
              /* Read a variable value.  */
              ch = READCHAR;

              while (ch == ' ' || ch == '\t')
                ch = READCHAR;

              i = 0;
              beg_end_state = NOMINAL;
              while (ch != ';' && ch != '\n' && ch != EOF && in_file_vars)
                {
                  if (i < sizeof val - 1)
                    val[i++] = ch;
                  UPDATE_BEG_END_STATE (ch);
                  ch = READCHAR;
                }
              if (!in_file_vars)
                i -= 3;
              while (i > 0 && (val[i - 1] == ' ' || val[i - 1] == '\t'))
                i--;
              val[i] = '\0';

              if (strcmp (var, "lexical-binding") == 0)
                {
                  rv = strcmp (val, "nil") != 0 ? Cookie_Lex : Cookie_Dyn;
                  break;
                }
            }
        }

      while (ch != '\n' && ch != EOF)
        ch = READCHAR;

      return rv;
    }
}

static Lisp_Object
read_internal_start (Lisp_Object stream, Lisp_Object start, Lisp_Object end,
                     bool locate_syms)
{
  UNUSED (start);
  UNUSED (end);
  TODO_NELISP_LATER;
  Lisp_Object retval;

  if (!HASH_TABLE_P (read_objects_map) || XHASH_TABLE (read_objects_map)->count)
    read_objects_map
      = make_hash_table (&hashtest_eq, DEFAULT_HASH_SIZE, Weak_None, false);
  if (!HASH_TABLE_P (read_objects_completed)
      || XHASH_TABLE (read_objects_completed)->count)
    read_objects_completed
      = make_hash_table (&hashtest_eq, DEFAULT_HASH_SIZE, Weak_None, false);

  if (STRINGP (stream) || ((CONSP (stream) && STRINGP (XCAR (stream)))))
    {
      ptrdiff_t startval, endval;
      Lisp_Object string;

      if (STRINGP (stream))
        string = stream;
      else
        string = XCAR (stream);

      validate_subarray (string, start, end, SCHARS (string), &startval,
                         &endval);

      read_from_string_index = startval;
      read_from_string_index_byte = string_char_to_byte (string, startval);
      read_from_string_limit = endval;
    }

  retval = read0 (stream, locate_syms);
  if (HASH_TABLE_P (read_objects_map)
      && XHASH_TABLE (read_objects_map)->count > 0)
    read_objects_map = Qnil;
  if (HASH_TABLE_P (read_objects_completed)
      && XHASH_TABLE (read_objects_completed)->count > 0)
    read_objects_completed = Qnil;
  return retval;
}
static void
readevalloop (Lisp_Object readcharfun, struct infile *infile0,
              Lisp_Object sourcename, bool printflag, Lisp_Object unibyte,
              Lisp_Object readfun, Lisp_Object start, Lisp_Object end)
{
  UNUSED (unibyte);
  UNUSED (end);
  TODO_NELISP_LATER;
  int c;
  Lisp_Object val;
  struct buffer *b = 0;
  Lisp_Object lex_bound;
  bool continue_reading_p;
  specpdl_ref count = SPECPDL_INDEX ();

  if (!NILP (sourcename))
    CHECK_STRING (sourcename);

  if (!NILP (start) && !b)
    emacs_abort ();

  lex_bound = find_symbol_value (Qlexical_binding);
  specbind (Qinternal_interpreter_environment,
            (NILP (lex_bound) || BASE_EQ (lex_bound, Qunbound) ? Qnil
                                                               : list1 (Qt)));
  specbind (Qmacroexp__dynvars, Vmacroexp__dynvars);

  continue_reading_p = 1;
  while (continue_reading_p)
    {
      specpdl_ref count1 = SPECPDL_INDEX ();
      eassert (!infile0 || infile == infile0);
    read_next:
      c = READCHAR;
      if (c == ';')
        {
          while ((c = READCHAR) != '\n' && c != -1)
            ;
          goto read_next;
        }
      if (c < 0)
        {
          unbind_to (count1, Qnil);
          break;
        }
      if (c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r'
          || c == NO_BREAK_SPACE)
        goto read_next;
      UNREAD (c);
      if (!HASH_TABLE_P (read_objects_map)
          || XHASH_TABLE (read_objects_map)->count)
        read_objects_map
          = make_hash_table (&hashtest_eq, DEFAULT_HASH_SIZE, Weak_None, false);
      if (!HASH_TABLE_P (read_objects_completed)
          || XHASH_TABLE (read_objects_completed)->count)
        read_objects_completed
          = make_hash_table (&hashtest_eq, DEFAULT_HASH_SIZE, Weak_None, false);
      if (!NILP (Vpurify_flag) && c == '(')
        val = read0 (readcharfun, false);
      else
        {
          if (!NILP (readfun))
            TODO;
          else
            val = read_internal_start (readcharfun, Qnil, Qnil, false);
        }
      if (HASH_TABLE_P (read_objects_map)
          && XHASH_TABLE (read_objects_map)->count > 0)
        read_objects_map = Qnil;
      if (HASH_TABLE_P (read_objects_completed)
          && XHASH_TABLE (read_objects_completed)->count > 0)
        read_objects_completed = Qnil;
      unbind_to (count1, Qnil);
      val = eval_sub (val);
      if (printflag)
        TODO;
    }
  unbind_to (count, Qnil);
}

DEFUN ("read-from-string", Fread_from_string, Sread_from_string, 1, 3, 0,
       doc: /* Read one Lisp expression which is represented as text by STRING.
Returns a cons: (OBJECT-READ . FINAL-STRING-INDEX).
FINAL-STRING-INDEX is an integer giving the position of the next
remaining character in STRING.  START and END optionally delimit
a substring of STRING from which to read;  they default to 0 and
\(length STRING) respectively.  Negative values are counted from
the end of STRING.  */)
(Lisp_Object string, Lisp_Object start, Lisp_Object end)
{
  Lisp_Object ret;
  CHECK_STRING (string);
  ret = read_internal_start (string, start, end, false);
  return Fcons (ret, make_fixnum (read_from_string_index));
}

void
init_lread (void)
{
  load_in_progress = 0;

  Vload_file_name = Qnil;
}

void
syms_of_lread (void)
{
  DEFVAR_LISP ("obarray", Vobarray,
               doc: /* Symbol table for use by `intern' and `read'.
It is a vector whose length ought to be prime for best results.
The vector's contents don't make sense if examined from Lisp programs;
to find all the symbols in an obarray, use `mapatoms'.  */);

  DEFSYM (Qhash_table, "hash-table");
  DEFSYM (Qdata, "data");
  DEFSYM (Qtest, "test");
  DEFSYM (Qsize, "size");
  DEFSYM (Qpurecopy, "purecopy");
  DEFSYM (Qweakness, "weakness");

  DEFSYM (Qobarray_cache, "obarray-cache");

  DEFSYM (Qfunction, "function");

  DEFSYM (Qlexical_binding, "lexical-binding");

  DEFSYM (Qget_file_char, "get-file-char");

  DEFSYM (Qbackquote, "`");
  DEFSYM (Qcomma, ",");
  DEFSYM (Qcomma_at, ",@");

  DEFSYM (Qmacroexp__dynvars, "macroexp--dynvars");
  DEFVAR_LISP ("macroexp--dynvars", Vmacroexp__dynvars,
        doc:   /* List of variables declared dynamic in the current scope.
Only valid during macro-expansion.  Internal use only. */);
  Vmacroexp__dynvars = Qnil;

  DEFVAR_LISP ("read-circle", Vread_circle,
               doc: /* Non-nil means read recursive structures using #N= and #N# syntax.  */);
  Vread_circle = Qt;

  DEFVAR_LISP ("load-path", Vload_path,
               doc: /* List of directories to search for files to load.
Each element is a string (directory file name) or nil (meaning
`default-directory').
This list is consulted by the `require' function.
Initialized during startup as described in Info node `(elisp)Library Search'.
Use `directory-file-name' when adding items to this path.  However, Lisp
programs that process this list should tolerate directories both with
and without trailing slashes.  */);

  DEFVAR_LISP ("load-suffixes", Vload_suffixes,
        doc: /* List of suffixes for Emacs Lisp files and dynamic modules.
This list includes suffixes for both compiled and source Emacs Lisp files.
This list should not include the empty string.
`load' and related functions try to append these suffixes, in order,
to the specified file name if a suffix is allowed or required.  */);
  Vload_suffixes
    = list2 (build_pure_c_string (".elc"), build_pure_c_string (".el"));

  DEFVAR_LISP ("load-file-rep-suffixes", Vload_file_rep_suffixes,
        doc: /* List of suffixes that indicate representations of \
the same file.
This list should normally start with the empty string.

Enabling Auto Compression mode appends the suffixes in
`jka-compr-load-suffixes' to this list and disabling Auto Compression
mode removes them again.  `load' and related functions use this list to
determine whether they should look for compressed versions of a file
and, if so, which suffixes they should try to append to the file name
in order to do so.  However, if you want to customize which suffixes
the loading functions recognize as compression suffixes, you should
customize `jka-compr-load-suffixes' rather than the present variable.  */);
  Vload_file_rep_suffixes = list1 (empty_unibyte_string);

  DEFVAR_LISP ("load-file-name", Vload_file_name,
        doc: /* Full name of file being loaded by `load'.

In case of native code being loaded this is indicating the
corresponding bytecode filename.  Use `load-true-file-name' to obtain
the .eln filename.  */);
  Vload_file_name = Qnil;

  DEFVAR_LISP ("byte-boolean-vars", Vbyte_boolean_vars,
        doc: /* List of all DEFVAR_BOOL variables, used by the byte code optimizer.  */);
  Vbyte_boolean_vars = Qnil;

  DEFVAR_LISP ("lread--unescaped-character-literals",
               Vlread_unescaped_character_literals,
               doc: /* List of deprecated unescaped character literals encountered by `read'.
For internal use only.  */);
  Vlread_unescaped_character_literals = Qnil;
  DEFSYM (Qlread_unescaped_character_literals,
          "lread--unescaped-character-literals");

  DEFVAR_BOOL ("load-in-progress", load_in_progress,
        doc: /* Non-nil if inside of `load'.  */);

  DEFVAR_LISP ("current-load-list", Vcurrent_load_list,
        doc: /* Used for internal purposes by `load'.  */);
  Vcurrent_load_list = Qnil;

  staticpro (&read_objects_map);
  read_objects_map = Qnil;
  staticpro (&read_objects_completed);
  read_objects_completed = Qnil;

  defsubr (&Sobarray_make);
  defsubr (&Sget_load_suffixes);
  defsubr (&Sload);
  defsubr (&Slocate_file_internal);
  defsubr (&Sread);
  defsubr (&Sintern);
  defsubr (&Sunintern);
  defsubr (&Slread__substitute_object_in_subtree);
  defsubr (&Sread_from_string);
}
