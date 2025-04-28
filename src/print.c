#include <math.h>

#include "lisp.h"
#include "buffer.h"
#include "character.h"

static ptrdiff_t print_depth;

#define PRINT_CIRCLE 200
static Lisp_Object being_printed[PRINT_CIRCLE];

static ptrdiff_t new_backquote_output;

struct print_buffer
{
  char *buffer;
  ptrdiff_t size;
  ptrdiff_t pos;
  ptrdiff_t pos_byte;
};

static struct print_buffer print_buffer;

static ptrdiff_t print_number_index;

static void
print_free_buffer (void)
{
  xfree (print_buffer.buffer);
  print_buffer.buffer = NULL;
}

static void
print_unwind (Lisp_Object saved_text)
{
  memcpy (print_buffer.buffer, SDATA (saved_text), SCHARS (saved_text));
}

struct print_context
{
  Lisp_Object printcharfun;
  Lisp_Object old_printcharfun;
  ptrdiff_t old_point, start_point;
  ptrdiff_t old_point_byte, start_point_byte;
  specpdl_ref specpdl_count;
};

static inline struct print_context
print_prepare (Lisp_Object printcharfun, bool special)
{
  struct print_context pc = {
    .old_printcharfun = printcharfun,
    .old_point = -1,
    .start_point = -1,
    .old_point_byte = -1,
    .start_point_byte = -1,
    .specpdl_count = SPECPDL_INDEX (),
  };
  if (special)
    {
      eassert (printcharfun == Qnil);
    }
  else if (NILP (printcharfun))
    printcharfun = Qt;
  if (BUFFERP (printcharfun))
    TODO;
  if (MARKERP (printcharfun))
    TODO;
  if (NILP (printcharfun))
    {
      if ((special || (TODO, false)) && !print_escape_multibyte)
        specbind (Qprint_escape_multibyte, Qt);
      if ((special || (TODO, false)) && !print_escape_nonascii)
        specbind (Qprint_escape_nonascii, Qt);
      if (print_buffer.buffer != NULL)
        {
          Lisp_Object string
            = make_string_from_bytes (print_buffer.buffer, print_buffer.pos,
                                      print_buffer.pos_byte);
          record_unwind_protect (print_unwind, string);
        }
      else
        {
          int new_size = 1000;
          print_buffer.buffer = xmalloc (new_size);
          print_buffer.size = new_size;
          record_unwind_protect_void (print_free_buffer);
        }
      print_buffer.pos = 0;
      print_buffer.pos_byte = 0;
    }
  if (EQ (printcharfun, Qt) && (TODO, false))
    TODO;
  pc.printcharfun = printcharfun;
  return pc;
}

static inline void
print_finish (struct print_context *pc, bool special)
{
  if (NILP (pc->printcharfun) && !special)
    TODO;
  if (MARKERP (pc->old_printcharfun))
    TODO;
  if (pc->old_point >= 0)
    TODO;
  unbind_to (pc->specpdl_count, Qnil);
}

enum print_entry_type
{
  PE_list,
  PE_rbrac,
  PE_vector,
  PE_hash,
};

struct print_stack_entry
{
  enum print_entry_type type;

  union
  {
    struct
    {
      Lisp_Object last;
      intmax_t maxlen;
      Lisp_Object tortoise;
      ptrdiff_t n;
      ptrdiff_t m;
      intmax_t tortoise_idx;
    } list;

    struct
    {
      Lisp_Object obj;
    } dotted_cdr;

    struct
    {
      Lisp_Object obj;
      ptrdiff_t size;
      ptrdiff_t idx;
      const char *end;
      bool truncated;
    } vector;

    struct
    {
      Lisp_Object obj;
      ptrdiff_t nobjs;
      ptrdiff_t idx;
      ptrdiff_t printed;
      bool truncated;
    } hash;
  } u;
};

struct print_stack
{
  struct print_stack_entry *stack;
  ptrdiff_t size;
  ptrdiff_t sp;
};

static struct print_stack prstack = { NULL, 0, 0 };

NO_INLINE static void
grow_print_stack (void)
{
  struct print_stack *ps = &prstack;
  eassert (ps->sp == ps->size);
  ps->stack = xpalloc (ps->stack, &ps->size, 1, -1, sizeof *ps->stack);
  eassert (ps->sp < ps->size);
}

static inline void
print_stack_push (struct print_stack_entry e)
{
  if (prstack.sp >= prstack.size)
    grow_print_stack ();
  prstack.stack[prstack.sp++] = e;
}

static void
strout (const char *ptr, ptrdiff_t size, ptrdiff_t size_byte,
        Lisp_Object printcharfun)
{
  if (NILP (printcharfun))
    {
      ptrdiff_t incr = size_byte - (print_buffer.size - print_buffer.pos_byte);
      if (incr > 0)
        print_buffer.buffer
          = xpalloc (print_buffer.buffer, &print_buffer.size, incr, -1, 1);
      memcpy (print_buffer.buffer + print_buffer.pos_byte, ptr, size_byte);
      print_buffer.pos += size;
      print_buffer.pos_byte += size_byte;
    }
  else
    TODO;
}

static void
print_c_string (char const *string, Lisp_Object printcharfun)
{
  ptrdiff_t len = strlen (string);
  strout (string, len, len, printcharfun);
}

static void
printchar (unsigned int ch, Lisp_Object fun)
{
  if (!NILP (fun) && !EQ (fun, Qt))
    call1 (fun, make_fixnum (ch));
  else
    {
      unsigned char str[MAX_MULTIBYTE_LENGTH];
      int len = CHAR_STRING (ch, str);

      maybe_quit ();

      if (NILP (fun))
        {
          ptrdiff_t incr = len - (print_buffer.size - print_buffer.pos_byte);
          if (incr > 0)
            print_buffer.buffer
              = xpalloc (print_buffer.buffer, &print_buffer.size, incr, -1, 1);
          memcpy (print_buffer.buffer + print_buffer.pos_byte, str, len);
          print_buffer.pos += 1;
          print_buffer.pos_byte += len;
        }
      else
        TODO;
    }
}

int
float_to_string (char *buf, double data)
{
  char *cp;
  int width;
  int len;

  if (isinf (data))
    {
      static char const minus_infinity_string[] = "-1.0e+INF";
      bool positive = 0 < data;
      strcpy (buf, minus_infinity_string + positive);
      return sizeof minus_infinity_string - 1 - positive;
    }
#if IEEE_FLOATING_POINT
  if (isnan (data))
    {
      union ieee754_double u = { .d = data };
      uintmax_t hi = u.ieee_nan.mantissa0;
      return sprintf (buf, &"-%" PRIuMAX ".0e+NaN"[!u.ieee_nan.negative],
                      (hi << 31 << 1) + u.ieee_nan.mantissa1);
    }
#endif

  if (NILP (Vfloat_output_format) || !STRINGP (Vfloat_output_format))
    {
    lose:
      TODO; // len = dtoastr (buf, FLOAT_TO_STRING_BUFSIZE - 2, 0, 0, data);
      width = 1;
    }
  else
    {
      cp = SSDATA (Vfloat_output_format);

      if (cp[0] != '%')
        goto lose;
      if (cp[1] != '.')
        goto lose;

      cp += 2;

      width = -1;
      if ('0' <= *cp && *cp <= '9')
        {
          width = 0;
          do
            {
              width = (width * 10) + (*cp++ - '0');
              if (DBL_DIG < width)
                goto lose;
            }
          while (*cp >= '0' && *cp <= '9');

          if (width == 0 && *cp != 'f')
            goto lose;
        }

      if (*cp != 'e' && *cp != 'f' && *cp != 'g')
        goto lose;

      if (cp[1] != 0)
        goto lose;

      len = sprintf (buf, SSDATA (Vfloat_output_format), data);
    }

  if (width != 0)
    {
      for (cp = buf; *cp; cp++)
        if ((*cp < '0' || *cp > '9') && *cp != '-')
          break;

      if (*cp == '.' && cp[1] == 0)
        {
          cp[1] = '0';
          cp[2] = 0;
          len++;
        }
      else if (*cp == 0)
        {
          *cp++ = '.';
          *cp++ = '0';
          *cp++ = 0;
          len += 2;
        }
    }

  return len;
}

static void
print_object (Lisp_Object obj, Lisp_Object printcharfun, bool escapeflag)
{
  ptrdiff_t base_depth = print_depth;
  ptrdiff_t base_sp = prstack.sp;
  char
    buf[max (sizeof "from..to..in " + 2 * INT_STRLEN_BOUND (EMACS_INT),
             max (sizeof " . #" + INT_STRLEN_BOUND (intmax_t),
                  max ((sizeof " with data 0x" + (UINTMAX_WIDTH + 4 - 1) / 4),
                       40)))];

print_obj:
  maybe_quit ();

  if (NILP (Vprint_circle))
    {
      if (print_depth >= PRINT_CIRCLE)
        error ("Apparently circular structure being printed");

      for (int i = 0; i < print_depth; i++)
        if (BASE_EQ (obj, being_printed[i]))
          {
            int len = sprintf (buf, "#%d", i);
            strout (buf, len, len, printcharfun);
            goto next_obj;
          }
      being_printed[print_depth] = obj;
    }

  print_depth++;

  switch (XTYPE (obj))
    {
    case_Lisp_Int:
      {
        EMACS_INT i = XFIXNUM (obj);

        if (print_integers_as_characters && (TODO, false))
          TODO;
        else
          {
            char *end = buf + sizeof buf;
            char *start = fixnum_to_string (i, buf, end);
            ptrdiff_t len = end - start;
            strout (start, len, len, printcharfun);
          }
      }
      break;

    case Lisp_Float:
      {
        char pigbuf[FLOAT_TO_STRING_BUFSIZE];
        int len = float_to_string (pigbuf, XFLOAT_DATA (obj));
        strout (pigbuf, len, len, printcharfun);
      }
      break;

    case Lisp_String:
      UNUSED (escapeflag);
      TODO;
      break;

    case Lisp_Symbol:
      TODO;
      break;

    case Lisp_Cons:
      TODO;
      break;

    case Lisp_Vectorlike:
      switch (PSEUDOVECTOR_TYPE (XVECTOR (obj)))
        {
        case PVEC_NORMAL_VECTOR:
          TODO;
        case PVEC_RECORD:
          TODO;
        case PVEC_CLOSURE:
          TODO;
        case PVEC_CHAR_TABLE:
          TODO;
        case PVEC_SUB_CHAR_TABLE:
          TODO;
        case PVEC_HASH_TABLE:
          {
            struct Lisp_Hash_Table *h = XHASH_TABLE (obj);
            print_c_string ("#s(hash-table", printcharfun);

            if (!BASE_EQ (h->test->name, Qeql))
              {
                print_c_string (" test ", printcharfun);
                print_object (h->test->name, printcharfun, escapeflag);
              }

            if (h->weakness != Weak_None)
              {
                TODO;
              }

            if (h->purecopy)
              print_c_string (" purecopy t", printcharfun);

            ptrdiff_t size = h->count;
            if (size > 0)
              {
                print_c_string (" data (", printcharfun);

                if (FIXNATP (Vprint_length) && XFIXNAT (Vprint_length) < size)
                  size = XFIXNAT (Vprint_length);

                print_stack_push ((struct print_stack_entry) {
                  .type = PE_hash,
                  .u.hash.obj = obj,
                  .u.hash.nobjs = size * 2,
                  .u.hash.idx = 0,
                  .u.hash.printed = 0,
                  .u.hash.truncated = (size < h->count),
                });
              }
            else
              {
                printchar (')', printcharfun);
                --print_depth; /* Done with this.  */
              }
            goto next_obj;
          }
        case PVEC_BIGNUM:
          TODO;
        case PVEC_BOOL_VECTOR:
          TODO;
        default:
          TODO;
        }
      break;

    default:
      emacs_abort ();
    }

  print_depth--;

next_obj:

  if (prstack.sp > base_sp)
    {
      TODO;
      goto print_obj;
    }
  eassert (print_depth == base_depth);
}

static void
print (Lisp_Object obj, Lisp_Object printcharfun, bool escapeflag)
{
  new_backquote_output = 0;

  if (NILP (Vprint_continuous_numbering) || NILP (Vprint_number_table))
    {
      print_number_index = 0;
      Vprint_number_table = Qnil;
    }

  if (!NILP (Vprint_circle))
    TODO;

  print_depth = 0;
  print_object (obj, printcharfun, escapeflag);
}

DEFUN ("prin1-to-string", Fprin1_to_string, Sprin1_to_string, 1, 3, 0,
       doc: /* Return a string containing the printed representation of OBJECT.
OBJECT can be any Lisp object.  This function outputs quoting characters
when necessary to make output that `read' can handle, whenever possible,
unless the optional second argument NOESCAPE is non-nil.  For complex objects,
the behavior is controlled by `print-level' and `print-length', which see.

OBJECT is any of the Lisp data types: a number, a string, a symbol,
a list, a buffer, a window, a frame, etc.

See `prin1' for the meaning of OVERRIDES.

A printed representation of an object is text which describes that object.  */)
(Lisp_Object object, Lisp_Object noescape, Lisp_Object overrides)
{
  specpdl_ref count = SPECPDL_INDEX ();

  if (!NILP (overrides))
    TODO; // print_bind_overrides (overrides);

  struct print_context pc = print_prepare (Qnil, true);

  print (object, pc.printcharfun, NILP (noescape));

  object = make_string_from_bytes (print_buffer.buffer, print_buffer.pos,
                                   print_buffer.pos_byte);

  print_finish (&pc, true);

  return unbind_to (count, object);
}

void
syms_of_print (void)
{
  DEFSYM (Qprint_escape_multibyte, "print-escape-multibyte");
  DEFSYM (Qprint_escape_nonascii, "print-escape-nonascii");

  DEFVAR_LISP ("float-output-format", Vfloat_output_format,
	       doc: /* The format descriptor string used to print floats.
This is a %-spec like those accepted by `printf' in C,
but with some restrictions.  It must start with the two characters `%.'.
After that comes an integer precision specification,
and then a letter which controls the format.
The letters allowed are `e', `f' and `g'.
Use `e' for exponential notation \"DIG.DIGITSeEXPT\"
Use `f' for decimal point notation \"DIGITS.DIGITS\".
Use `g' to choose the shorter of those two formats for the number at hand.
The precision in any of these cases is the number of digits following
the decimal point.  With `f', a precision of 0 means to omit the
decimal point.  0 is not allowed with `e' or `g'.

A value of nil means to use the shortest notation
that represents the number without losing information.  */);
  Vfloat_output_format = Qnil;

  DEFVAR_BOOL ("print-integers-as-characters", print_integers_as_characters,
	       doc: /* Non-nil means integers are printed using characters syntax.
Only independent graphic characters, and control characters with named
escape sequences such as newline, are printed this way.  Other
integers, including those corresponding to raw bytes, are printed
as numbers the usual way.  */);
  print_integers_as_characters = false;

  DEFVAR_BOOL ("print-escape-nonascii", print_escape_nonascii,
               doc: /* Non-nil means print unibyte non-ASCII chars in strings as \\OOO.
\(OOO is the octal representation of the character code.)
Only single-byte characters are affected, and only in `prin1'.
When the output goes in a multibyte buffer, this feature is
enabled regardless of the value of the variable.  */);
  print_escape_nonascii = 0;

  DEFVAR_BOOL ("print-escape-multibyte", print_escape_multibyte,
               doc: /* Non-nil means print multibyte characters in strings as \\xXXXX.
\(XXXX is the hex representation of the character code.)
This affects only `prin1'.  */);
  print_escape_multibyte = 0;

  DEFVAR_LISP ("print-circle", Vprint_circle,
	       doc: /* Non-nil means print recursive structures using #N= and #N# syntax.
If nil, printing proceeds recursively and may lead to
`max-lisp-eval-depth' being exceeded or an error may occur:
\"Apparently circular structure being printed.\"  Also see
`print-length' and `print-level'.
If non-nil, shared substructures anywhere in the structure are printed
with `#N=' before the first occurrence (in the order of the print
representation) and `#N#' in place of each subsequent occurrence,
where N is a positive decimal integer.  */);
  Vprint_circle = Qnil;

  DEFVAR_LISP ("print-continuous-numbering", Vprint_continuous_numbering,
	       doc: /* Non-nil means number continuously across print calls.
This affects the numbers printed for #N= labels and #M# references.
See also `print-circle', `print-gensym', and `print-number-table'.
This variable should not be set with `setq'; bind it with a `let' instead.  */);
  Vprint_continuous_numbering = Qnil;

  DEFVAR_LISP ("print-number-table", Vprint_number_table,
	       doc: /* A vector used internally to produce `#N=' labels and `#N#' references.
The Lisp printer uses this vector to detect Lisp objects referenced more
than once.  If an entry contains a number, then the corresponding key is
referenced more than once: a positive sign indicates that it's already been
printed, and the absolute value indicates the number to use when printing.
If an entry contains a string, that string is printed instead.

When you bind `print-continuous-numbering' to t, you should probably
also bind `print-number-table' to nil.  This ensures that the value of
`print-number-table' can be garbage-collected once the printing is
done.  If all elements of `print-number-table' are nil, it means that
the printing done so far has not found any shared structure or objects
that need to be recorded in the table.  */);
  Vprint_number_table = Qnil;

  DEFVAR_LISP ("print-length", Vprint_length,
	       doc: /* Maximum length of list to print before abbreviating.
A value of nil means no limit.  See also `eval-expression-print-length'.  */);
  Vprint_length = Qnil;

  defsubr (&Sprin1_to_string);
}
