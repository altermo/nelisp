#include "lisp.h"

#include "lua.h"

DEFUN ("message", Fmessage, Smessage, 1, MANY, 0,
       doc: /* Display a message at the bottom of the screen.
The message also goes into the `*Messages*' buffer, if `message-log-max'
is non-nil.  (In keyboard macros, that's all it does.)
Return the message.

In batch mode, the message is printed to the standard error stream,
followed by a newline.

The first argument is a format control string, and the rest are data
to be formatted under control of the string.  Percent sign (%), grave
accent (\\=`) and apostrophe (\\=') are special in the format; see
`format-message' for details.  To display STRING without special
treatment, use (message "%s" STRING).

If the first argument is nil or the empty string, the function clears
any existing message; this lets the minibuffer contents show.  See
also `current-message'.

usage: (message FORMAT-STRING &rest ARGS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  UNUSED (nargs);
  TODO_NELISP_LATER;
  LUA (5)
  {
    lua_getglobal (L, "print");
    lua_pushlstring (L, (char *) SDATA (args[0]), SBYTES (args[0]));
    lua_pcall (L, 1, 0, 0);
  }
  return args[0];
}

void
syms_of_editfns (void)
{
  defsubr (&Smessage);
}
