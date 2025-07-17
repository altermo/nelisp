#include "lisp.h"
#include "buffer.h"
#include "character.h"

#include <c-ctype.h>

static Lisp_Object cached_system_name;

static void
init_and_cache_system_name (void)
{
  init_system_name ();
  cached_system_name = Vsystem_name;
}

DEFUN ("point-max", Fpoint_max, Spoint_max, 0, 0, 0,
       doc: /* Return the maximum permissible value of point in the current buffer.
This is (1+ (buffer-size)), unless narrowing (a buffer restriction)
is in effect, in which case it is less.  */)
(void)
{
  Lisp_Object temp;
  XSETFASTINT (temp, ZV);
  return temp;
}

DEFUN ("system-name", Fsystem_name, Ssystem_name, 0, 0, 0,
       doc: /* Return the host name of the machine you are running on, as a string.  */)
(void)
{
  if (EQ (Vsystem_name, cached_system_name))
    init_and_cache_system_name ();
  return Vsystem_name;
}

Lisp_Object
make_buffer_string_both (ptrdiff_t start, ptrdiff_t start_byte, ptrdiff_t end,
                         ptrdiff_t end_byte, bool props)
{
  Lisp_Object result;
  ptrdiff_t beg0, end0, size;

  // if (start_byte < GPT_BYTE && GPT_BYTE < end_byte)
  //   {
  //     beg0 = start_byte;
  //     end0 = GPT_BYTE;
  //     beg1 = GPT_BYTE + GAP_SIZE - BEG_BYTE;
  //     end1 = end_byte + GAP_SIZE - BEG_BYTE;
  //   }
  // else
  {
    beg0 = start_byte;
    end0 = end_byte;
    // beg1 = -1;
    // end1 = -1;
  }

  if (!NILP (BVAR (current_buffer, enable_multibyte_characters)))
    result = make_uninit_multibyte_string (end - start, end_byte - start_byte);
  else
    result = make_uninit_string (end - start);

  size = end0 - beg0;
  // memcpy (SDATA (result), BYTE_POS_ADDR (beg0), size);
  // if (beg1 != -1)
  //   memcpy (SDATA (result) + size, BEG_ADDR + beg1, end1 - beg1);
  buffer_memcpy (SDATA (result), beg0, size);

  if (props)
    {
#if TODO_NELISP_LATER_AND
      update_buffer_properties (start, end);

      tem
        = Fnext_property_change (make_fixnum (start), Qnil, make_fixnum (end));
      tem1 = Ftext_properties_at (make_fixnum (start), Qnil);

      if (XFIXNUM (tem) != end || !NILP (tem1))
        copy_intervals_to_string (result, current_buffer, start, end - start);
#endif
    }

  return result;
}

DEFUN ("buffer-string", Fbuffer_string, Sbuffer_string, 0, 0, 0,
       doc: /* Return the contents of the current buffer as a string.
If narrowing is in effect, this function returns only the visible part
of the buffer.

This function copies the text properties of that part of the buffer
into the result string; if you don’t want the text properties,
use `buffer-substring-no-properties' instead.  */)
(void) { return make_buffer_string_both (BEGV, BEGV_BYTE, ZV, ZV_BYTE, 1); }

DEFUN ("message", Fmessage, Smessage, 1, MANY, 0,
       doc: /* Display a message at the bottom of the screen.
The message also goes into the `*Messages*' buffer, if `message-log-max'
is non-nil.  (In keyboard macros, that's all it does.)
Return the message.

In batch mode, the message is printed to the standard error stream,
followed by a newline.

The first argument is a format control string, and the rest are data
to be formatted under control of the string.  Percent sign (%), grave
accent (\\=`) and apostrophe (\\=') are special in the format; see
`format-message' for details.  To display STRING without special
treatment, use (message "%s" STRING).

If the first argument is nil or the empty string, the function clears
any existing message; this lets the minibuffer contents show.  See
also `current-message'.

usage: (message FORMAT-STRING &rest ARGS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  if (NILP (args[0]) || (STRINGP (args[0]) && SBYTES (args[0]) == 0))
    {
      message1 (0);
      return args[0];
    }
  else
    {
      Lisp_Object val = Fformat_message (nargs, args);
      message3 (val);
      return val;
    }
}

static ptrdiff_t
str2num (char *str, char **str_end)
{
  ptrdiff_t n = 0;
  for (; c_isdigit (*str); str++)
    if (ckd_mul (&n, n, 10) || ckd_add (&n, n, *str - '0'))
      n = PTRDIFF_MAX;
  *str_end = str;
  return n;
}

static Lisp_Object
styled_format (ptrdiff_t nargs, Lisp_Object *args, bool message)
{
  enum
  {
    USEFUL_PRECISION_MAX = ((1 - LDBL_MIN_EXP)
                            * (FLT_RADIX == 2 || FLT_RADIX == 10 ? 1
                               : FLT_RADIX == 16                 ? 4
                                                                 : -1)),

    SPRINTF_BUFSIZE
    = (sizeof "-." + (LDBL_MAX_10_EXP + 1) + USEFUL_PRECISION_MAX)
  };
  verify (USEFUL_PRECISION_MAX > 0);

  ptrdiff_t n;
  char initial_buffer[1000 + SPRINTF_BUFSIZE];
  char *buf = initial_buffer;
  ptrdiff_t bufsize = sizeof initial_buffer;
  ptrdiff_t max_bufsize = STRING_BYTES_BOUND + 1;
  char *p;
  specpdl_ref buf_save_value_index UNINIT;
  char *format, *end;
  ptrdiff_t nchars;
  bool maybe_combine_byte;
  Lisp_Object val;
  bool arg_intervals = false;
  USE_SAFE_ALLOCA;
  sa_avail -= sizeof initial_buffer;

  struct info
  {
    Lisp_Object argument;

    ptrdiff_t start, end;

    ptrdiff_t fbeg;

    bool_bf intervals : 1;
  } *info;

  CHECK_STRING (args[0]);
  char *format_start = SSDATA (args[0]);
  bool multibyte_format = STRING_MULTIBYTE (args[0]);
  ptrdiff_t formatlen = SBYTES (args[0]);
  bool fmt_props = !!string_intervals (args[0]);

  ptrdiff_t nspec_bound = SCHARS (args[0]) >> 1;

  ptrdiff_t info_size, alloca_size;
  if (ckd_mul (&info_size, nspec_bound, sizeof *info)
      || ckd_add (&alloca_size, formatlen, info_size)
      || SIZE_MAX < (unsigned long) alloca_size)
    memory_full (SIZE_MAX);
  info = SAFE_ALLOCA (alloca_size);

  char *discarded = (char *) &info[nspec_bound];
  memset (discarded, 0, formatlen);

  bool multibyte = multibyte_format;
  for (ptrdiff_t i = 1; !multibyte && i < nargs; i++)
    if (STRINGP (args[i]) && STRING_MULTIBYTE (args[i]))
      multibyte = true;

  Lisp_Object quoting_style = message ? Ftext_quoting_style () : Qnil;

  ptrdiff_t ispec;
  ptrdiff_t nspec = 0;

  bool new_result = false;

retry:

  p = buf;
  nchars = 0;

  n = 0;
  ispec = 0;

  format = format_start;
  end = format + formatlen;
  maybe_combine_byte = false;

  while (format != end)
    {
      ptrdiff_t n0 = n;
      ptrdiff_t ispec0 = ispec;
      char *format0 = format;
      char const *convsrc = format;
      unsigned char format_char = *format++;

      ptrdiff_t convbytes = 1;
      enum
      {
        CONVBYTES_ROOM = SPRINTF_BUFSIZE - 1
      };
      eassert (p <= buf + bufsize - SPRINTF_BUFSIZE);

      if (format_char == '%')
        {
          ptrdiff_t num;
          char *num_end;
          if (c_isdigit (*format))
            {
              num = str2num (format, &num_end);
              if (*num_end == '$')
                {
                  n = num - 1;
                  format = num_end + 1;
                }
            }

          bool minus_flag = false;
          bool plus_flag = false;
          bool space_flag = false;
          bool sharp_flag = false;
          bool zero_flag = false;

          for (;; format++)
            {
              switch (*format)
                {
                case '-':
                  minus_flag = true;
                  continue;
                case '+':
                  plus_flag = true;
                  continue;
                case ' ':
                  space_flag = true;
                  continue;
                case '#':
                  sharp_flag = true;
                  continue;
                case '0':
                  zero_flag = true;
                  continue;
                }
              break;
            }

          space_flag &= !plus_flag;
          zero_flag &= !minus_flag;

          num = str2num (format, &num_end);
          if (max_bufsize <= num)
            string_overflow ();
          ptrdiff_t field_width = num;

          bool precision_given = *num_end == '.';
          ptrdiff_t precision
            = (precision_given ? str2num (num_end + 1, &num_end) : PTRDIFF_MAX);
          format = num_end;

          if (format == end)
            error ("Format string ends in middle of format specifier");

          char conversion = *format++;
          memset (&discarded[format0 - format_start], 1,
                  format - format0 - (conversion == '%'));
          info[ispec].fbeg = format0 - format_start;
          if (conversion == '%')
            {
              new_result = true;
              goto copy_char;
            }

          ++n;
          if (!(n < nargs))
            error ("Not enough arguments for format string");

          struct info *spec = &info[ispec++];
          if (nspec < ispec)
            {
              spec->argument = args[n];
              spec->intervals = false;
              nspec = ispec;
            }
          Lisp_Object arg = spec->argument;

          if ((conversion == 'S'
               || (conversion == 's' && !STRINGP (arg) && !SYMBOLP (arg))))
            {
              if (EQ (arg, args[n]))
                {
                  Lisp_Object noescape = conversion == 'S' ? Qnil : Qt;
                  spec->argument = arg = Fprin1_to_string (arg, noescape, Qnil);
                  if (STRING_MULTIBYTE (arg) && !multibyte)
                    {
                      multibyte = true;
                      goto retry;
                    }
                }
              conversion = 's';
            }
          else if (conversion == 'c')
            {
              if (FIXNUMP (arg) && !ASCII_CHAR_P (XFIXNUM (arg)))
                {
                  if (!multibyte)
                    {
                      multibyte = true;
                      goto retry;
                    }
                  spec->argument = arg = Fchar_to_string (arg);
                }

              if (!EQ (arg, args[n]))
                conversion = 's';
              zero_flag = false;
            }

          if (SYMBOLP (arg))
            {
              spec->argument = arg = SYMBOL_NAME (arg);
              if (STRING_MULTIBYTE (arg) && !multibyte)
                {
                  multibyte = true;
                  goto retry;
                }
            }

          bool float_conversion
            = conversion == 'e' || conversion == 'f' || conversion == 'g';

          if (conversion == 's')
            {
              if (format == end && format - format_start == 2
                  && !string_intervals (args[0]))
                {
                  val = arg;
                  goto return_val;
                }

              ptrdiff_t prec = -1;
              if (precision_given)
                prec = precision;

              ptrdiff_t width, nbytes;
              ptrdiff_t nchars_string;
              if (prec == 0)
                width = nchars_string = nbytes = 0;
              else
                {
                  ptrdiff_t nch, nby;
                  nchars_string = SCHARS (arg);
                  width = lisp_string_width (arg, 0, nchars_string, prec, &nch,
                                             &nby, false);
                  if (prec < 0)
                    nbytes = SBYTES (arg);
                  else
                    {
                      nchars_string = nch;
                      nbytes = nby;
                    }
                }

              convbytes = nbytes;
              if (convbytes && multibyte && !STRING_MULTIBYTE (arg))
                TODO; // convbytes = count_size_as_multibyte (SDATA (arg),
                      // nbytes);

              ptrdiff_t padding = width < field_width ? field_width - width : 0;

              if (max_bufsize - padding <= convbytes)
                string_overflow ();
              convbytes += padding;
              if (convbytes <= buf + bufsize - p)
                {
                  if (fmt_props)
                    spec->start = nchars;
                  if (!minus_flag)
                    {
                      memset (p, ' ', padding);
                      p += padding;
                      nchars += padding;
                    }
                  if (!fmt_props)
                    spec->start = nchars;

                  if (p > buf && multibyte
                      && !ASCII_CHAR_P (*((unsigned char *) p - 1))
                      && STRING_MULTIBYTE (arg) && !CHAR_HEAD_P (SREF (arg, 0)))
                    maybe_combine_byte = true;

                  p += copy_text (SDATA (arg), (unsigned char *) p, nbytes,
                                  STRING_MULTIBYTE (arg), multibyte);

                  nchars += nchars_string;

                  if (minus_flag)
                    {
                      memset (p, ' ', padding);
                      p += padding;
                      nchars += padding;
                    }
                  spec->end = nchars;

                  if (string_intervals (arg))
                    spec->intervals = arg_intervals = true;

                  new_result = true;
                  convbytes = CONVBYTES_ROOM;
                }
            }
          else if (!(conversion == 'c' || conversion == 'd' || float_conversion
                     || conversion == 'i' || conversion == 'o'
                     || conversion == 'x' || conversion == 'X'))
            {
              unsigned char *p = (unsigned char *) format - 1;
              if (multibyte_format)
                error ("Invalid format operation %%%c", STRING_CHAR (p));
              else
                error (*p <= 127 ? "Invalid format operation %%%c"
                                 : "Invalid format operation char #o%03o",
                       *p);
            }
          else if (!(FIXNUMP (arg)
                     || ((BIGNUMP (arg) || FLOATP (arg)) && conversion != 'c')))
            error ("Format specifier doesn't match argument type");
          else
            {
              enum
              {
                pMlen = sizeof PRIdMAX - 2
              };

              if (conversion == 'd' || conversion == 'i')
                sharp_flag = false;

              char convspec[sizeof "%FF.*d" + max (sizeof "L" - 1, pMlen)];
              char *f = convspec;
              *f++ = '%';
              *f = '+';
              f += plus_flag;
              *f = ' ';
              f += space_flag;
              *f = '#';
              f += sharp_flag;
              *f++ = '.';
              *f++ = '*';
              if (!(float_conversion || conversion == 'c'))
                {
                  memcpy (f, PRIdMAX, pMlen);
                  f += pMlen;
                  zero_flag &= !precision_given;
                }
              *f++ = conversion;
              *f = '\0';

              int prec = -1;
              if (precision_given)
                prec = min (precision, USEFUL_PRECISION_MAX);

              char prefix[sizeof "-0x" - 1];
              int prefixlen = 0;

              ptrdiff_t sprintf_bytes;
              if (float_conversion)
                {
                  bool format_as_long_double = false;
                  double darg;
                  long double ldarg UNINIT;

                  if (FLOATP (arg))
                    darg = XFLOAT_DATA (arg);
                  else
                    {
                      bool format_bignum_as_double = false;
                      if (LDBL_MANT_DIG <= DBL_MANT_DIG)
                        {
                          if (FIXNUMP (arg))
                            darg = XFIXNUM (arg);
                          else
                            format_bignum_as_double = true;
                        }
                      else
                        {
                          if (INTEGERP (arg))
                            {
                              intmax_t iarg;
                              uintmax_t uarg;
                              if (integer_to_intmax (arg, &iarg))
                                ldarg = iarg;
                              else if (integer_to_uintmax (arg, &uarg))
                                ldarg = uarg;
                              else
                                format_bignum_as_double = true;
                            }
                          if (!format_bignum_as_double)
                            {
                              darg = ldarg;
                              format_as_long_double = darg != ldarg;
                            }
                        }
                      if (format_bignum_as_double)
                        TODO; // darg = bignum_to_double (arg);
                    }

                  if (format_as_long_double)
                    {
                      f[-1] = 'L';
                      *f++ = conversion;
                      *f = '\0';
                      sprintf_bytes = sprintf (p, convspec, prec, ldarg);
                    }
                  else
                    sprintf_bytes = sprintf (p, convspec, prec, darg);
                }
              else if (conversion == 'c')
                {
                  p[0] = XFIXNUM (arg);
                  p[1] = '\0';
                  sprintf_bytes = prec != 0;
                }
              else if (BIGNUMP (arg))
                // bignum_arg:
                {
                  int base = ((conversion == 'd' || conversion == 'i') ? 10
                              : conversion == 'o'                      ? 8
                                                                       : 16);
                  TODO; // sprintf_bytes = bignum_bufsize (arg, base);
                  if (sprintf_bytes <= buf + bufsize - p)
                    {
                      TODO; // int signedbase = conversion == 'X' ? -base :
                            // base;
                      // sprintf_bytes = bignum_to_c_string (p, sprintf_bytes,
                      // arg, signedbase);
                      bool negative = p[0] == '-';
                      prec = min (precision, sprintf_bytes - prefixlen);
                      prefix[prefixlen] = plus_flag ? '+' : ' ';
                      prefixlen += (plus_flag | space_flag) & !negative;
                      prefix[prefixlen] = '0';
                      prefix[prefixlen + 1] = conversion;
                      prefixlen += sharp_flag && base == 16 ? 2 : 0;
                    }
                }
              else if (conversion == 'd' || conversion == 'i')
                {
                  if (FIXNUMP (arg))
                    {
                      intmax_t x = XFIXNUM (arg);
                      sprintf_bytes = sprintf (p, convspec, prec, x);
                    }
                  else
                    {
                      strcpy (f - pMlen - 1, "f");
                      double x = XFLOAT_DATA (arg);

                      TODO; // x = trunc (x);
                      x = x ? x : 0;

                      sprintf_bytes = sprintf (p, convspec, 0, x);
                      bool signedp = !c_isdigit (p[0]);
                      prec = min (precision, sprintf_bytes - signedp);
                    }
                }
              else
                {
                  uintmax_t x;
                  bool negative;
                  if (FIXNUMP (arg))
                    {
                      if (binary_as_unsigned)
                        {
                          x = XUFIXNUM (arg);
                          negative = false;
                        }
                      else
                        {
                          EMACS_INT i = XFIXNUM (arg);
                          negative = i < 0;
                          x = negative ? -i : i;
                        }
                    }
                  else
                    {
                      TODO; // double d = XFLOAT_DATA (arg);
                      // double abs_d = fabs (d);
                      //  if (abs_d < UINTMAX_MAX + 1.0)
                      //    {
                      //      negative = d <= -1;
                      //      x = abs_d;
                      //    }
                      //  else
                      //    {
                      //      arg = double_to_integer (d);
                      //      goto bignum_arg;
                      //    }
                    }
                  p[0] = negative ? '-' : plus_flag ? '+' : ' ';
                  bool signedp = negative | plus_flag | space_flag;
                  sprintf_bytes = sprintf (p + signedp, convspec, prec, x);
                  sprintf_bytes += signedp;
                }

              ptrdiff_t excess_precision
                = precision_given ? precision - prec : 0;
              ptrdiff_t trailing_zeros = 0;
              if (excess_precision != 0 && float_conversion)
                {
                  if (!c_isdigit (p[sprintf_bytes - 1])
                      || (conversion == 'g'
                          && !(sharp_flag && strchr (p, '.'))))
                    excess_precision = 0;
                  trailing_zeros = excess_precision;
                }
              ptrdiff_t leading_zeros = excess_precision - trailing_zeros;

              ptrdiff_t numwidth;
              if (ckd_add (&numwidth, prefixlen + sprintf_bytes,
                           excess_precision))
                numwidth = PTRDIFF_MAX;
              ptrdiff_t padding
                = numwidth < field_width ? field_width - numwidth : 0;
              if (max_bufsize - (prefixlen + sprintf_bytes) <= excess_precision
                  || max_bufsize - padding <= numwidth)
                string_overflow ();
              convbytes = numwidth + padding;

              if (convbytes <= buf + bufsize - p)
                {
                  bool signedp = p[0] == '-' || p[0] == '+' || p[0] == ' ';
                  int beglen
                    = (signedp
                       + ((p[signedp] == '0'
                           && (p[signedp + 1] == 'x' || p[signedp + 1] == 'X'))
                            ? 2
                            : 0));
                  eassert (prefixlen == 0 || beglen == 0
                           || (beglen == 1 && p[0] == '-'
                               && !(prefix[0] == '-' || prefix[0] == '+'
                                    || prefix[0] == ' ')));
                  if (zero_flag && 0 <= char_hexdigit (p[beglen]))
                    {
                      leading_zeros += padding;
                      padding = 0;
                    }
                  if (leading_zeros == 0 && sharp_flag && conversion == 'o'
                      && p[beglen] != '0')
                    {
                      leading_zeros++;
                      padding -= padding != 0;
                    }

                  int endlen = 0;
                  if (trailing_zeros
                      && (conversion == 'e' || conversion == 'g'))
                    {
                      char *e = strchr (p, 'e');
                      if (e)
                        endlen = p + sprintf_bytes - e;
                    }

                  ptrdiff_t midlen = sprintf_bytes - beglen - endlen;
                  ptrdiff_t leading_padding = minus_flag ? 0 : padding;
                  ptrdiff_t trailing_padding = padding - leading_padding;

                  ptrdiff_t incr = (padding + leading_zeros + prefixlen
                                    + sprintf_bytes + trailing_zeros);

                  if (incr != sprintf_bytes)
                    {
                      char *src = p + sprintf_bytes;
                      char *dst = p + incr;
                      dst -= trailing_padding;
                      memset (dst, ' ', trailing_padding);
                      src -= endlen;
                      dst -= endlen;
                      memmove (dst, src, endlen);
                      dst -= trailing_zeros;
                      memset (dst, '0', trailing_zeros);
                      src -= midlen;
                      dst -= midlen;
                      memmove (dst, src, midlen);
                      dst -= leading_zeros;
                      memset (dst, '0', leading_zeros);
                      dst -= prefixlen;
                      memcpy (dst, prefix, prefixlen);
                      src -= beglen;
                      dst -= beglen;
                      memmove (dst, src, beglen);
                      dst -= leading_padding;
                      memset (dst, ' ', leading_padding);
                    }

                  p += incr;
                  spec->start = nchars;
                  spec->end = nchars += incr;
                  new_result = true;
                  convbytes = CONVBYTES_ROOM;
                }
            }
        }
      else
        {
          unsigned char str[MAX_MULTIBYTE_LENGTH];

          if ((format_char == '`' || format_char == '\'')
              && EQ (quoting_style, Qcurve))
            {
              if (!multibyte)
                {
                  multibyte = true;
                  goto retry;
                }
              TODO; // convsrc = format_char == '`' ? uLSQM : uRSQM;
              convbytes = 3;
              new_result = true;
            }
          else if (format_char == '`' && EQ (quoting_style, Qstraight))
            {
              convsrc = "'";
              new_result = true;
            }
          else
            {
              if (multibyte_format)
                {
                  if (p > buf && !ASCII_CHAR_P (*((unsigned char *) p - 1))
                      && !CHAR_HEAD_P (format_char))
                    maybe_combine_byte = true;

                  while (!CHAR_HEAD_P (*format))
                    format++;

                  convbytes = format - format0;
                  memset (&discarded[format0 + 1 - format_start], 2,
                          convbytes - 1);
                }
              else if (multibyte && !ASCII_CHAR_P (format_char))
                {
                  int c = BYTE8_TO_CHAR (format_char);
                  convbytes = CHAR_STRING (c, str);
                  convsrc = (char *) str;
                  new_result = true;
                }
            }

        copy_char:
          memcpy (p, convsrc, convbytes);
          p += convbytes;
          nchars++;
          convbytes = CONVBYTES_ROOM;
        }

      ptrdiff_t used = p - buf;
      ptrdiff_t buflen_needed;
      if (ckd_add (&buflen_needed, used, convbytes))
        string_overflow ();
      if (bufsize <= buflen_needed)
        {
          if (max_bufsize <= buflen_needed)
            string_overflow ();

          bufsize = (buflen_needed <= max_bufsize / 2 ? buflen_needed * 2
                                                      : max_bufsize);

          if (buf == initial_buffer)
            {
              buf = xmalloc (bufsize);
              buf_save_value_index = SPECPDL_INDEX ();
              record_unwind_protect_ptr (xfree, buf);
              memcpy (buf, initial_buffer, used);
            }
          else
            {
              buf = xrealloc (buf, bufsize);
              set_unwind_protect_ptr (buf_save_value_index, xfree, buf);
            }

          p = buf + used;
          if (convbytes != CONVBYTES_ROOM)
            {
              eassert (CONVBYTES_ROOM < convbytes);
              format = format0;
              n = n0;
              ispec = ispec0;
            }
        }
    }

  if (bufsize < p - buf)
    emacs_abort ();

  if (!new_result)
    {
      val = args[0];
      goto return_val;
    }

  if (maybe_combine_byte)
    TODO; // nchars = multibyte_chars_in_text ((unsigned char *) buf, p - buf);
  val = make_specified_string (buf, nchars, p - buf, multibyte);

  if (string_intervals (args[0]) || arg_intervals)
    {
      TODO;
    }

return_val:
  SAFE_FREE ();

  return val;
}

DEFUN ("format", Fformat, Sformat, 1, MANY, 0,
       doc: /* Format a string out of a format-string and arguments.
The first argument is a format control string.
The other arguments are substituted into it to make the result, a string.

The format control string may contain %-sequences meaning to substitute
the next available argument, or the argument explicitly specified:

%s means produce a string argument.  Actually, produces any object with `princ'.
%d means produce as signed number in decimal.
%o means produce a number in octal.
%x means produce a number in hex.
%X is like %x, but uses upper case.
%e means produce a number in exponential notation.
%f means produce a number in decimal-point notation.
%g means produce a number in exponential notation if the exponent would be
   less than -4 or greater than or equal to the precision (default: 6);
   otherwise it produces in decimal-point notation.
%c means produce a number as a single character.
%S means produce any object as an s-expression (using `prin1').

The argument used for %d, %o, %x, %e, %f, %g or %c must be a number.
%o, %x, and %X treat arguments as unsigned if `binary-as-unsigned' is t
  (this is experimental; email 32252@debbugs.gnu.org if you need it).
Use %% to put a single % into the output.

A %-sequence other than %% may contain optional field number, flag,
width, and precision specifiers, as follows:

  %<field><flags><width><precision>character

where field is [0-9]+ followed by a literal dollar "$", flags is
[+ #0-]+, width is [0-9]+, and precision is a literal period "."
followed by [0-9]+.

If a %-sequence is numbered with a field with positive value N, the
Nth argument is substituted instead of the next one.  A format can
contain either numbered or unnumbered %-sequences but not both, except
that %% can be mixed with numbered %-sequences.

The + flag character inserts a + before any nonnegative number, while a
space inserts a space before any nonnegative number; these flags
affect only numeric %-sequences, and the + flag takes precedence.
The - and 0 flags affect the width specifier, as described below.

The # flag means to use an alternate display form for %o, %x, %X, %e,
%f, and %g sequences: for %o, it ensures that the result begins with
\"0\"; for %x and %X, it prefixes nonzero results with \"0x\" or \"0X\";
for %e and %f, it causes a decimal point to be included even if the
precision is zero; for %g, it causes a decimal point to be
included even if the precision is zero, and also forces trailing
zeros after the decimal point to be left in place.

The width specifier supplies a lower limit for the length of the
produced representation.  The padding, if any, normally goes on the
left, but it goes on the right if the - flag is present.  The padding
character is normally a space, but it is 0 if the 0 flag is present.
The 0 flag is ignored if the - flag is present, or the format sequence
is something other than %d, %o, %x, %e, %f, and %g.

For %e and %f sequences, the number after the "." in the precision
specifier says how many decimal places to show; if zero, the decimal
point itself is omitted.  For %g, the precision specifies how many
significant digits to produce; zero or omitted are treated as 1.
For %s and %S, the precision specifier truncates the string to the
given width.

Text properties, if any, are copied from the format-string to the
produced text.

usage: (format STRING &rest OBJECTS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  return styled_format (nargs, args, false);
}

DEFUN ("format-message", Fformat_message, Sformat_message, 1, MANY, 0,
       doc: /* Format a string out of a format-string and arguments.
The first argument is a format control string.
The other arguments are substituted into it to make the result, a string.

This acts like `format', except it also replaces each grave accent (\\=`)
by a left quote, and each apostrophe (\\=') by a right quote.  The left
and right quote replacement characters are specified by
`text-quoting-style'.

usage: (format-message STRING &rest OBJECTS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  return styled_format (nargs, args, true);
}

DEFUN ("current-message", Fcurrent_message, Scurrent_message, 0, 0, 0,
       doc: /* Return the string currently displayed in the echo area, or nil if none.  */)
(void) { return current_message (); }

DEFUN ("propertize", Fpropertize, Spropertize, 1, MANY, 0,
       doc: /* Return a copy of STRING with text properties added.
First argument is the string to copy.
Remaining arguments form a sequence of PROPERTY VALUE pairs for text
properties to add to the result.

See Info node `(elisp) Text Properties' for more information.
usage: (propertize STRING &rest PROPERTIES)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  Lisp_Object properties, string;
  ptrdiff_t i;

  if ((nargs & 1) == 0)
    xsignal2 (Qwrong_number_of_arguments, Qpropertize, make_fixnum (nargs));

  properties = string = Qnil;

  CHECK_STRING (args[0]);
  string = Fcopy_sequence (args[0]);

  for (i = 1; i < nargs; i += 2)
    properties = Fcons (args[i], Fcons (args[i + 1], properties));

  Fadd_text_properties (make_fixnum (0), make_fixnum (SCHARS (string)),
                        properties, string);
  return string;
}

DEFUN ("char-equal", Fchar_equal, Schar_equal, 2, 2, 0,
       doc: /* Return t if two characters match, optionally ignoring case.
Both arguments must be characters (i.e. integers).
Case is ignored if `case-fold-search' is non-nil in the current buffer.  */)
(register Lisp_Object c1, Lisp_Object c2)
{
  int i1, i2;

  CHECK_CHARACTER (c1);
  CHECK_CHARACTER (c2);

  if (XFIXNUM (c1) == XFIXNUM (c2))
    return Qt;
  if (NILP (Vcase_fold_search))
    return Qnil;

  i1 = XFIXNAT (c1);
  i2 = XFIXNAT (c2);

  if (NILP (BVAR (current_buffer, enable_multibyte_characters)))
    {
      if (SINGLE_BYTE_CHAR_P (i1))
        i1 = UNIBYTE_TO_CHAR (i1);
      if (SINGLE_BYTE_CHAR_P (i2))
        i2 = UNIBYTE_TO_CHAR (i2);
    }

  return (downcase (i1) == downcase (i2) ? Qt : Qnil);
}

DEFUN ("char-to-string", Fchar_to_string, Schar_to_string, 1, 1, 0,
       doc: /* Convert arg CHAR to a string containing that character.
usage: (char-to-string CHAR)  */)
(Lisp_Object character)
{
  int c, len;
  unsigned char str[MAX_MULTIBYTE_LENGTH];

  CHECK_CHARACTER (character);
  c = XFIXNAT (character);

  len = CHAR_STRING (c, str);
  return make_string_from_bytes ((char *) str, 1, len);
}

void
syms_of_editfns (void)
{
  DEFSYM (Qpropertize, "propertize");

  DEFVAR_LISP ("system-name", Vsystem_name,
          doc: /* The host name of the machine Emacs is running on.  */);
  Vsystem_name = cached_system_name = Qnil;

  DEFVAR_BOOL ("binary-as-unsigned",
	       binary_as_unsigned,
	       doc: /* Non-nil means `format' %x and %o treat integers as unsigned.
This has machine-dependent results.  Nil means to treat integers as
signed, which is portable and is the default; for example, if N is a
negative integer, (read (format "#x%x" N)) returns N only when this
variable is nil.

This variable is experimental; email 32252@debbugs.gnu.org if you need
it to be non-nil.  */);
  binary_as_unsigned = false;

  defsubr (&Spoint_max);
  defsubr (&Ssystem_name);
  defsubr (&Sbuffer_string);
  defsubr (&Smessage);
  defsubr (&Sformat);
  defsubr (&Sformat_message);
  defsubr (&Scurrent_message);
  defsubr (&Spropertize);
  defsubr (&Schar_equal);
  defsubr (&Schar_to_string);
}
