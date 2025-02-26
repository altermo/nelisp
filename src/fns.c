#ifndef EMACS_FNS_C
#define EMACS_FNS_C

#include "lisp.h"
#include "character.c"

ptrdiff_t
list_length (Lisp_Object list)
{
    intptr_t i = 0;
    FOR_EACH_TAIL (list) i++;
#if TODO_NELISP_LATER_AND
    CHECK_LIST_END (list, list);
#endif
    return i;
}

INLINE EMACS_UINT
sxhash_combine (EMACS_UINT x, EMACS_UINT y)
{
    return (x << 4) + (x >> (EMACS_INT_WIDTH - 4)) + y;
}

EMACS_UINT
hash_string (char const *ptr, ptrdiff_t len)
{
    char const *p   = ptr;
    char const *end = ptr + len;
    EMACS_UINT hash = len;
    ptrdiff_t step = max ((long)sizeof hash, ((end - p) >> 3));

    if (p + sizeof hash <= end)
    {
        do
        {
            EMACS_UINT c;
            memcpy (&c, p, sizeof hash);
            p += step;
            hash = sxhash_combine (hash, c);
        }
        while (p + sizeof hash <= end);
        EMACS_UINT c;
        memcpy (&c, end - sizeof c, sizeof c);
        hash = sxhash_combine (hash, c);
    }
    else
{
        eassume (p <= end && end - p < (long)sizeof (EMACS_UINT));
        EMACS_UINT tail = 0;
#if EMACS_INT_MAX > INT32_MAX
        if (end - p >= 4)
        {
            uint32_t c;
            memcpy (&c, p, sizeof c);
            tail = (tail << (8 * sizeof c)) + c;
            p += sizeof c;
        }
#endif
        if (end - p >= 2)
        {
            uint16_t c;
            memcpy (&c, p, sizeof c);
            tail = (tail << (8 * sizeof c)) + c;
            p += sizeof c;
        }
        if (p < end)
            tail = (tail << 8) + (unsigned char)*p;
        hash = sxhash_combine (hash, tail);
    }

    return hash;
}
static Lisp_Object string_char_byte_cache_string;
static ptrdiff_t string_char_byte_cache_charpos;
static ptrdiff_t string_char_byte_cache_bytepos;
ptrdiff_t
string_char_to_byte (Lisp_Object string, ptrdiff_t char_index)
{
  ptrdiff_t i_byte;
  ptrdiff_t best_below, best_below_byte;
  ptrdiff_t best_above, best_above_byte;

  best_below = best_below_byte = 0;
  best_above = SCHARS (string);
  best_above_byte = SBYTES (string);
  if (best_above == best_above_byte)
    return char_index;

  if (BASE_EQ (string, string_char_byte_cache_string))
    {
      if (string_char_byte_cache_charpos < char_index)
	{
	  best_below = string_char_byte_cache_charpos;
	  best_below_byte = string_char_byte_cache_bytepos;
	}
      else
	{
	  best_above = string_char_byte_cache_charpos;
	  best_above_byte = string_char_byte_cache_bytepos;
	}
    }

  if (char_index - best_below < best_above - char_index)
    {
      unsigned char *p = SDATA (string) + best_below_byte;

      while (best_below < char_index)
	{
	  p += BYTES_BY_CHAR_HEAD (*p);
	  best_below++;
	}
      i_byte = p - SDATA (string);
    }
  else
    {
      unsigned char *p = SDATA (string) + best_above_byte;

      while (best_above > char_index)
	{
	  p--;
	  while (!CHAR_HEAD_P (*p)) p--;
	  best_above--;
	}
      i_byte = p - SDATA (string);
    }

  string_char_byte_cache_bytepos = i_byte;
  string_char_byte_cache_charpos = char_index;
  string_char_byte_cache_string = string;

  return i_byte;
}
#endif
