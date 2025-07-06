#include <math.h>

#include "lisp.h"
#include "bignum.h"

DEFUN ("logb", Flogb, Slogb, 1, 1, 0,
       doc: /* Returns largest integer <= the base 2 log of the magnitude of ARG.
This is the same as the exponent of a float.  */)
(Lisp_Object arg)
{
  EMACS_INT value;
  CHECK_NUMBER (arg);

  if (FLOATP (arg))
    {
      double f = XFLOAT_DATA (arg);
      if (f == 0)
        return make_float (-HUGE_VAL);
      if (!isfinite (f))
        return f < 0 ? make_float (-f) : arg;
      int ivalue;
      frexp (f, &ivalue);
      value = ivalue - 1;
    }
  else if (!FIXNUMP (arg))
    value = mpz_sizeinbase (*xbignum_val (arg), 2) - 1;
  else
    {
      EMACS_INT i = XFIXNUM (arg);
      if (i == 0)
        return make_float (-HUGE_VAL);
      value = elogb (eabs (i));
    }

  return make_fixnum (value);
}

void
syms_of_floatfns (void)
{
  defsubr (&Slogb);
}
