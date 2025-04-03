#ifndef EMACS_BUFFER_H
#define EMACS_BUFFER_H

#include "lisp.h"
#include "nvim.h"

#define current_buffer (nvim_current_buffer ())

INLINE bool
BUFFERP (Lisp_Object a)
{
  return PSEUDOVECTORP (a, PVEC_BUFFER);
}

INLINE void
CHECK_BUFFER (Lisp_Object x)
{
  CHECK_TYPE (BUFFERP (x), Qbufferp, x);
}

INLINE struct buffer *
XBUFFER (Lisp_Object a)
{
  eassert (BUFFERP (a));
  return XUNTAG (a, Lisp_Vectorlike, struct buffer);
}

INLINE struct buffer *
decode_buffer (Lisp_Object b)
{
  return NILP (b) ? current_buffer : (CHECK_BUFFER (b), XBUFFER (b));
}

#define BVAR(buf, field) nvim_bvar (buf, NVIM_BUFFER_VAR__##field)

INLINE bool
BUFFER_LIVE_P (struct buffer *b)
{
  return !NILP (BVAR (b, name));
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
