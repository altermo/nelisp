#include "lisp.h"

void
syms_of_xdisp (void)
{
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
