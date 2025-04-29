#include "lisp.h"
#include "bignum.h"

// Taken from lib/timespec.h
enum
{
  TIMESPEC_HZ = 1000000000
};
// Taken from lib/gettime.c
#include <sys/time.h>
void
gettime (struct timespec *ts)
{
  struct timeval tv;
  gettimeofday (&tv, NULL);
  *ts = (struct timespec) { .tv_sec = tv.tv_sec, .tv_nsec = tv.tv_usec * 1000 };
}
struct timespec
current_timespec (void)
{
  struct timespec ts;
  gettime (&ts);
  return ts;
}
//

#ifndef FASTER_TIMEFNS
# define FASTER_TIMEFNS 1
#endif

#ifndef CURRENT_TIME_LIST
enum
{
  CURRENT_TIME_LIST = true
};
#endif

#if FIXNUM_OVERFLOW_P(1000000000)
static Lisp_Object timespec_hz;
#else
# define timespec_hz make_fixnum (TIMESPEC_HZ)
#endif

enum
{
  LO_TIME_BITS = 16
};

static Lisp_Object
hi_time (time_t t)
{
  return INT_TO_INTEGER (t >> LO_TIME_BITS);
}

static Lisp_Object
lo_time (time_t t)
{
  return make_fixnum (t & ((1 << LO_TIME_BITS) - 1));
}

static void
mpz_set_time (mpz_t rop, time_t t)
{
  if (EXPR_SIGNED (t))
    mpz_set_intmax (rop, t);
  else
    mpz_set_uintmax (rop, t);
}

static void
timespec_mpz (struct timespec t)
{
  /* mpz[0] = sec * TIMESPEC_HZ + nsec.  */
  mpz_set_ui (mpz[0], t.tv_nsec);
  mpz_set_time (mpz[1], t.tv_sec);
  mpz_addmul_ui (mpz[0], mpz[1], TIMESPEC_HZ);
}

static Lisp_Object
timespec_ticks (struct timespec t)
{
  intmax_t accum;
  if (FASTER_TIMEFNS && !ckd_mul (&accum, t.tv_sec, TIMESPEC_HZ)
      && !ckd_add (&accum, accum, t.tv_nsec))
    return make_int (accum);

  timespec_mpz (t);
  return make_integer_mpz ();
}

Lisp_Object
timespec_to_lisp (struct timespec t)
{
  return Fcons (timespec_ticks (t), timespec_hz);
}

Lisp_Object
make_lisp_time (struct timespec t)
{
  if (current_time_list)
    {
      time_t s = t.tv_sec;
      int ns = t.tv_nsec;
      return list4 (hi_time (s), lo_time (s), make_fixnum (ns / 1000),
                    make_fixnum (ns % 1000 * 1000));
    }
  else
    return timespec_to_lisp (t);
}

DEFUN ("current-time", Fcurrent_time, Scurrent_time, 0, 0, 0,
       doc: /* Return the current time, as the number of seconds since 1970-01-01 00:00:00.
If the variable `current-time-list' is nil, the time is returned as a
pair of integers (TICKS . HZ), where TICKS counts clock ticks and HZ
is the clock ticks per second.  Otherwise, the time is returned as a
list of integers (HIGH LOW USEC PSEC) where HIGH has the most
significant bits of the seconds, LOW has the least significant 16
bits, and USEC and PSEC are the microsecond and picosecond counts.

You can use `time-convert' to get a particular timestamp form
regardless of the value of `current-time-list'.  */)
(void) { return make_lisp_time (current_timespec ()); }

void
syms_of_timefns (void)
{
  DEFVAR_BOOL ("current-time-list", current_time_list,
               doc: /* Whether `current-time' should return list or (TICKS . HZ) form.

This boolean variable is a transition aid.  If t, `current-time' and
related functions return timestamps in list form, typically
\(HIGH LOW USEC PSEC); otherwise, they use (TICKS . HZ) form.
Currently this variable defaults to t, for behavior compatible with
previous Emacs versions.  Developers are encouraged to test
timestamp-related code with this variable set to nil, as it will
default to nil in a future Emacs version, and will be removed in some
version after that.  */);
  current_time_list = CURRENT_TIME_LIST;

  defsubr (&Scurrent_time);
}
