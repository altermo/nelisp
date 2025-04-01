#ifndef EMACS_PURESIZE_H
#define EMACS_PURESIZE_H

#include "lisp.h"

#define SYSTEM_PURESIZE_EXTRA 0
#define SITELOAD_PURESIZE_EXTRA 0
#define BASE_PURESIZE \
  (3400000 + SYSTEM_PURESIZE_EXTRA + SITELOAD_PURESIZE_EXTRA)
#define PURESIZE (BASE_PURESIZE * PURESIZE_RATIO * PURESIZE_CHECKING_RATIO)
#define PURESIZE_RATIO 10 / 6
#define PURESIZE_CHECKING_RATIO 1
extern EMACS_INT pure[];

#define puresize_h_PURE_P(ptr) \
  ((uintptr_t) (ptr) - (uintptr_t) pure <= PURESIZE)
#define PURE_P(ptr) puresize_h_PURE_P (ptr)

#endif
