#include "lisp.h"
#include "frame.h"
#include "termhooks.h"

static struct terminal *
decode_terminal (Lisp_Object terminal)
{
  struct terminal *t;

  if (NILP (terminal))
    terminal = selected_frame;
  t = (TERMINALP (terminal) ? XTERMINAL (terminal)
       : FRAMEP (terminal)  ? FRAME_TERMINAL (XFRAME (terminal))
                            : NULL);
  return t && t->name ? t : NULL;
}

struct terminal *
decode_live_terminal (Lisp_Object terminal)
{
  struct terminal *t = decode_terminal (terminal);

  if (!t)
    wrong_type_argument (Qterminal_live_p, terminal);
  return t;
}

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

DEFUN ("terminal-name", Fterminal_name, Sterminal_name, 0, 1, 0,
       doc: /* Return the name of the terminal device TERMINAL.
It is not guaranteed that the returned value is unique among opened devices.

TERMINAL may be a terminal object, a frame, or nil (meaning the
selected frame's terminal). */)
(Lisp_Object terminal)
{
  struct terminal *t = decode_live_terminal (terminal);

  return t->name ? build_string (t->name) : Qnil;
}

void
syms_of_terminal (void)
{
  DEFSYM (Qterminal_live_p, "terminal-live-p");

  defsubr (&Sframe_terminal);
  defsubr (&Sterminal_name);
}
