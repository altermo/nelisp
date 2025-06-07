#include "lisp.h"

void
syms_of_xfaces (void)
{
  DEFSYM (Qface, "face");

  DEFSYM (Qtab_line, "tab-line");
  DEFSYM (Qheader_line, "header-line");
}
