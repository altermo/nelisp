#include "lisp.h"
#include "buffer.h"
#include "lua.h"

Lisp_Object echo_area_buffer[2];

Lisp_Object
current_message (void)
{
  TODO_NELISP_LATER;
  Lisp_Object msg;

  if (!BUFFERP (echo_area_buffer[0]))
    msg = Qnil;
  else
    {
      TODO;
    }

  return msg;
}

void
message3 (Lisp_Object m)
{
  TODO_NELISP_LATER;
  if (STRINGP (m))
    {
      LUA (5)
      {
        lua_getglobal (L, "print");
        lua_pushlstring (L, (char *) SDATA (m), SBYTES (m));
        lua_call (L, 1, 0);
      }
    }
  else
    TODO;
}

void
message1 (const char *m)
{
  message3 (m ? build_unibyte_string (m) : Qnil);
}

void
syms_of_xdisp (void)
{
  DEFSYM (Qeval, "eval");

  DEFSYM (Qinhibit_redisplay, "inhibit-redisplay");

  echo_area_buffer[0] = echo_area_buffer[1] = Qnil;
  staticpro (&echo_area_buffer[0]);
  staticpro (&echo_area_buffer[1]);

  DEFVAR_INT ("max-redisplay-ticks", max_redisplay_ticks,
    doc: /* Maximum number of redisplay ticks before aborting redisplay of a window.

This enables aborting the display of a window if the amount of
low-level redisplay operations exceeds the value of this variable.
When display of a window is aborted due to this reason, the buffer
shown in that window will not have its windows redisplayed until the
buffer is modified or until you type \\[recenter-top-bottom] with one
of its windows selected.  You can also decide to kill the buffer and
visit it in some other way, like under `so-long-mode' or literally.

The default value is zero, which disables this feature.
The recommended non-zero value is between 100000 and 1000000,
depending on your patience and the speed of your system.  */);
  max_redisplay_ticks = 0;

  DEFSYM (Qempty_box, "empty-box");

  DEFSYM (Qglyphless_char_display, "glyphless-char-display");
  Fput (Qglyphless_char_display, Qchar_table_extra_slots, make_fixnum (1));

  DEFVAR_LISP ("glyphless-char-display", Vglyphless_char_display,
        doc: /* Char-table defining glyphless characters.
Each element, if non-nil, should be one of the following:
  an ASCII acronym string: display this string in a box
  `hex-code':   display the hexadecimal code of a character in a box
  `empty-box':  display as an empty box
  `thin-space': display as 1-pixel width space
  `zero-width': don't display
Any other value is interpreted as `empty-box'.
An element may also be a cons cell (GRAPHICAL . TEXT), which specifies the
display method for graphical terminals and text terminals respectively.
GRAPHICAL and TEXT should each have one of the values listed above.

The char-table has one extra slot to control the display of characters
for which no font is found on graphical terminals, and characters that
cannot be displayed by text-mode terminals.  Its value should be an
ASCII acronym string, `hex-code', `empty-box', or `thin-space'.  It
could also be a cons cell of any two of these, to specify separate
values for graphical and text terminals.  The default is `empty-box'.

With the obvious exception of `zero-width', all the other representations
are displayed using the face `glyphless-char'.

If a character has a non-nil entry in an active display table, the
display table takes effect; in this case, Emacs does not consult
`glyphless-char-display' at all.  */);
  Vglyphless_char_display = Fmake_char_table (Qglyphless_char_display, Qnil);
  Fset_char_table_extra_slot (Vglyphless_char_display, make_fixnum (0),
                              Qempty_box);
}
