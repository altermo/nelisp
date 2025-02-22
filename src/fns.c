#ifndef EMACS_FNS_C
#define EMACS_FNS_C

#include "lisp.h"

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
    ptrdiff_t step = sizeof hash + ((end - p) >> 3);
    while (p + sizeof hash <= end)
    {
        EMACS_UINT c;
        memcpy (&c, p, sizeof hash);
        p += step;
        hash = sxhash_combine (hash, c);
    }
    while (p < end)
    {
        unsigned char c = *p++;
        hash = sxhash_combine (hash, c);
    }

    return hash;
}
#endif
