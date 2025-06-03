#include "character.h"
#include "buffer.h"

Lisp_Object Vchar_unify_table;

EMACS_INT
char_resolve_modifier_mask (EMACS_INT c)
{
  if (!ASCII_CHAR_P ((c & ~CHAR_MODIFIER_MASK)))
    return c;

  if (c & CHAR_SHIFT)
    {
      if ((c & 0377) >= 'A' && (c & 0377) <= 'Z')
        c &= ~CHAR_SHIFT;
      else if ((c & 0377) >= 'a' && (c & 0377) <= 'z')
        c = (c & ~CHAR_SHIFT) - ('a' - 'A');
      else if ((c & ~CHAR_MODIFIER_MASK) <= 0x20)
        c &= ~CHAR_SHIFT;
    }
  if (c & CHAR_CTL)
    {
      if ((c & 0377) == ' ')
        c &= ~0177 & ~CHAR_CTL;
      else if ((c & 0377) == '?')
        c = 0177 | (c & ~0177 & ~CHAR_CTL);
      else if ((c & 0137) >= 0101 && (c & 0137) <= 0132)
        c &= (037 | (~0177 & ~CHAR_CTL));
      else if ((c & 0177) >= 0100 && (c & 0177) <= 0137)
        c &= (037 | (~0177 & ~CHAR_CTL));
    }

  return c;
}
int
char_string (unsigned int c, unsigned char *p)
{
  int bytes;

  if (c & CHAR_MODIFIER_MASK)
    {
      c = char_resolve_modifier_mask (c);
      /* If C still has any modifier bits, just ignore it.  */
      c &= ~CHAR_MODIFIER_MASK;
    }

  if (c <= MAX_3_BYTE_CHAR)
    {
      bytes = CHAR_STRING (c, p);
    }
  else if (c <= MAX_4_BYTE_CHAR)
    {
      p[0] = (0xF0 | (c >> 18));
      p[1] = (0x80 | ((c >> 12) & 0x3F));
      p[2] = (0x80 | ((c >> 6) & 0x3F));
      p[3] = (0x80 | (c & 0x3F));
      bytes = 4;
    }
  else if (c <= MAX_5_BYTE_CHAR)
    {
      p[0] = 0xF8;
      p[1] = (0x80 | ((c >> 18) & 0x0F));
      p[2] = (0x80 | ((c >> 12) & 0x3F));
      p[3] = (0x80 | ((c >> 6) & 0x3F));
      p[4] = (0x80 | (c & 0x3F));
      bytes = 5;
    }
  else if (c <= MAX_CHAR)
    {
      c = CHAR_TO_BYTE8 (c);
      bytes = BYTE8_STRING (c, p);
    }
  else
    error ("Invalid character: %x", c);

  return bytes;
}

ptrdiff_t
str_as_unibyte (unsigned char *str, ptrdiff_t bytes)
{
  const unsigned char *p = str, *endp = str + bytes;
  unsigned char *to;
  int c, len;

  while (p < endp)
    {
      c = *p;
      len = BYTES_BY_CHAR_HEAD (c);
      if (CHAR_BYTE8_HEAD_P (c))
        break;
      p += len;
    }
  to = str + (p - str);
  while (p < endp)
    {
      c = *p;
      len = BYTES_BY_CHAR_HEAD (c);
      if (CHAR_BYTE8_HEAD_P (c))
        {
          c = string_char_advance (&p);
          *to++ = CHAR_TO_BYTE8 (c);
        }
      else
        {
          while (len--)
            *to++ = *p++;
        }
    }
  return (to - str);
}
INLINE int
multibyte_length (unsigned char const *p, unsigned char const *pend, bool check,
                  bool allow_8bit)
{
  if (!check || p < pend)
    {
      unsigned char c = p[0];
      if (c < 0x80)
        return 1;
      if (!check || p + 1 < pend)
        {
          unsigned char d = p[1];
          int w = ((d & 0xC0) << 2) + c;
          if ((allow_8bit ? 0x2C0 : 0x2C2) <= w && w <= 0x2DF)
            return 2;
          if (!check || p + 2 < pend)
            {
              unsigned char e = p[2];
              w += (e & 0xC0) << 4;
              int w1 = w | ((d & 0x20) >> 2);
              if (0xAE1 <= w1 && w1 <= 0xAEF)
                return 3;
              if (!check || p + 3 < pend)
                {
                  unsigned char f = p[3];
                  w += (f & 0xC0) << 6;
                  int w2 = w | ((d & 0x30) >> 3);
                  if (0x2AF1 <= w2 && w2 <= 0x2AF7)
                    return 4;
                  if (!check || p + 4 < pend)
                    {
                      int_fast64_t lw = w + ((p[4] & 0xC0) << 8),
                                   w3 = (lw << 24) + (d << 16) + (e << 8) + f;
                      if (0xAAF8888080 <= w3 && w3 <= 0xAAF88FBFBD)
                        return 5;
                    }
                }
            }
        }
    }

  return 0;
}
void
parse_str_as_multibyte (const unsigned char *str, ptrdiff_t len,
                        ptrdiff_t *nchars, ptrdiff_t *nbytes)
{
  const unsigned char *endp = str + len;
  ptrdiff_t chars = 0, bytes = 0;

  if (len >= MAX_MULTIBYTE_LENGTH)
    {
      const unsigned char *adjusted_endp = endp - (MAX_MULTIBYTE_LENGTH - 1);
      while (str < adjusted_endp)
        {
          int n = multibyte_length (str, NULL, false, false);
          if (0 < n)
            str += n, bytes += n;
          else
            str++, bytes += 2;
          chars++;
        }
    }
  while (str < endp)
    {
      int n = multibyte_length (str, endp, true, false);
      if (0 < n)
        str += n, bytes += n;
      else
        str++, bytes += 2;
      chars++;
    }

  *nchars = chars;
  *nbytes = bytes;
  return;
}
ptrdiff_t
str_to_multibyte (unsigned char *dst, const unsigned char *src,
                  ptrdiff_t nchars)
{
  unsigned char *d = dst;
  for (ptrdiff_t i = 0; i < nchars; i++)
    {
      unsigned char c = src[i];
      if (c <= 0x7f)
        *d++ = c;
      else
        {
          *d++ = 0xc0 + ((c >> 6) & 1);
          *d++ = 0x80 + (c & 0x3f);
        }
    }
  return d - dst;
}
ptrdiff_t
count_size_as_multibyte (const unsigned char *str, ptrdiff_t len)
{
  ptrdiff_t nonascii = 0;
  for (ptrdiff_t i = 0; i < len; i++)
    nonascii += str[i] >> 7;
  ptrdiff_t bytes;
  if (ckd_add (&bytes, len, nonascii))
    TODO; // string_overflow ();
  return bytes;
}

static ptrdiff_t
string_count_byte8 (Lisp_Object string)
{
  bool multibyte = STRING_MULTIBYTE (string);
  ptrdiff_t nbytes = SBYTES (string);
  unsigned char *p = SDATA (string);
  unsigned char *pend = p + nbytes;
  ptrdiff_t count = 0;
  int c, len;

  if (multibyte)
    while (p < pend)
      {
        c = *p;
        len = BYTES_BY_CHAR_HEAD (c);

        if (CHAR_BYTE8_HEAD_P (c))
          count++;
        p += len;
      }
  else
    while (p < pend)
      {
        if (*p++ >= 0x80)
          count++;
      }
  return count;
}

Lisp_Object
string_escape_byte8 (Lisp_Object string)
{
  ptrdiff_t nchars = SCHARS (string);
  ptrdiff_t nbytes = SBYTES (string);
  bool multibyte = STRING_MULTIBYTE (string);
  ptrdiff_t byte8_count;
  ptrdiff_t uninit_nchars = 0;
  ptrdiff_t uninit_nbytes = 0;
  ptrdiff_t thrice_byte8_count;
  const unsigned char *src, *src_end;
  unsigned char *dst;
  Lisp_Object val;
  int c, len;

  if (multibyte && nchars == nbytes)
    return string;

  byte8_count = string_count_byte8 (string);

  if (byte8_count == 0)
    return string;

  if (ckd_mul (&thrice_byte8_count, byte8_count, 3))
    string_overflow ();

  if (multibyte)
    {
      if (ckd_add (&uninit_nchars, nchars, thrice_byte8_count)
          || ckd_add (&uninit_nbytes, nbytes, 2 * byte8_count))
        string_overflow ();
      val = make_uninit_multibyte_string (uninit_nchars, uninit_nbytes);
    }
  else
    {
      if (ckd_add (&uninit_nbytes, thrice_byte8_count, nbytes))
        string_overflow ();
      val = make_uninit_string (uninit_nbytes);
    }

  src = SDATA (string);
  src_end = src + nbytes;
  dst = SDATA (val);
  if (multibyte)
    while (src < src_end)
      {
        c = *src;
        len = BYTES_BY_CHAR_HEAD (c);

        if (CHAR_BYTE8_HEAD_P (c))
          {
            c = string_char_advance (&src);
            c = CHAR_TO_BYTE8 (c);
            dst += sprintf ((char *) dst, "\\%03o", c + 0u);
          }
        else
          while (len--)
            *dst++ = *src++;
      }
  else
    while (src < src_end)
      {
        c = *src++;
        if (c >= 0x80)
          dst += sprintf ((char *) dst, "\\%03o", c + 0u);
        else
          *dst++ = c;
      }
  return val;
}

static ptrdiff_t
char_width (int c, struct Lisp_Char_Table *dp)
{
  ptrdiff_t width = CHARACTER_WIDTH (c);

  if (dp)
    TODO;
  return width;
}

ptrdiff_t
lisp_string_width (Lisp_Object string, ptrdiff_t from, ptrdiff_t to,
                   ptrdiff_t precision, ptrdiff_t *nchars, ptrdiff_t *nbytes,
                   bool auto_comp)
{
  UNUSED (auto_comp);
  bool multibyte = SCHARS (string) < SBYTES (string);
  ptrdiff_t i = from, i_byte = from ? string_char_to_byte (string, from) : 0;
  ptrdiff_t from_byte = i_byte;
  ptrdiff_t width = 0;
#if TODO_NELISP_LATER_AND
  struct Lisp_Char_Table *dp = buffer_display_table ();
#else
  struct Lisp_Char_Table *dp = NULL;
#endif

  eassert (precision <= 0 || (nchars && nbytes));

  while (i < to)
    {
      ptrdiff_t chars, bytes, thiswidth;
#if TODO_NELISP_LATER_AND
      Lisp_Object val;
      ptrdiff_t cmp_id;
      ptrdiff_t ignore, end;

      if (find_composition (i, -1, &ignore, &end, &val, string)
          && ((cmp_id = get_composition_id (i, i_byte, end - i, val, string))
              >= 0))
        {
          thiswidth = composition_table[cmp_id]->width;
          chars = end - i;
          bytes = string_char_to_byte (string, end) - i_byte;
        }
      else
#endif
        {
          int c;
          unsigned char *str = SDATA (string);

          if (multibyte)
            {
              int cbytes;
              c = string_char_and_length (str + i_byte, &cbytes);
              bytes = cbytes;
            }
          else
            c = str[i_byte], bytes = 1;
          chars = 1;
          thiswidth = char_width (c, dp);
        }

      if (0 < precision && precision - width < thiswidth)
        {
          *nchars = i - from;
          *nbytes = i_byte - from_byte;
          return width;
        }
      if (ckd_add (&width, width, thiswidth))
        string_overflow ();
      i += chars;
      i_byte += bytes;
    }

  if (precision > 0)
    {
      *nchars = i - from;
      *nbytes = i_byte - from_byte;
    }

  return width;
}

ptrdiff_t
multibyte_chars_in_text (const unsigned char *ptr, ptrdiff_t nbytes)
{
  const unsigned char *endp = ptr + nbytes;
  ptrdiff_t chars = 0;

  while (ptr < endp)
    {
      int len = multibyte_length (ptr, endp, true, true);

      if (len == 0)
        emacs_abort ();
      ptr += len;
      chars++;
    }

  return chars;
}

signed char const hexdigit[UCHAR_MAX + 1]
  = { ['0'] = 1 + 0,  ['1'] = 1 + 1,  ['2'] = 1 + 2,  ['3'] = 1 + 3,
      ['4'] = 1 + 4,  ['5'] = 1 + 5,  ['6'] = 1 + 6,  ['7'] = 1 + 7,
      ['8'] = 1 + 8,  ['9'] = 1 + 9,  ['A'] = 1 + 10, ['B'] = 1 + 11,
      ['C'] = 1 + 12, ['D'] = 1 + 13, ['E'] = 1 + 14, ['F'] = 1 + 15,
      ['a'] = 1 + 10, ['b'] = 1 + 11, ['c'] = 1 + 12, ['d'] = 1 + 13,
      ['e'] = 1 + 14, ['f'] = 1 + 15 };
void
syms_of_character (void)
{
  DEFSYM (Qcharacterp, "characterp");

  staticpro (&Vchar_unify_table);
  Vchar_unify_table = Qnil;
}
