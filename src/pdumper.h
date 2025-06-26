#ifndef EMACS_PDUMPER_H
#define EMACS_PDUMPER_H

#include "lisp.h"

INLINE bool
pdumper_object_p (const void *obj)
{
  UNUSED (obj);
  TODO_NELISP_LATER;
  return false;
}
INLINE bool
pdumper_marked_p (const void *obj)
{
  UNUSED (obj);
  TODO_NELISP_LATER;
  return false;
}
INLINE void
pdumper_set_marked (const void *obj)
{
  UNUSED (obj);
  TODO_NELISP_LATER;
}
INLINE bool
pdumper_cold_object_p (const void *obj)
{
  UNUSED (obj);
  TODO_NELISP_LATER;
  return false;
}

#endif
