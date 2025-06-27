#ifndef EMACS_FRAME_H
#define EMACS_FRAME_H

#include "lisp.h"

struct frame
{
  union vectorlike_header header;

  Lisp_Object _last_obj;

  long tabpageid;
};

#define XSETFRAME(a, b) XSETPSEUDOVECTOR (a, b, PVEC_FRAME)

#endif
