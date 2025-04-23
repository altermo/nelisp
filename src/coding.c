#include "lisp.h"

void
syms_of_coding (void)
{
  DEFSYM (Qfilenamep, "filenamep");

  DEFSYM (QCascii_compatible_p, ":ascii-compatible-p");
}
