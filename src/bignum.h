#ifndef BIGNUM_H
#define BIGNUM_H

#include <gmp.h>
#include "lisp.h"

extern mpz_t mpz[5];

struct Lisp_Bignum
{
  union vectorlike_header header;
  mpz_t value;
} GCALIGNED_STRUCT;

extern Lisp_Object make_integer_mpz (void);
extern void init_bignum (void);

INLINE struct Lisp_Bignum *
XBIGNUM (Lisp_Object a)
{
  eassert (BIGNUMP (a));
  return XUNTAG (a, Lisp_Vectorlike, struct Lisp_Bignum);
}

INLINE void
mpz_set_intmax (mpz_t result, intmax_t v)
{
  if (LONG_MIN <= v && v <= LONG_MAX)
    mpz_set_si (result, v);
  else
    TODO; // mpz_set_intmax_slow (result, v);
}
INLINE void
mpz_set_uintmax (mpz_t result, uintmax_t v)
{
  if (v <= ULONG_MAX)
    mpz_set_ui (result, v);
  else
    TODO; // mpz_set_uintmax_slow (result, v);
}

INLINE mpz_t const *
bignum_val (struct Lisp_Bignum const *i)
{
  return &i->value;
}
INLINE mpz_t const *
xbignum_val (Lisp_Object i)
{
  return bignum_val (XBIGNUM (i));
}

#endif
