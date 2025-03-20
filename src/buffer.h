#ifndef EMACS_BUFFER_H
#define EMACS_BUFFER_H

#include "lisp.h"

INLINE bool
BUFFERP (Lisp_Object a)
{
  return PSEUDOVECTORP (a, PVEC_BUFFER);
}

struct buffer
{
  union vectorlike_header header;

  Lisp_Object _last_obj;

  long bufid;
};

enum
{
  BUFFER_LISP_SIZE = PSEUDOVECSIZE (struct buffer, _last_obj),
};

enum
{
  BUFFER_REST_SIZE = VECSIZE (struct buffer) - BUFFER_LISP_SIZE
};

INLINE void
BUFFER_PVEC_INIT (struct buffer *b)
{
  XSETPVECTYPESIZE (b, PVEC_BUFFER, BUFFER_LISP_SIZE, BUFFER_REST_SIZE);
}

#endif
