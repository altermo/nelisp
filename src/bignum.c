#include "bignum.h"
#include "lisp.h"

mpz_t mpz[5];

enum
{
  GMP_NLIMBS_MAX = min (INT_MAX, ULONG_MAX / GMP_NUMB_BITS)
};

enum
{
  NLIMBS_LIMIT = min (min (GMP_NLIMBS_MAX,

                           min (PTRDIFF_MAX, SIZE_MAX) / sizeof (mp_limb_t)),

                      MOST_POSITIVE_FIXNUM / GMP_NUMB_BITS)
};

static int
emacs_mpz_size (mpz_t const op)
{
  mp_size_t size = mpz_size (op);
  eassume (0 <= size && size <= INT_MAX);
  return size;
}

void
emacs_mpz_mul_2exp (mpz_t rop, mpz_t const op1, EMACS_INT op2)
{
  enum
  {
    mul_2exp_extra_limbs = 1
  };
  enum
  {
    lim = min (NLIMBS_LIMIT, GMP_NLIMBS_MAX - mul_2exp_extra_limbs)
  };

  EMACS_INT op2limbs = op2 / GMP_NUMB_BITS;
  if (lim - emacs_mpz_size (op1) < op2limbs)
    overflow_error ();
  mpz_mul_2exp (rop, op1, op2);
}

static void *
xrealloc_for_gmp (void *ptr, size_t ignore, size_t size)
{
  UNUSED (ignore);
  return xrealloc (ptr, size);
}

static void
xfree_for_gmp (void *ptr, size_t ignore)
{
  UNUSED (ignore);
  xfree (ptr);
}

void
init_bignum (void)
{
  eassert (mp_bits_per_limb == GMP_NUMB_BITS);
  integer_width = 1 << 16;

  mp_set_memory_functions (xmalloc, xrealloc_for_gmp, xfree_for_gmp);

  for (unsigned long i = 0; i < ARRAYELTS (mpz); i++)
    mpz_init (mpz[i]);
}

static Lisp_Object
make_bignum_bits (size_t bits)
{
  if (integer_width < (long) bits
      && 2 * max (INTMAX_WIDTH, UINTMAX_WIDTH) < bits)
    overflow_error ();

  struct Lisp_Bignum *b
    = ALLOCATE_PLAIN_PSEUDOVECTOR (struct Lisp_Bignum, PVEC_BIGNUM);
  mpz_init (b->value);
  mpz_swap (b->value, mpz[0]);
  return make_lisp_ptr (b, Lisp_Vectorlike);
}

Lisp_Object
make_integer_mpz (void)
{
  size_t bits = mpz_sizeinbase (mpz[0], 2);

  if (bits <= FIXNUM_BITS)
    {
      EMACS_INT v = 0;
      int i = 0, shift = 0;

      do
        {
          EMACS_INT limb = mpz_getlimbn (mpz[0], i++);
          v += limb << shift;
          shift += GMP_NUMB_BITS;
        }
      while (shift < (long) bits);

      if (mpz_sgn (mpz[0]) < 0)
        v = -v;

      if (!FIXNUM_OVERFLOW_P (v))
        return make_fixnum (v);
    }

  return make_bignum_bits (bits);
}

intmax_t
check_integer_range (Lisp_Object x, intmax_t lo, intmax_t hi)
{
  CHECK_INTEGER (x);
  intmax_t i;
  if (!(integer_to_intmax (x, &i) && lo <= i && i <= hi))
    args_out_of_range_3 (x, make_int (lo), make_int (hi));
  return i;
}
