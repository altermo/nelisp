#include "lisp.h"

AVOID
wrong_type_argument (Lisp_Object predicate, Lisp_Object value)
{
  eassert (!TAGGEDP (value, Lisp_Type_Unused0));
  xsignal2 (Qwrong_type_argument, predicate, value);
}

static bool
OBJFWDP (lispfwd a)
{
  return XFWDTYPE (a) == Lisp_Fwd_Obj;
}
static struct Lisp_Objfwd const *
XOBJFWD (lispfwd a)
{
  eassert (OBJFWDP (a));
  return a.fwdptr;
}

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
do_symval_forwarding (lispfwd valcontents)
{
    switch (XFWDTYPE (valcontents))
    {
        case Lisp_Fwd_Int:
            TODO;

        case Lisp_Fwd_Bool:
            TODO;

        case Lisp_Fwd_Obj:
            return *XOBJFWD (valcontents)->objvar;

        case Lisp_Fwd_Buffer_Obj:
            TODO;

        case Lisp_Fwd_Kboard_Obj:
            TODO;
        default: emacs_abort ();
    }
}
Lisp_Object
find_symbol_value (Lisp_Object symbol)
{
  struct Lisp_Symbol *sym;

  CHECK_SYMBOL (symbol);
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
        return do_symval_forwarding (SYMBOL_FWD (sym));
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
    TODO;
#endif
}

void
set_internal (Lisp_Object symbol, Lisp_Object newval, Lisp_Object where,
              enum Set_Internal_Bind bindflag)
{
    UNUSED(where);
    UNUSED(bindflag);
  CHECK_SYMBOL (symbol);
  struct Lisp_Symbol *sym = XSYMBOL (symbol);
  switch (sym->u.s.trapped_write)
    {
    case SYMBOL_NOWRITE:
        TODO;
    case SYMBOL_TRAPPED_WRITE:
        TODO;
    case SYMBOL_UNTRAPPED_WRITE:
      break;
    default: emacs_abort ();
    }

  switch (sym->u.s.redirect)
    {
    case SYMBOL_VARALIAS: TODO;
    case SYMBOL_PLAINVAL: SET_SYMBOL_VAL (sym , newval); return;
    case SYMBOL_LOCALIZED: TODO;
    case SYMBOL_FORWARDED: TODO;
    default: emacs_abort ();
    }
  return;
}
DEFUN ("set", Fset, Sset, 2, 2, 0,
       doc: /* Set SYMBOL's value to NEWVAL, and return NEWVAL.  */)
  (register Lisp_Object symbol, Lisp_Object newval)
{
  set_internal (symbol, newval, Qnil, SET_INTERNAL_SET);
  return newval;
}

DEFUN ("setcar", Fsetcar, Ssetcar, 2, 2, 0,
       doc: /* Set the car of CELL to be NEWCAR.  Returns NEWCAR.  */)
  (register Lisp_Object cell, Lisp_Object newcar)
{
  CHECK_CONS (cell);
  CHECK_IMPURE (cell, XCONS (cell));
  XSETCAR (cell, newcar);
  return newcar;
}

DEFUN ("setcdr", Fsetcdr, Ssetcdr, 2, 2, 0,
       doc: /* Set the cdr of CELL to be NEWCDR.  Returns NEWCDR.  */)
  (register Lisp_Object cell, Lisp_Object newcdr)
{
  CHECK_CONS (cell);
  CHECK_IMPURE (cell, XCONS (cell));
  XSETCDR (cell, newcdr);
  return newcdr;
}

void
syms_of_data (void)
{
    DEFSYM (Qquote, "quote");
    DEFSYM (Qtop_level, "top-level");
    DEFSYM (Qerror_conditions, "error-conditions");
    DEFSYM (Qerror_message, "error-message");

    DEFSYM (Qsymbolp, "symbolp");
    DEFSYM (Qconsp, "consp");
    DEFSYM (Qlistp, "listp");
    DEFSYM (Qwholenump, "wholenump");
    DEFSYM (Qstringp, "stringp");

    DEFSYM (Qvoid_function, "void-function");
    DEFSYM (Qwrong_type_argument, "wrong-type-argument");

    Lisp_Object error_tail = Fcons (Qerror, Qnil);

    Fput (Qerror, Qerror_conditions,
        error_tail);
    Fput (Qerror, Qerror_message,
        build_pure_c_string ("error"));

#define PUT_ERROR(sym, tail, msg)			\
    Fput (sym, Qerror_conditions, Fcons (sym, tail)); \
    Fput (sym, Qerror_message, build_pure_c_string (msg))

    PUT_ERROR (Qvoid_function, error_tail,
               "Symbol's function definition is void");
    PUT_ERROR (Qwrong_type_argument, error_tail, "Wrong type argument");

    defsubr (&Ssymbol_value);
    defsubr (&Scar);
    defsubr (&Scdr);
    defsubr (&Scar_safe);
    defsubr (&Scdr_safe);
    defsubr (&Sset);
    defsubr (&Ssetcar);
    defsubr (&Ssetcdr);
}
