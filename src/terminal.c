#include "lisp.h"
#include "frame.h"
#include "termhooks.h"

DEFUN ("frame-terminal", Fframe_terminal, Sframe_terminal, 0, 1, 0,
       doc: /* Return the terminal that FRAME is displayed on.
If FRAME is nil, use the selected frame.

The terminal device is represented by its integer identifier.  */)
(Lisp_Object frame)
{
  struct terminal *t = FRAME_TERMINAL (decode_live_frame (frame));

  if (!t)
    return Qnil;
  else
    {
      Lisp_Object terminal;
      XSETTERMINAL (terminal, t);
      return terminal;
    }
}

void
syms_of_terminal (void)
{
  defsubr (&Sframe_terminal);
}
