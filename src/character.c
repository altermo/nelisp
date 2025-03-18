#include "character.h"

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
    TODO; // error ("Invalid character: %x", c);

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
