#include "lisp.h"
#include "nvim.h"

DEFUN ("frame-list", Fframe_list, Sframe_list,
       0, 0, 0,
       doc: /* Return a list of all live frames.
The return value does not include any tooltip frame.  */)
(void)
{
  Lisp_Object list = Qnil;
  return nvim_frames_list ();
}

void
syms_of_frame (void)
{
  defsubr (&Sframe_list);
}
