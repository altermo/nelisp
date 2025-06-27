#ifndef EMACS_FRAME_H
#define EMACS_FRAME_H

#include "lisp.h"
#include "nvim.h"

struct frame
{
  union vectorlike_header header;

  Lisp_Object face_hash_table;

  Lisp_Object _last_obj;

  long tabpageid;
};

#define XFRAME(p) \
  (eassert (FRAMEP (p)), XUNTAG (p, Lisp_Vectorlike, struct frame))
#define XSETFRAME(a, b) XSETPSEUDOVECTOR (a, b, PVEC_FRAME)

#define FRAME_LIVE_P(f) nvim_frame_is_valid (f)

#define CHECK_LIVE_FRAME(x) \
  CHECK_TYPE (FRAMEP (x) && FRAME_LIVE_P (XFRAME (x)), Qframe_live_p, x)

#endif
