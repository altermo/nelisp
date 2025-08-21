#include "lisp.h"
#include "buffer.h"
#include "nvim.h"

static void
CHECK_MARKER (Lisp_Object x)
{
  CHECK_TYPE (MARKERP (x), Qmarkerp, x);
}

void
unchain_marker (register struct Lisp_Marker *marker)
{
  register struct buffer *b = marker->buffer;

  if (b)
    {
      register struct Lisp_Marker *tail, **prev;

      eassert (BUFFER_LIVE_P (b));

      marker->buffer = NULL;
      marker->extmark_id = 0;
    }
}

ptrdiff_t
marker_byte_position (Lisp_Object marker)
{
  register struct Lisp_Marker *m = XMARKER (marker);
  register struct buffer *buf = m->buffer;

  if (!buf)
    error ("Marker does not point anywhere");

  ptrdiff_t bytepos = nvim_mark_bytepos (m);

  eassert (BUF_BEG_BYTE (buf) <= bytepos && bytepos <= BUF_Z_BYTE (buf));

  return bytepos;
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
