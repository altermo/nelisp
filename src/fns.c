#ifndef EMACS_FNS_C
#define EMACS_FNS_C

#include "lisp.h"

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
#endif
