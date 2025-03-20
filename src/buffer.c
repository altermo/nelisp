#include "buffer.h"
#include "lisp.h"
#include "nvim.h"

DEFUN ("get-buffer", Fget_buffer, Sget_buffer, 1, 1, 0,
       doc: /* Return the buffer named BUFFER-OR-NAME.
BUFFER-OR-NAME must be either a string or a buffer.  If BUFFER-OR-NAME
is a string and there is no buffer with that name, return nil.  If
BUFFER-OR-NAME is a buffer, return it as given.  */)
(register Lisp_Object buffer_or_name)
{
  if (BUFFERP (buffer_or_name))
    return buffer_or_name;
  CHECK_STRING (buffer_or_name);

  return nvim_name_to_bufobj (buffer_or_name);
}

DEFUN ("get-buffer-create", Fget_buffer_create, Sget_buffer_create, 1, 2, 0,
       doc: /* Return the buffer specified by BUFFER-OR-NAME, creating a new one if needed.
If BUFFER-OR-NAME is a string and a live buffer with that name exists,
return that buffer.  If no such buffer exists, create a new buffer with
that name and return it.

If BUFFER-OR-NAME starts with a space, the new buffer does not keep undo
information.  If optional argument INHIBIT-BUFFER-HOOKS is non-nil, the
new buffer does not run the hooks `kill-buffer-hook',
`kill-buffer-query-functions', and `buffer-list-update-hook'.  This
avoids slowing down internal or temporary buffers that are never
presented to users or passed on to other applications.

If BUFFER-OR-NAME is a buffer instead of a string, return it as given,
even if it is dead.  The return value is never nil.  */)
(register Lisp_Object buffer_or_name, Lisp_Object inhibit_buffer_hooks)
{
  register Lisp_Object buffer;

  buffer = Fget_buffer (buffer_or_name);
  if (!NILP (buffer))
    return buffer;

  if (SCHARS (buffer_or_name) == 0)
    TODO; // error ("Empty string for buffer name is not allowed");
  buffer = nvim_create_buf (buffer_or_name, inhibit_buffer_hooks);
  return buffer;
}

DEFUN ("buffer-name", Fbuffer_name, Sbuffer_name, 0, 1, 0,
       doc: /* Return the name of BUFFER, as a string.
BUFFER defaults to the current buffer.
Return nil if BUFFER has been killed.  */)
(register Lisp_Object buffer) { return BVAR (decode_buffer (buffer), name); }

void
init_buffer (void)
{
  AUTO_STRING (scratch, "*scratch*");
  Fget_buffer_create (scratch, Qnil);
}

void
syms_of_buffer (void)
{
  defsubr (&Sget_buffer);
  defsubr (&Sget_buffer_create);
  defsubr (&Sbuffer_name);
}
