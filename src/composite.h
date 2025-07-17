#ifndef EMACS_COMPOSITE_H
#define EMACS_COMPOSITE_H

#include "lisp.h"

enum composition_method
{
  COMPOSITION_RELATIVE,

  COMPOSITION_WITH_RULE,

  COMPOSITION_WITH_ALTCHARS,

  COMPOSITION_WITH_RULE_ALTCHARS,

  COMPOSITION_NO
};

#define MAX_COMPOSITION_COMPONENTS 16

extern void make_composition_value_copy (Lisp_Object);

#endif
