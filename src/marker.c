#include "lisp.h"
#include "buffer.h"

static void
CHECK_MARKER (Lisp_Object x)
{
  CHECK_TYPE (MARKERP (x), Qmarkerp, x);
}

DEFUN ("marker-buffer", Fmarker_buffer, Smarker_buffer, 1, 1, 0,
       doc: /* Return the buffer that MARKER points into, or nil if none.
Returns nil if MARKER points into a dead buffer.  */)
(register Lisp_Object marker)
{
  register Lisp_Object buf;
  CHECK_MARKER (marker);
  if (XMARKER (marker)->buffer)
    {
      XSETBUFFER (buf, XMARKER (marker)->buffer);

      eassert (BUFFER_LIVE_P (XBUFFER (buf)));
      return buf;
    }
  return Qnil;
}

void
syms_of_marker (void)
{
  defsubr (&Smarker_buffer);
}
