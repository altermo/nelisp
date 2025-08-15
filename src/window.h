#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include "lisp.h"
#include "nvim.h"

struct window
{
  union vectorlike_header header;

  Lisp_Object _last_obj;

  long winid;
};

#define selected_window nvim_get_current_window ()

INLINE bool
WINDOWP (Lisp_Object a)
{
  return PSEUDOVECTORP (a, PVEC_WINDOW);
}

INLINE void
CHECK_WINDOW (Lisp_Object x)
{
  CHECK_TYPE (WINDOWP (x), Qwindowp, x);
}

INLINE struct window *
XWINDOW (Lisp_Object a)
{
  eassert (WINDOWP (a));
  return XUNTAG (a, Lisp_Vectorlike, struct window);
}

extern struct window *allocate_window (void);

#endif
