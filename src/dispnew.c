#include "lisp.h"
#include "buffer.h"
#include "frame.h"

static Lisp_Object frame_and_buffer_state;

DEFUN ("frame-or-buffer-changed-p", Fframe_or_buffer_changed_p,
       Sframe_or_buffer_changed_p, 0, 1, 0,
       doc: /* Return non-nil if the frame and buffer state appears to have changed.
VARIABLE is a variable name whose value is either nil or a state vector
that will be updated to contain all frames and buffers,
aside from buffers whose names start with space,
along with the buffers' read-only and modified flags.  This allows a fast
check to see whether buffer menus might need to be recomputed.
If this function returns non-nil, it updates the internal vector to reflect
the current state.

If VARIABLE is nil, an internal variable is used.  Users should not
pass nil for VARIABLE.  */)
(Lisp_Object variable)
{
  Lisp_Object state, tail, frame, buf;
  ptrdiff_t n, idx;

  if (!NILP (variable))
    {
      CHECK_SYMBOL (variable);
      state = Fsymbol_value (variable);
      if (!VECTORP (state))
        goto changed;
    }
  else
    state = frame_and_buffer_state;

  idx = 0;
  FOR_EACH_FRAME (tail, frame)
    {
      if (idx == ASIZE (state))
        goto changed;
      if (!BASE_EQ (AREF (state, idx++), frame))
        goto changed;
      if (idx == ASIZE (state))
        goto changed;
      if (!EQ (AREF (state, idx++), frame_name (XFRAME (frame))))
        goto changed;
    }

  FOR_EACH_LIVE_BUFFER (tail, buf)
    {
      if (SREF (BVAR (XBUFFER (buf), name), 0) == ' ')
        continue;
      if (idx == ASIZE (state))
        goto changed;
      if (!BASE_EQ (AREF (state, idx++), buf))
        goto changed;
      if (idx == ASIZE (state))
        goto changed;
      if (!EQ (AREF (state, idx++), BVAR (XBUFFER (buf), read_only)))
        goto changed;
      if (idx == ASIZE (state))
        goto changed;
      if (!EQ (AREF (state, idx++), Fbuffer_modified_p (buf)))
        goto changed;
    }
  if (idx == ASIZE (state))
    goto changed;
  if (EQ (AREF (state, idx), Qlambda))
    return Qnil;

changed:
  n = 1;
  FOR_EACH_FRAME (tail, frame)
    n += 2;
  FOR_EACH_LIVE_BUFFER (tail, buf)
    n += 3;
  if (!VECTORP (state) || n > ASIZE (state) || n + 20 < ASIZE (state) / 2)
    {
      state = make_vector (n + 20, Qlambda);
      if (!NILP (variable))
        Fset (variable, state);
      else
        frame_and_buffer_state = state;
    }

  idx = 0;
  FOR_EACH_FRAME (tail, frame)
    {
      ASET (state, idx, frame);
      idx++;
      ASET (state, idx, frame_name (XFRAME (frame)));
      idx++;
    }
  FOR_EACH_LIVE_BUFFER (tail, buf)
    {
      if (SREF (BVAR (XBUFFER (buf), name), 0) == ' ')
        continue;
      ASET (state, idx, buf);
      idx++;
      ASET (state, idx, BVAR (XBUFFER (buf), read_only));
      idx++;
      ASET (state, idx, Fbuffer_modified_p (buf));
      idx++;
    }
  ASET (state, idx, Qlambda);
  idx++;
  while (idx < ASIZE (state))
    {
      ASET (state, idx, Qlambda);
      idx++;
    }
  eassert (idx <= ASIZE (state));
  return Qt;
}

DEFUN ("redraw-frame", Fredraw_frame, Sredraw_frame, 0, 1, 0,
       doc: /* Clear frame FRAME and output again what is supposed to appear on it.
If FRAME is omitted or nil, the selected frame is used.  */)
(Lisp_Object frame)
{
#if TODO_NELISP_LATER_AND
  redraw_frame (decode_live_frame (frame));
#endif
  return Qnil;
}

void
syms_of_display (void)
{
  DEFSYM (Qdisplay_table, "display-table");

  defsubr (&Sframe_or_buffer_changed_p);
  defsubr (&Sredraw_frame);

  frame_and_buffer_state = make_vector (20, Qlambda);
  staticpro (&frame_and_buffer_state);

  DEFVAR_LISP ("standard-display-table", Vstandard_display_table,
        doc: /* Display table to use for buffers that specify none.
It is also used for standard output and error streams.
See `buffer-display-table' for more information.  */);
  Vstandard_display_table = Qnil;

#if TODO_NELISP_LATER_AND
  DEFVAR_KBOARD ("window-system", Vwindow_system,
    doc: /* Name of window system through which the selected frame is displayed.
The value is a symbol:
 nil for a termcap frame (a character-only terminal),
 `x' for an Emacs frame that is really an X window,
 `w32' for an Emacs frame that is a window on MS-Windows display,
 `ns' for an Emacs frame on a GNUstep or Macintosh Cocoa display,
 `pc' for a direct-write MS-DOS frame.
 `pgtk' for an Emacs frame using pure GTK facilities.
 `haiku' for an Emacs frame running in Haiku.
 `android' for an Emacs frame running in Android.

Use of this variable as a boolean is deprecated.  Instead,
use `display-graphic-p' or any of the other `display-*-p'
predicates which report frame's specific UI-related capabilities.  */);
#else
  DEFVAR_LISP("window-system", Vwindow_system,
    doc: /* */);
  Vwindow_system = Qnil;
#endif
}
