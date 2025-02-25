#ifndef EMACS_EVAL_C
#define EMACS_EVAL_C

#include "lisp.h"

Lisp_Object
find_symbol_value (Lisp_Object symbol)
{
  struct Lisp_Symbol *sym;

#if TODO_NELISP_LATER_AND
  CHECK_SYMBOL (symbol);
#endif
  sym = XSYMBOL (symbol);

  switch (sym->u.s.redirect)
    {
    case SYMBOL_VARALIAS: TODO;
    case SYMBOL_PLAINVAL: return SYMBOL_VAL (sym);
    case SYMBOL_LOCALIZED:
      {
                TODO;
      }
    case SYMBOL_FORWARDED:
        TODO;
    default: emacs_abort ();
    }
}
DEFUN ("symbol-value", Fsymbol_value, Ssymbol_value, 1, 1, 0,
       doc: /* Return SYMBOL's value.  Error if that is void.
Note that if `lexical-binding' is in effect, this returns the
global value outside of any lexical scope.  */)
  (Lisp_Object symbol)
{
  Lisp_Object val;

  val = find_symbol_value (symbol);
  if (!BASE_EQ (val, Qunbound))
    return val;

#if TODO_NELISP_LATER_AND
  xsignal1 (Qvoid_variable, symbol);
#else
    TODO
#endif
}

void
syms_of_data (void)
{
    defsubr (&Ssymbol_value);
}

#endif
