#include <math.h>

#include "lisp.h"

AVOID
wrong_type_argument (Lisp_Object predicate, Lisp_Object value)
{
  eassert (!TAGGEDP (value, Lisp_Type_Unused0));
  xsignal2 (Qwrong_type_argument, predicate, value);
}

static bool
BOOLFWDP (lispfwd a)
{
  return XFWDTYPE (a) == Lisp_Fwd_Bool;
}
static bool
INTFWDP (lispfwd a)
{
  return XFWDTYPE (a) == Lisp_Fwd_Int;
}
static bool
OBJFWDP (lispfwd a)
{
  return XFWDTYPE (a) == Lisp_Fwd_Obj;
}
static struct Lisp_Boolfwd const *
XBOOLFWD (lispfwd a)
{
  eassert (BOOLFWDP (a));
  return a.fwdptr;
}
static struct Lisp_Intfwd const *
XFIXNUMFWD (lispfwd a)
{
  eassert (INTFWDP (a));
  return a.fwdptr;
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
(register Lisp_Object list) { return CAR (list); }
DEFUN ("car-safe", Fcar_safe, Scar_safe, 1, 1, 0,
       doc: /* Return the car of OBJECT if it is a cons cell, or else nil.  */)
(Lisp_Object object) { return CAR_SAFE (object); }
DEFUN ("cdr", Fcdr, Scdr, 1, 1, 0,
       doc: /* Return the cdr of LIST.  If LIST is nil, return nil.
Error if LIST is not nil and not a cons cell.  See also `cdr-safe'.

See Info node `(elisp)Cons Cells' for a discussion of related basic
Lisp concepts such as cdr, car, cons cell and list.  */)
(register Lisp_Object list) { return CDR (list); }
DEFUN ("cdr-safe", Fcdr_safe, Scdr_safe, 1, 1, 0,
       doc: /* Return the cdr of OBJECT if it is a cons cell, or else nil.  */)
(Lisp_Object object) { return CDR_SAFE (object); }

Lisp_Object
do_symval_forwarding (lispfwd valcontents)
{
  switch (XFWDTYPE (valcontents))
    {
    case Lisp_Fwd_Int:
      return make_int (*XFIXNUMFWD (valcontents)->intvar);

    case Lisp_Fwd_Bool:
      TODO;

    case Lisp_Fwd_Obj:
      return *XOBJFWD (valcontents)->objvar;

    case Lisp_Fwd_Buffer_Obj:
      TODO;

    case Lisp_Fwd_Kboard_Obj:
      TODO;
    default:
      emacs_abort ();
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
    case SYMBOL_VARALIAS:
      TODO;
    case SYMBOL_PLAINVAL:
      return SYMBOL_VAL (sym);
    case SYMBOL_LOCALIZED:
      {
        TODO;
      }
    case SYMBOL_FORWARDED:
      return do_symval_forwarding (SYMBOL_FWD (sym));
    default:
      emacs_abort ();
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

  xsignal1 (Qvoid_variable, symbol);
}

DEFUN ("bare-symbol", Fbare_symbol, Sbare_symbol, 1, 1, 0,
       doc: /* Extract, if need be, the bare symbol from SYM.
SYM is either a symbol or a symbol with position.
Ignore `symbols-with-pos-enabled'.  */)
(register Lisp_Object sym)
{
  if (BARE_SYMBOL_P (sym))
    return sym;
  if (SYMBOL_WITH_POS_P (sym))
    return XSYMBOL_WITH_POS_SYM (sym);
  xsignal2 (Qwrong_type_argument, list2 (Qsymbolp, Qsymbol_with_pos_p), sym);
}

Lisp_Object
default_value (Lisp_Object symbol)
{
  struct Lisp_Symbol *sym;

  CHECK_SYMBOL (symbol);
  sym = XSYMBOL (symbol);

start:
  switch (sym->u.s.redirect)
    {
    case SYMBOL_VARALIAS:
      TODO;
      goto start;
    case SYMBOL_PLAINVAL:
      return SYMBOL_VAL (sym);
    case SYMBOL_LOCALIZED:
      {
        TODO;
      }
    case SYMBOL_FORWARDED:
      {
        lispfwd valcontents = SYMBOL_FWD (sym);

        if (BUFFER_OBJFWDP (valcontents))
          {
            TODO;
          }

        return do_symval_forwarding (valcontents);
      }
    default:
      emacs_abort ();
    }
}
DEFUN ("default-boundp", Fdefault_boundp, Sdefault_boundp, 1, 1, 0,
       doc: /* Return t if SYMBOL has a non-void default value.
A variable may have a buffer-local value.  This function says whether
the variable has a non-void value outside of the current buffer
context.  Also see `default-value'.  */)
(Lisp_Object symbol)
{
  register Lisp_Object value;

  value = default_value (symbol);
  return (BASE_EQ (value, Qunbound) ? Qnil : Qt);
}

void
set_default_internal (Lisp_Object symbol, Lisp_Object value,
                      enum Set_Internal_Bind bindflag, KBOARD *where)
{
  UNUSED (where);
  CHECK_SYMBOL (symbol);
  struct Lisp_Symbol *sym = XSYMBOL (symbol);
  switch (sym->u.s.trapped_write)
    {
    case SYMBOL_NOWRITE:
      if (NILP (Fkeywordp (symbol)) || !EQ (value, Fsymbol_value (symbol)))
        xsignal1 (Qsetting_constant, symbol);
      else
        return;

    case SYMBOL_TRAPPED_WRITE:
      if (sym->u.s.redirect != SYMBOL_PLAINVAL
          && bindflag != SET_INTERNAL_THREAD_SWITCH)
        TODO;
      break;

    case SYMBOL_UNTRAPPED_WRITE:
      break;

    default:
      emacs_abort ();
    }

start:
  switch (sym->u.s.redirect)
    {
    case SYMBOL_VARALIAS:
      TODO;
      goto start;
    case SYMBOL_PLAINVAL:
      set_internal (symbol, value, Qnil, bindflag);
      return;
    case SYMBOL_LOCALIZED:
      {
        TODO;
        return;
      }
    case SYMBOL_FORWARDED:
      {
        lispfwd valcontents = SYMBOL_FWD (sym);

        if (BUFFER_OBJFWDP (valcontents))
          TODO;
        else if (KBOARD_OBJFWDP (valcontents))
          TODO;
        else
          set_internal (symbol, value, Qnil, bindflag);
        return;
      }
    default:
      emacs_abort ();
    }
}

DEFUN ("set-default", Fset_default, Sset_default, 2, 2, 0,
       doc: /* Set SYMBOL's default value to VALUE.  SYMBOL and VALUE are evaluated.
The default value is seen in buffers that do not have their own values
for this variable.  */)
(Lisp_Object symbol, Lisp_Object value)
{
  set_default_internal (symbol, value, SET_INTERNAL_SET, NULL);
  return value;
}

static void
store_symval_forwarding (lispfwd valcontents, Lisp_Object newval)
{
  switch (XFWDTYPE (valcontents))
    {
    case Lisp_Fwd_Int:
      {
        intmax_t i;
        CHECK_INTEGER (newval);
        if (!integer_to_intmax (newval, &i))
          TODO;
        *XFIXNUMFWD (valcontents)->intvar = i;
      }
      break;

    case Lisp_Fwd_Bool:
      *XBOOLFWD (valcontents)->boolvar = !NILP (newval);
      break;

    case Lisp_Fwd_Obj:
      *XOBJFWD (valcontents)->objvar = newval;

#if TODO_NELISP_LATER_AND
      if (XOBJFWD (valcontents)->objvar > (Lisp_Object *) &buffer_defaults
          && XOBJFWD (valcontents)->objvar
               < (Lisp_Object *) (&buffer_defaults + 1))
        {
          TODO;
        }
#endif
      break;
    case Lisp_Fwd_Buffer_Obj:
      TODO;
    case Lisp_Fwd_Kboard_Obj:
      TODO;
    default:
      emacs_abort (); /* goto def; */
    }
}
void
set_internal (Lisp_Object symbol, Lisp_Object newval, Lisp_Object where,
              enum Set_Internal_Bind bindflag)
{
  UNUSED (where);
  UNUSED (bindflag);
  bool voide = BASE_EQ (newval, Qunbound);
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
    default:
      emacs_abort ();
    }

  switch (sym->u.s.redirect)
    {
    case SYMBOL_VARALIAS:
      TODO;
    case SYMBOL_PLAINVAL:
      SET_SYMBOL_VAL (sym, newval);
      return;
    case SYMBOL_LOCALIZED:
      TODO;
    case SYMBOL_FORWARDED:
      {
        lispfwd innercontents = SYMBOL_FWD (sym);
        if (BUFFER_OBJFWDP (innercontents))
          {
            TODO;
          }

        if (voide)
          {
            sym->u.s.redirect = SYMBOL_PLAINVAL;
            SET_SYMBOL_VAL (sym, newval);
          }
        else
          {
            store_symval_forwarding (innercontents, newval);
          }
        break;
      }
    default:
      emacs_abort ();
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

DEFUN ("fset", Ffset, Sfset, 2, 2, 0,
       doc: /* Set SYMBOL's function definition to DEFINITION, and return DEFINITION.
If the resulting chain of function definitions would contain a loop,
signal a `cyclic-function-indirection' error.  */)
(register Lisp_Object symbol, Lisp_Object definition)
{
  CHECK_SYMBOL (symbol);
  if (NILP (symbol) && !NILP (definition))
    xsignal1 (Qsetting_constant, symbol);

#if TODO_NELISP_LATER_AND
  eassert (valid_lisp_object_p (definition));
#endif

  for (Lisp_Object s = definition; SYMBOLP (s) && !NILP (s);
       s = XSYMBOL (s)->u.s.function)
    if (EQ (s, symbol))
      xsignal1 (Qcyclic_function_indirection, symbol);

  set_symbol_function (symbol, definition);

  return definition;
}

void
defalias (Lisp_Object symbol, Lisp_Object definition)
{
  TODO_NELISP_LATER;
  Ffset (symbol, definition);
}

DEFUN ("defalias", Fdefalias, Sdefalias, 2, 3, 0,
       doc: /* Set SYMBOL's function definition to DEFINITION.
Associates the function with the current load file, if any.
The optional third argument DOCSTRING specifies the documentation string
for SYMBOL; if it is omitted or nil, SYMBOL uses the documentation string
determined by DEFINITION.

Internally, this normally uses `fset', but if SYMBOL has a
`defalias-fset-function' property, the associated value is used instead.

The return value is undefined.  */)
(register Lisp_Object symbol, Lisp_Object definition, Lisp_Object docstring)
{
  CHECK_SYMBOL (symbol);
  if (!NILP (Vpurify_flag))
    TODO;

  defalias (symbol, definition);

  if (!NILP (docstring))
    Fput (symbol, Qfunction_documentation, docstring);
  return symbol;
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

DEFUN ("consp", Fconsp, Sconsp, 1, 1, 0,
       doc: /* Return t if OBJECT is a cons cell.  */
       attributes: const)
(Lisp_Object object)
{
  if (CONSP (object))
    return Qt;
  return Qnil;
}

DEFUN ("atom", Fatom, Satom, 1, 1, 0,
       doc: /* Return t if OBJECT is not a cons cell.  This includes nil.  */
       attributes: const)
(Lisp_Object object)
{
  if (CONSP (object))
    return Qnil;
  return Qt;
}

DEFUN ("listp", Flistp, Slistp, 1, 1, 0,
       doc: /* Return t if OBJECT is a list, that is, a cons cell or nil.
Otherwise, return nil.  */
       attributes: const)
(Lisp_Object object)
{
  if (CONSP (object) || NILP (object))
    return Qt;
  return Qnil;
}

DEFUN ("nlistp", Fnlistp, Snlistp, 1, 1, 0,
       doc: /* Return t if OBJECT is not a list.  Lists include nil.  */
       attributes: const)
(Lisp_Object object)
{
  if (CONSP (object) || NILP (object))
    return Qnil;
  return Qt;
}

DEFUN ("bare-symbol-p", Fbare_symbol_p, Sbare_symbol_p, 1, 1, 0,
       doc: /* Return t if OBJECT is a symbol, but not a symbol together with position.  */
       attributes: const)
(Lisp_Object object)
{
  if (BARE_SYMBOL_P (object))
    return Qt;
  return Qnil;
}

DEFUN ("symbol-with-pos-p", Fsymbol_with_pos_p, Ssymbol_with_pos_p, 1, 1, 0,
       doc: /* Return t if OBJECT is a symbol together with position.
Ignore `symbols-with-pos-enabled'.  */
       attributes: const)
(Lisp_Object object)
{
  if (SYMBOL_WITH_POS_P (object))
    return Qt;
  return Qnil;
}

DEFUN ("symbolp", Fsymbolp, Ssymbolp, 1, 1, 0,
       doc: /* Return t if OBJECT is a symbol.  */
       attributes: const)
(Lisp_Object object)
{
  if (SYMBOLP (object))
    return Qt;
  return Qnil;
}

DEFUN ("keywordp", Fkeywordp, Skeywordp, 1, 1, 0,
       doc: /* Return t if OBJECT is a keyword.
This means that it is a symbol with a print name beginning with `:'
interned in the initial obarray.  */)
(Lisp_Object object)
{
  if (SYMBOLP (object) && SREF (SYMBOL_NAME (object), 0) == ':'
      && SYMBOL_INTERNED_IN_INITIAL_OBARRAY_P (object))
    return Qt;
  return Qnil;
}

DEFUN ("vectorp", Fvectorp, Svectorp, 1, 1, 0,
       doc: /* Return t if OBJECT is a vector.  */)
(Lisp_Object object)
{
  if (VECTORP (object))
    return Qt;
  return Qnil;
}

DEFUN ("recordp", Frecordp, Srecordp, 1, 1, 0,
       doc: /* Return t if OBJECT is a record.  */)
(Lisp_Object object)
{
  if (RECORDP (object))
    return Qt;
  return Qnil;
}

DEFUN ("stringp", Fstringp, Sstringp, 1, 1, 0,
       doc: /* Return t if OBJECT is a string.  */
       attributes: const)
(Lisp_Object object)
{
  if (STRINGP (object))
    return Qt;
  return Qnil;
}

DEFUN ("null", Fnull, Snull, 1, 1, 0,
       doc: /* Return t if OBJECT is nil, and return nil otherwise.  */
       attributes: const)
(Lisp_Object object)
{
  if (NILP (object))
    return Qt;
  return Qnil;
}

DEFUN ("eq", Feq, Seq, 2, 2, 0,
       doc: /* Return t if the two args are the same Lisp object.  */
       attributes: const)
(Lisp_Object obj1, Lisp_Object obj2)
{
  if (EQ (obj1, obj2))
    return Qt;
  return Qnil;
}

static Lisp_Object
check_number_coerce_marker (Lisp_Object x)
{
  if (MARKERP (x))
    TODO; // return make_fixnum (marker_position (x));
  CHECK_TYPE (NUMBERP (x), Qnumber_or_marker_p, x);
  return x;
}

Lisp_Object
arithcompare (Lisp_Object num1, Lisp_Object num2,
              enum Arith_Comparison comparison)
{
  EMACS_INT i1 = 0, i2 = 0;
  bool lt, eq = true, gt;
  bool test;

  num1 = check_number_coerce_marker (num1);
  num2 = check_number_coerce_marker (num2);

  if (FLOATP (num1))
    {
      double f1 = XFLOAT_DATA (num1);
      if (FLOATP (num2))
        {
          double f2 = XFLOAT_DATA (num2);
          lt = f1 < f2;
          eq = f1 == f2;
          gt = f1 > f2;
        }
      else if (FIXNUMP (num2))
        {
          double f2 = XFIXNUM (num2);
          lt = f1 < f2;
          eq = f1 == f2;
          gt = f1 > f2;
          i1 = f2;
          i2 = XFIXNUM (num2);
        }
      else if (isnan (f1))
        lt = eq = gt = false;
      else
        TODO; // i2 = mpz_cmp_d (*xbignum_val (num2), f1);
    }
  else if (FIXNUMP (num1))
    {
      if (FLOATP (num2))
        {
          double f1 = XFIXNUM (num1), f2 = XFLOAT_DATA (num2);
          lt = f1 < f2;
          eq = f1 == f2;
          gt = f1 > f2;
          i1 = XFIXNUM (num1);
          i2 = f1;
        }
      else if (FIXNUMP (num2))
        {
          i1 = XFIXNUM (num1);
          i2 = XFIXNUM (num2);
        }
      else
        TODO; // i2 = mpz_sgn (*xbignum_val (num2));
    }
  else if (FLOATP (num2))
    {
      double f2 = XFLOAT_DATA (num2);
      if (isnan (f2))
        lt = eq = gt = false;
      else
        TODO; // i1 = mpz_cmp_d (*xbignum_val (num1), f2);
    }
  else if (FIXNUMP (num2))
    TODO; // i1 = mpz_sgn (*xbignum_val (num1));
  else
    TODO; // i1 = mpz_cmp (*xbignum_val (num1), *xbignum_val (num2));

  if (eq)
    {
      lt = i1 < i2;
      eq = i1 == i2;
      gt = i1 > i2;
    }

  switch (comparison)
    {
    case ARITH_EQUAL:
      test = eq;
      break;

    case ARITH_NOTEQUAL:
      test = !eq;
      break;

    case ARITH_LESS:
      test = lt;
      break;

    case ARITH_LESS_OR_EQUAL:
      test = lt | eq;
      break;

    case ARITH_GRTR:
      test = gt;
      break;

    case ARITH_GRTR_OR_EQUAL:
      test = gt | eq;
      break;

    default:
      eassume (false);
    }

  return test ? Qt : Qnil;
}

static Lisp_Object
minmax_driver (ptrdiff_t nargs, Lisp_Object *args,
               enum Arith_Comparison comparison)
{
  Lisp_Object accum = check_number_coerce_marker (args[0]);
  for (ptrdiff_t argnum = 1; argnum < nargs; argnum++)
    {
      Lisp_Object val = check_number_coerce_marker (args[argnum]);
      if (!NILP (arithcompare (val, accum, comparison)))
        accum = val;
      else if (FLOATP (val) && isnan (XFLOAT_DATA (val)))
        return val;
    }
  return accum;
}

DEFUN ("max", Fmax, Smax, 1, MANY, 0,
       doc: /* Return largest of all the arguments (which must be numbers or markers).
The value is always a number; markers are converted to numbers.
usage: (max NUMBER-OR-MARKER &rest NUMBERS-OR-MARKERS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  return minmax_driver (nargs, args, ARITH_GRTR);
}

DEFUN ("min", Fmin, Smin, 1, MANY, 0,
       doc: /* Return smallest of all the arguments (which must be numbers or markers).
The value is always a number; markers are converted to numbers.
usage: (min NUMBER-OR-MARKER &rest NUMBERS-OR-MARKERS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  return minmax_driver (nargs, args, ARITH_LESS);
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
  DEFSYM (Qintegerp, "integerp");
  DEFSYM (Qarrayp, "arrayp");
  DEFSYM (Qnumber_or_marker_p, "number-or-marker-p");
  DEFSYM (Qbufferp, "bufferp");
  DEFSYM (Qsymbol_with_pos_p, "symbol-with-pos-p");

  DEFSYM (Qvoid_function, "void-function");
  DEFSYM (Qwrong_type_argument, "wrong-type-argument");
  DEFSYM (Qwrong_number_of_arguments, "wrong-number-of-arguments");
  DEFSYM (Qvoid_variable, "void-variable");
  DEFSYM (Qsetting_constant, "setting-constant");
  DEFSYM (Qcyclic_function_indirection, "cyclic-function-indirection");
  DEFSYM (Qinvalid_function, "invalid-function");

  Lisp_Object error_tail = Fcons (Qerror, Qnil);

  Fput (Qerror, Qerror_conditions, error_tail);
  Fput (Qerror, Qerror_message, build_pure_c_string ("error"));

#define PUT_ERROR(sym, tail, msg)                   \
  Fput (sym, Qerror_conditions, Fcons (sym, tail)); \
  Fput (sym, Qerror_message, build_pure_c_string (msg))

  PUT_ERROR (Qvoid_function, error_tail,
             "Symbol's function definition is void");
  PUT_ERROR (Qwrong_type_argument, error_tail, "Wrong type argument");
  PUT_ERROR (Qwrong_number_of_arguments, error_tail,
             "Wrong number of arguments");
  PUT_ERROR (Qvoid_variable, error_tail, "Symbol's value as variable is void");
  DEFSYM (Qcircular_list, "circular-list");
  PUT_ERROR (Qcircular_list, error_tail, "List contains a loop");
  PUT_ERROR (Qsetting_constant, error_tail, "Attempt to set a constant symbol");
  PUT_ERROR (Qcyclic_function_indirection, error_tail,
             "Symbol's chain of function indirections contains a loop");
  PUT_ERROR (Qinvalid_function, error_tail, "Invalid function");

  defsubr (&Ssymbol_value);
  defsubr (&Sbare_symbol);
  defsubr (&Sdefault_boundp);
  defsubr (&Sset_default);
  defsubr (&Scar);
  defsubr (&Scdr);
  defsubr (&Scar_safe);
  defsubr (&Scdr_safe);
  defsubr (&Sset);
  defsubr (&Sfset);
  defsubr (&Sdefalias);
  defsubr (&Ssetcar);
  defsubr (&Ssetcdr);
  defsubr (&Sconsp);
  defsubr (&Satom);
  defsubr (&Slistp);
  defsubr (&Snlistp);
  defsubr (&Sbare_symbol_p);
  defsubr (&Ssymbol_with_pos_p);
  defsubr (&Ssymbolp);
  defsubr (&Skeywordp);
  defsubr (&Svectorp);
  defsubr (&Srecordp);
  defsubr (&Sstringp);
  defsubr (&Snull);
  defsubr (&Seq);
  defsubr (&Smax);
  defsubr (&Smin);
}
