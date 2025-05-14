#ifndef EMACS_CHARACTER_H
#define EMACS_CHARACTER_H

#include "lisp.h"

enum
{
  MAX_CHAR = 0x3FFFFF
};
enum
{
  MAX_UNICODE_CHAR = 0x10FFFF
};
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
enum
{
  MAX_MULTIBYTE_LENGTH = 5
};
enum
{
  MAX_1_BYTE_CHAR = 0x7F
};
enum
{
  MAX_2_BYTE_CHAR = 0x7FF
};
enum
{
  MAX_3_BYTE_CHAR = 0xFFFF
};
enum
{
  MAX_4_BYTE_CHAR = 0x1FFFFF
};
enum
{
  MAX_5_BYTE_CHAR = 0x3FFF7F
};

enum
{
  MIN_MULTIBYTE_LEADING_CODE = 0xC0
};
enum
{
  MAX_MULTIBYTE_LEADING_CODE = 0xF8
};

extern int char_string (unsigned, unsigned char *);

INLINE bool
SINGLE_BYTE_CHAR_P (intmax_t c)
{
  return 0 <= c && c < 0x100;
}
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
UNIBYTE_TO_CHAR (int byte)
{
  return ASCII_CHAR_P (byte) ? byte : BYTE8_TO_CHAR (byte);
}
INLINE bool
CHAR_VALID_P (intmax_t c)
{
  return 0 <= c && c <= MAX_CHAR;
}
INLINE bool
CHARACTERP (Lisp_Object x)
{
  return FIXNUMP (x) && CHAR_VALID_P (XFIXNUM (x));
}
INLINE void
CHECK_CHARACTER (Lisp_Object x)
{
  CHECK_TYPE (CHARACTERP (x), Qcharacterp, x);
}
INLINE void
CHECK_CHARACTER_CAR (Lisp_Object x)
{
  CHECK_CHARACTER (XCAR (x));
}
INLINE void
CHECK_CHARACTER_CDR (Lisp_Object x)
{
  CHECK_CHARACTER (XCDR (x));
}

INLINE int
CHAR_LEADING_CODE (int c)
{
  return (c <= MAX_1_BYTE_CHAR   ? c
          : c <= MAX_2_BYTE_CHAR ? 0xC0 | (c >> 6)
          : c <= MAX_3_BYTE_CHAR ? 0xE0 | (c >> 12)
          : c <= MAX_4_BYTE_CHAR ? 0xF0 | (c >> 18)
          : c <= MAX_5_BYTE_CHAR ? 0xF8
                                 : 0xC0 | ((c >> 6) & 0x01));
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
  int len = char_string (c, p);
  eassume (0 < len && len <= MAX_MULTIBYTE_LENGTH);
  return len;
}
INLINE int
BYTE8_STRING (int b, unsigned char *p)
{
  p[0] = 0xC0 | ((b >> 6) & 0x01);
  p[1] = 0x80 | (b & 0x3F);
  return 2;
}
INLINE bool
CHAR_HEAD_P (int byte)
{
  return (byte & 0xC0) != 0x80;
}
INLINE int
BYTES_BY_CHAR_HEAD (int byte)
{
  return (!(byte & 0x80)   ? 1
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
CHAR_TO_BYTE_SAFE (int c)
{
  return ASCII_CHAR_P (c) ? c : CHAR_BYTE8_P (c) ? c - 0x3FFF00 : -1;
}
INLINE bool
TRAILING_CODE_P (int byte)
{
  return (byte & 0xC0) == 0x80;
}
INLINE int
string_char_and_length (unsigned char const *p, int *length)
{
  int c = p[0];
  if (!(c & 0x80))
    {
      *length = 1;
      return c;
    }
  eassume (0xC0 <= c);

  int d = (c << 6) + p[1] - ((0xC0 << 6) + 0x80);
  if (!(c & 0x20))
    {
      *length = 2;
      return d + (c < 0xC2 ? 0x3FFF80 : 0);
    }

  d = (d << 6) + p[2] - ((0x20 << 12) + 0x80);
  if (!(c & 0x10))
    {
      *length = 3;
      eassume (MAX_2_BYTE_CHAR < d && d <= MAX_3_BYTE_CHAR);
      return d;
    }

  d = (d << 6) + p[3] - ((0x10 << 18) + 0x80);
  if (!(c & 0x08))
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
raw_prev_char_len (unsigned char const *p)
{
  for (int len = 1;; len++)
    if (CHAR_HEAD_P (p[-len]))
      return len;
}

INLINE int
string_char_advance (unsigned char const **pp)
{
  unsigned char const *p = *pp;
  int len, c = string_char_and_length (p, &len);
  *pp = p + len;
  return c;
}
INLINE int
fetch_string_char_advance (Lisp_Object string, ptrdiff_t *charidx,
                           ptrdiff_t *byteidx)
{
  int output;
  ptrdiff_t b = *byteidx;
  unsigned char *chp = SDATA (string) + b;
  if (STRING_MULTIBYTE (string))
    {
      int chlen;
      output = string_char_and_length (chp, &chlen);
      b += chlen;
    }
  else
    {
      output = *chp;
      b++;
    }
  (*charidx)++;
  *byteidx = b;
  return output;
}

INLINE int
fetch_string_char_advance_no_check (Lisp_Object string, ptrdiff_t *charidx,
                                    ptrdiff_t *byteidx)
{
  ptrdiff_t b = *byteidx;
  unsigned char *chp = SDATA (string) + b;
  int chlen, output = string_char_and_length (chp, &chlen);
  (*charidx)++;
  *byteidx = b + chlen;
  return output;
}

INLINE int
STRING_CHAR (unsigned char const *p)
{
  int len;
  return string_char_and_length (p, &len);
}

ptrdiff_t str_as_unibyte (unsigned char *str, ptrdiff_t bytes);
void parse_str_as_multibyte (const unsigned char *str, ptrdiff_t len,
                             ptrdiff_t *nchars, ptrdiff_t *nbytes);
ptrdiff_t count_size_as_multibyte (const unsigned char *str, ptrdiff_t len);
ptrdiff_t str_to_multibyte (unsigned char *dst, const unsigned char *src,
                            ptrdiff_t nchars);

extern Lisp_Object Vchar_unify_table;

INLINE int
char_table_translate (Lisp_Object obj, int ch)
{
  eassert (CHAR_VALID_P (ch));
  eassert (CHAR_TABLE_P (obj));
  obj = CHAR_TABLE_REF (obj, ch);
  return CHARACTERP (obj) ? XFIXNUM (obj) : ch;
}

extern ptrdiff_t lisp_string_width (Lisp_Object string, ptrdiff_t from,
                                    ptrdiff_t to, ptrdiff_t precision,
                                    ptrdiff_t *nchars, ptrdiff_t *nbytes,
                                    bool auto_comp);

extern signed char const hexdigit[];

INLINE int
char_hexdigit (int c)
{
  return 0 <= c && c <= UCHAR_MAX ? hexdigit[c] - 1 : -1;
}

#endif
