#ifndef EMACS_CHARACTER_H
#define EMACS_CHARACTER_H

#include "lisp.h"

enum
{
    NO_BREAK_SPACE = 0x00A0,
    SOFT_HYPHEN = 0x00AD,
    ZERO_WIDTH_NON_JOINER = 0x200C,
    ZERO_WIDTH_JOINER = 0x200D,
    HYPHEN = 0x2010,
    NON_BREAKING_HYPHEN = 0x2011,
    LEFT_SINGLE_QUOTATION_MARK = 0x2018,
    RIGHT_SINGLE_QUOTATION_MARK = 0x2019,
    PARAGRAPH_SEPARATOR = 0x2029,
    LEFT_POINTING_ANGLE_BRACKET = 0x2329,
    RIGHT_POINTING_ANGLE_BRACKET = 0x232A,
    LEFT_ANGLE_BRACKET = 0x3008,
    RIGHT_ANGLE_BRACKET = 0x3009,
    OBJECT_REPLACEMENT_CHARACTER = 0xFFFC,
    TAG_SPACE = 0xE0020,
    CANCEL_TAG = 0xE007F,
};
enum { MAX_MULTIBYTE_LENGTH = 5 };
enum { MAX_1_BYTE_CHAR = 0x7F };
enum { MAX_2_BYTE_CHAR = 0x7FF };
enum { MAX_3_BYTE_CHAR = 0xFFFF };
enum { MAX_4_BYTE_CHAR = 0x1FFFFF };
enum { MAX_5_BYTE_CHAR = 0x3FFF7F };

INLINE bool
CHAR_BYTE8_P (int c)
{
  return MAX_5_BYTE_CHAR < c;
}
INLINE int
BYTE8_TO_CHAR (int byte)
{
  return byte + 0x3FFF00;
}

INLINE int
CHAR_STRING (int c, unsigned char *p)
{
    eassume (0 <= c);
    if (c <= MAX_1_BYTE_CHAR)
    {
        p[0] = c;
        return 1;
    }
    if (c <= MAX_2_BYTE_CHAR)
    {
        p[0] = 0xC0 | (c >> 6);
        p[1] = 0x80 | (c & 0x3F);
        return 2;
    }
    if (c <= MAX_3_BYTE_CHAR)
    {
        p[0] = 0xE0 | (c >> 12);
        p[1] = 0x80 | ((c >> 6) & 0x3F);
        p[2] = 0x80 | (c & 0x3F);
        return 3;
    }
    TODO;
    // int len = char_string (c, p);
    // eassume (0 < len && len <= MAX_MULTIBYTE_LENGTH);
    // return len;
}
INLINE bool
CHAR_HEAD_P (int byte)
{
  return (byte & 0xC0) != 0x80;
}
INLINE int
BYTES_BY_CHAR_HEAD (int byte)
{
  return (!(byte & 0x80) ? 1
	  : !(byte & 0x20) ? 2
	  : !(byte & 0x10) ? 3
	  : !(byte & 0x08) ? 4
	  : 5);
}
INLINE bool
CHAR_BYTE8_HEAD_P (int byte)
{
  return byte == 0xC0 || byte == 0xC1;
}
INLINE int
CHAR_TO_BYTE8 (int c)
{
  return CHAR_BYTE8_P (c) ? c - 0x3FFF00 : c & 0xFF;
}
INLINE int
string_char_and_length (unsigned char const *p, int *length)
{
  int c = p[0];
  if (! (c & 0x80))
    {
      *length = 1;
      return c;
    }
  eassume (0xC0 <= c);

  int d = (c << 6) + p[1] - ((0xC0 << 6) + 0x80);
  if (! (c & 0x20))
    {
      *length = 2;
      return d + (c < 0xC2 ? 0x3FFF80 : 0);
    }

  d = (d << 6) + p[2] - ((0x20 << 12) + 0x80);
  if (! (c & 0x10))
    {
      *length = 3;
      eassume (MAX_2_BYTE_CHAR < d && d <= MAX_3_BYTE_CHAR);
      return d;
    }

  d = (d << 6) + p[3] - ((0x10 << 18) + 0x80);
  if (! (c & 0x08))
    {
      *length = 4;
      eassume (MAX_3_BYTE_CHAR < d && d <= MAX_4_BYTE_CHAR);
      return d;
    }

  d = (d << 6) + p[4] - ((0x08 << 24) + 0x80);
  *length = 5;
  eassume (MAX_4_BYTE_CHAR < d && d <= MAX_5_BYTE_CHAR);
  return d;
}
INLINE int
string_char_advance (unsigned char const **pp)
{
  unsigned char const *p = *pp;
  int len, c = string_char_and_length (p, &len);
  *pp = p + len;
  return c;
}

ptrdiff_t str_as_unibyte (unsigned char *str, ptrdiff_t bytes);
void
parse_str_as_multibyte (const unsigned char *str, ptrdiff_t len,
			ptrdiff_t *nchars, ptrdiff_t *nbytes);

#endif
