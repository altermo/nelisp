#ifndef EMACS_DATA_C
#define EMACS_DATA_C

#include "lisp.h"

DEFUN ("car", Fcar, Scar, 1, 1, 0,
       doc: /* Return the car of LIST.  If LIST is nil, return nil.
Error if LIST is not nil and not a cons cell.  See also `car-safe'.

See Info node `(elisp)Cons Cells' for a discussion of related basic
Lisp concepts such as car, cdr, cons cell and list.  */)
  (register Lisp_Object list)
{
  return CAR (list);
}
DEFUN ("car-safe", Fcar_safe, Scar_safe, 1, 1, 0,
       doc: /* Return the car of OBJECT if it is a cons cell, or else nil.  */)
  (Lisp_Object object)
{
  return CAR_SAFE (object);
}
DEFUN ("cdr", Fcdr, Scdr, 1, 1, 0,
       doc: /* Return the cdr of LIST.  If LIST is nil, return nil.
Error if LIST is not nil and not a cons cell.  See also `cdr-safe'.

See Info node `(elisp)Cons Cells' for a discussion of related basic
Lisp concepts such as cdr, car, cons cell and list.  */)
  (register Lisp_Object list)
{
  return CDR (list);
}
DEFUN ("cdr-safe", Fcdr_safe, Scdr_safe, 1, 1, 0,
       doc: /* Return the cdr of OBJECT if it is a cons cell, or else nil.  */)
  (Lisp_Object object)
{
  return CDR_SAFE (object);
}

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
    defsubr (&Scar);
    defsubr (&Scdr);
    defsubr (&Scar_safe);
    defsubr (&Scdr_safe);
}

#endif
