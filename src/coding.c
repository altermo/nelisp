#include "lisp.h"

char emacs_mule_bytes[256];

void
syms_of_coding (void)
{
  DEFSYM (Qfilenamep, "filenamep");

  DEFSYM (QCascii_compatible_p, ":ascii-compatible-p");
}
