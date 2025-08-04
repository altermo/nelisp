#include "lisp.h"
#include "frame.h"
#include "termhooks.h"

static void
tset_param_alist (struct terminal *t, Lisp_Object val)
{
  t->param_alist = val;
}

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

DEFUN ("terminal-parameter", Fterminal_parameter, Sterminal_parameter, 2, 2, 0,
       doc: /* Return TERMINAL's value for parameter PARAMETER.
TERMINAL can be a terminal object, a frame, or nil (meaning the
selected frame's terminal).  */)
(Lisp_Object terminal, Lisp_Object parameter)
{
  CHECK_SYMBOL (parameter);
  return Fcdr (Fassq (parameter, decode_live_terminal (terminal)->param_alist));
}

static Lisp_Object
store_terminal_param (struct terminal *t, Lisp_Object parameter,
                      Lisp_Object value)
{
  Lisp_Object old_alist_elt = Fassq (parameter, t->param_alist);
  if (NILP (old_alist_elt))
    {
      tset_param_alist (t, Fcons (Fcons (parameter, value), t->param_alist));
      return Qnil;
    }
  else
    {
      Lisp_Object result = Fcdr (old_alist_elt);
      Fsetcdr (old_alist_elt, value);
      return result;
    }
}

DEFUN ("set-terminal-parameter", Fset_terminal_parameter,
       Sset_terminal_parameter, 3, 3, 0,
       doc: /* Set TERMINAL's value for parameter PARAMETER to VALUE.
Return the previous value of PARAMETER.

TERMINAL can be a terminal object, a frame or nil (meaning the
selected frame's terminal).  */)
(Lisp_Object terminal, Lisp_Object parameter, Lisp_Object value)
{
  return store_terminal_param (decode_live_terminal (terminal), parameter,
                               value);
}

void
syms_of_terminal (void)
{
  DEFSYM (Qterminal_live_p, "terminal-live-p");

  defsubr (&Sframe_terminal);
  defsubr (&Sterminal_name);
  defsubr (&Sterminal_parameter);
  defsubr (&Sset_terminal_parameter);

  DEFSYM (Qdefault_terminal_coding_system, "default-terminal-coding-system");
}
