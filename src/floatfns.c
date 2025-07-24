#include <math.h>

#include "lisp.h"
#include "bignum.h"

double
extract_float (Lisp_Object num)
{
  CHECK_NUMBER (num);
  return XFLOATINT (num);
}

DEFUN ("atan", Fatan, Satan, 1, 2, 0,
       doc: /* Return the inverse tangent of the arguments.
If only one argument Y is given, return the inverse tangent of Y.
If two arguments Y and X are given, return the inverse tangent of Y
divided by X, i.e. the angle in radians between the vector (X, Y)
and the x-axis.  */)
(Lisp_Object y, Lisp_Object x)
{
  double d = extract_float (y);

  if (NILP (x))
    d = atan (d);
  else
    {
      double d2 = extract_float (x);
      d = atan2 (d, d2);
    }
  return make_float (d);
}

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

DEFUN ("exp", Fexp, Sexp, 1, 1, 0,
       doc: /* Return the exponential base e of ARG.  */)
(Lisp_Object arg)
{
  double d = extract_float (arg);
  d = exp (d);
  return make_float (d);
}

void
syms_of_floatfns (void)
{
  defsubr (&Satan);
  defsubr (&Slogb);
  defsubr (&Sexp);
}
