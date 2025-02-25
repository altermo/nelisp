#ifndef EMACS_CHARACTER_C
#define EMACS_CHARACTER_C

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
    TODO
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
#endif
