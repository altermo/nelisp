#include "lisp.h"
#include "lua.h"

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
}
