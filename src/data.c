#include <math.h>

#include "lisp.h"
#include "bignum.h"
#include "buffer.h"
#include "character.h"

AVOID
wrong_type_argument (Lisp_Object predicate, Lisp_Object value)
{
  eassert (!TAGGEDP (value, Lisp_Type_Unused0));
  xsignal2 (Qwrong_type_argument, predicate, value);
}

void
args_out_of_range (Lisp_Object a1, Lisp_Object a2)
{
  xsignal2 (Qargs_out_of_range, a1, a2);
}

void
args_out_of_range_3 (Lisp_Object a1, Lisp_Object a2, Lisp_Object a3)
{
  xsignal3 (Qargs_out_of_range, a1, a2, a3);
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

Lisp_Object do_symval_forwarding (lispfwd valcontents);
static void store_symval_forwarding (lispfwd valcontents, Lisp_Object newval,
                                     struct buffer *buf);

static void
swap_in_symval_forwarding (struct Lisp_Symbol *symbol,
                           struct Lisp_Buffer_Local_Value *blv)
{
  register Lisp_Object tem1;

  eassert (blv == SYMBOL_BLV (symbol));

  tem1 = blv->where;

  if (NILP (tem1) || current_buffer != XBUFFER (tem1))
    {
      tem1 = blv->valcell;
      if (blv->fwd.fwdptr)
        set_blv_value (blv, do_symval_forwarding (blv->fwd));
      {
        Lisp_Object var;
        XSETSYMBOL (var, symbol);
        tem1 = assq_no_quit (var, BVAR (current_buffer, local_var_alist));
        set_blv_where (blv, Fcurrent_buffer ());
      }
      if (!(blv->found = !NILP (tem1)))
        tem1 = blv->defcell;

      set_blv_valcell (blv, tem1);
      if (blv->fwd.fwdptr)
        store_symval_forwarding (blv->fwd, blv_value (blv), NULL);
    }
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
      return (*XBOOLFWD (valcontents)->boolvar ? Qt : Qnil);

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
        struct Lisp_Buffer_Local_Value *blv = SYMBOL_BLV (sym);
        swap_in_symval_forwarding (sym, blv);
        return (blv->fwd.fwdptr ? do_symval_forwarding (blv->fwd)
                                : blv_value (blv));
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

DEFUN ("default-value", Fdefault_value, Sdefault_value, 1, 1, 0,
       doc: /* Return SYMBOL's default value.
This is the value that is seen in buffers that do not have their own values
for this variable.  The default value is meaningful for variables with
local bindings in certain buffers.  */)
(Lisp_Object symbol)
{
  Lisp_Object value = default_value (symbol);
  if (!BASE_EQ (value, Qunbound))
    return value;

  xsignal1 (Qvoid_variable, symbol);
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
        struct Lisp_Buffer_Local_Value *blv = SYMBOL_BLV (sym);

        XSETCDR (blv->defcell, value);

        if (blv->fwd.fwdptr && EQ (blv->defcell, blv->valcell))
          store_symval_forwarding (blv->fwd, value, NULL);
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

union Lisp_Val_Fwd
{
  Lisp_Object value;
  lispfwd fwd;
};

static struct Lisp_Buffer_Local_Value *
make_blv (struct Lisp_Symbol *sym, bool forwarded,
          union Lisp_Val_Fwd valcontents)
{
  struct Lisp_Buffer_Local_Value *blv = xmalloc (sizeof *blv);
  Lisp_Object symbol;
  Lisp_Object tem;

  XSETSYMBOL (symbol, sym);
  tem = Fcons (symbol, (forwarded ? do_symval_forwarding (valcontents.fwd)
                                  : valcontents.value));

  eassert (!(forwarded && BUFFER_OBJFWDP (valcontents.fwd)));
  eassert (!(forwarded && KBOARD_OBJFWDP (valcontents.fwd)));
  if (forwarded)
    blv->fwd = valcontents.fwd;
  else
    blv->fwd.fwdptr = NULL;
  set_blv_where (blv, Qnil);
  blv->local_if_set = 0;
  set_blv_defcell (blv, tem);
  set_blv_valcell (blv, tem);
  set_blv_found (blv, false);
#if TODO_NELISP_LATER_AND
  __lsan_ignore_object (blv);
#endif
  return blv;
}

DEFUN ("make-variable-buffer-local", Fmake_variable_buffer_local,
       Smake_variable_buffer_local, 1, 1, "vMake Variable Buffer Local: ",
       doc: /* Make VARIABLE become buffer-local whenever it is set.
At any time, the value for the current buffer is in effect,
unless the variable has never been set in this buffer,
in which case the default value is in effect.
Note that binding the variable with `let', or setting it while
a `let'-style binding made in this buffer is in effect,
does not make the variable buffer-local.  Return VARIABLE.

This globally affects all uses of this variable, so it belongs together with
the variable declaration, rather than with its uses (if you just want to make
a variable local to the current buffer for one particular use, use
`make-local-variable').  Buffer-local bindings are normally cleared
while setting up a new major mode, unless they have a `permanent-local'
property.

The function `default-value' gets the default value and `set-default' sets it.

See also `defvar-local'.  */)
(register Lisp_Object variable)
{
  struct Lisp_Symbol *sym;
  struct Lisp_Buffer_Local_Value *blv = NULL;
  union Lisp_Val_Fwd valcontents UNINIT;
  bool forwarded UNINIT;

  CHECK_SYMBOL (variable);
  sym = XSYMBOL (variable);

start:
  switch (sym->u.s.redirect)
    {
    case SYMBOL_VARALIAS:
      sym = SYMBOL_ALIAS (sym);
      goto start;
    case SYMBOL_PLAINVAL:
      forwarded = 0;
      valcontents.value = SYMBOL_VAL (sym);
      if (BASE_EQ (valcontents.value, Qunbound))
        valcontents.value = Qnil;
      break;
    case SYMBOL_LOCALIZED:
      blv = SYMBOL_BLV (sym);
      break;
    case SYMBOL_FORWARDED:
      forwarded = 1;
      valcontents.fwd = SYMBOL_FWD (sym);
      if (KBOARD_OBJFWDP (valcontents.fwd))
        error ("Symbol %s may not be buffer-local",
               SDATA (SYMBOL_NAME (variable)));
      else if (BUFFER_OBJFWDP (valcontents.fwd))
        return variable;
      break;
    default:
      emacs_abort ();
    }

  if (SYMBOL_CONSTANT_P (variable))
    xsignal1 (Qsetting_constant, variable);

  if (!blv)
    {
      blv = make_blv (sym, forwarded, valcontents);
      sym->u.s.redirect = SYMBOL_LOCALIZED;
      SET_SYMBOL_BLV (sym, blv);
    }

  blv->local_if_set = 1;
  return variable;
}

DEFUN ("local-variable-p", Flocal_variable_p, Slocal_variable_p,
       1, 2, 0,
       doc: /* Non-nil if VARIABLE has a local binding in buffer BUFFER.
BUFFER defaults to the current buffer.

Also see `buffer-local-boundp'.*/)
(Lisp_Object variable, Lisp_Object buffer)
{
  struct buffer *buf = decode_buffer (buffer);
  struct Lisp_Symbol *sym;

  CHECK_SYMBOL (variable);
  sym = XSYMBOL (variable);

start:
  switch (sym->u.s.redirect)
    {
    case SYMBOL_VARALIAS:
      sym = SYMBOL_ALIAS (sym);
      goto start;
    case SYMBOL_PLAINVAL:
      return Qnil;
    case SYMBOL_LOCALIZED:
      {
        Lisp_Object tmp;
        struct Lisp_Buffer_Local_Value *blv = SYMBOL_BLV (sym);
        XSETBUFFER (tmp, buf);
        XSETSYMBOL (variable, sym);

        if (EQ (blv->where, tmp))
          return blv_found (blv) ? Qt : Qnil;
        else
          return NILP (assq_no_quit (variable, BVAR (buf, local_var_alist)))
                   ? Qnil
                   : Qt;
      }
    case SYMBOL_FORWARDED:
      {
        lispfwd valcontents = SYMBOL_FWD (sym);
        if (BUFFER_OBJFWDP (valcontents))
          TODO;
        return Qnil;
      }
    default:
      emacs_abort ();
    }
}

DEFUN ("local-variable-if-set-p", Flocal_variable_if_set_p, Slocal_variable_if_set_p,
       1, 2, 0,
       doc: /* Non-nil if VARIABLE is local in buffer BUFFER when set there.
BUFFER defaults to the current buffer.

More precisely, return non-nil if either VARIABLE already has a local
value in BUFFER, or if VARIABLE is automatically buffer-local (see
`make-variable-buffer-local').  */)
(register Lisp_Object variable, Lisp_Object buffer)
{
  struct Lisp_Symbol *sym;

  CHECK_SYMBOL (variable);
  sym = XSYMBOL (variable);

start:
  switch (sym->u.s.redirect)
    {
    case SYMBOL_VARALIAS:
      sym = SYMBOL_ALIAS (sym);
      goto start;
    case SYMBOL_PLAINVAL:
      return Qnil;
    case SYMBOL_LOCALIZED:
      {
        struct Lisp_Buffer_Local_Value *blv = SYMBOL_BLV (sym);
        if (blv->local_if_set)
          return Qt;
        XSETSYMBOL (variable, sym);
        return Flocal_variable_p (variable, buffer);
      }
    case SYMBOL_FORWARDED:
      return (BUFFER_OBJFWDP (SYMBOL_FWD (sym)) ? Qt : Qnil);
    default:
      emacs_abort ();
    }
}

static void
store_symval_forwarding (lispfwd valcontents, Lisp_Object newval,
                         struct buffer *buf)
{
  UNUSED (buf);
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
      {
        struct Lisp_Buffer_Local_Value *blv = SYMBOL_BLV (sym);
        if (NILP (where))
          XSETBUFFER (where, current_buffer);

        if (!EQ (blv->where, where) || (EQ (blv->valcell, blv->defcell)))
          {
            if (blv->fwd.fwdptr)
              set_blv_value (blv, do_symval_forwarding (blv->fwd));

            XSETSYMBOL (symbol, sym);
            Lisp_Object tem1
              = assq_no_quit (symbol, BVAR (XBUFFER (where), local_var_alist));
            set_blv_where (blv, where);
            blv->found = true;

            if (NILP (tem1))
              {
                if (bindflag || !blv->local_if_set || (TODO, false))
                  {
                    blv->found = false;
                    tem1 = blv->defcell;
                  }
                else
                  {
                    tem1 = Fcons (symbol, XCDR (blv->defcell));
                    bset_local_var_alist (XBUFFER (where),
                                          Fcons (tem1, BVAR (XBUFFER (where),
                                                             local_var_alist)));
                  }
              }

            set_blv_valcell (blv, tem1);
          }

        /* Store the new value in the cons cell.  */
        set_blv_value (blv, newval);

        if (blv->fwd.fwdptr)
          {
            if (voide)
              /* If storing void (making the symbol void), forward only through
                 buffer-local indicator, not through Lisp_Objfwd, etc.  */
              blv->fwd.fwdptr = NULL;
            else
              store_symval_forwarding (blv->fwd, newval,
                                       BUFFERP (where) ? XBUFFER (where)
                                                       : current_buffer);
          }
        break;
      }
    case SYMBOL_FORWARDED:
      {
        struct buffer *buf = BUFFERP (where) ? XBUFFER (where) : current_buffer;
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
            store_symval_forwarding (innercontents, newval, buf);
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

DEFUN ("boundp", Fboundp, Sboundp, 1, 1, 0,
       doc: /* Return t if SYMBOL's value is not void.
Note that if `lexical-binding' is in effect, this refers to the
global value outside of any lexical scope.  */)
(register Lisp_Object symbol)
{
  Lisp_Object valcontents;
  struct Lisp_Symbol *sym;
  CHECK_SYMBOL (symbol);
  sym = XSYMBOL (symbol);

start:
  switch (sym->u.s.redirect)
    {
    case SYMBOL_PLAINVAL:
      valcontents = SYMBOL_VAL (sym);
      break;
    case SYMBOL_VARALIAS:
      sym = SYMBOL_ALIAS (sym);
      goto start;
    case SYMBOL_LOCALIZED:
      {
        TODO;
      }
    case SYMBOL_FORWARDED:
      return Qt;
    default:
      emacs_abort ();
    }

  return (BASE_EQ (valcontents, Qunbound) ? Qnil : Qt);
}

DEFUN ("fboundp", Ffboundp, Sfboundp, 1, 1, 0,
       doc: /* Return t if SYMBOL's function definition is not nil.  */)
(Lisp_Object symbol)
{
  CHECK_SYMBOL (symbol);
  return NILP (XSYMBOL (symbol)->u.s.function) ? Qnil : Qt;
}

DEFUN ("symbol-name", Fsymbol_name, Ssymbol_name, 1, 1, 0,
       doc: /* Return SYMBOL's name, a string.

Warning: never alter the string returned by `symbol-name'.
Doing that might make Emacs dysfunctional, and might even crash Emacs.  */)
(register Lisp_Object symbol)
{
  register Lisp_Object name;

  CHECK_SYMBOL (symbol);
  name = SYMBOL_NAME (symbol);
  return name;
}

DEFUN ("fmakunbound", Ffmakunbound, Sfmakunbound, 1, 1, 0,
       doc: /* Make SYMBOL's function definition be nil.
Return SYMBOL.

If a function definition is nil, trying to call a function by
that name will cause a `void-function' error.  For more details, see
Info node `(elisp) Function Cells'.

See also `makunbound'.  */)
(register Lisp_Object symbol)
{
  CHECK_SYMBOL (symbol);
  if (NILP (symbol) || EQ (symbol, Qt))
    xsignal1 (Qsetting_constant, symbol);
  set_symbol_function (symbol, Qnil);
  return symbol;
}

DEFUN ("symbol-function", Fsymbol_function, Ssymbol_function, 1, 1, 0,
       doc: /* Return SYMBOL's function definition.  */)
(Lisp_Object symbol)
{
  CHECK_SYMBOL (symbol);
  return XSYMBOL (symbol)->u.s.function;
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

DEFUN ("multibyte-string-p", Fmultibyte_string_p, Smultibyte_string_p,
       1, 1, 0,
       doc: /* Return t if OBJECT is a multibyte string.
Return nil if OBJECT is either a unibyte string, or not a string.  */)
(Lisp_Object object)
{
  if (STRINGP (object) && STRING_MULTIBYTE (object))
    return Qt;
  return Qnil;
}

DEFUN ("char-table-p", Fchar_table_p, Schar_table_p, 1, 1, 0,
       doc: /* Return t if OBJECT is a char-table.  */)
(Lisp_Object object)
{
  if (CHAR_TABLE_P (object))
    return Qt;
  return Qnil;
}

DEFUN ("vector-or-char-table-p", Fvector_or_char_table_p,
       Svector_or_char_table_p, 1, 1, 0,
       doc: /* Return t if OBJECT is a char-table or vector.  */)
(Lisp_Object object)
{
  if (VECTORP (object) || CHAR_TABLE_P (object))
    return Qt;
  return Qnil;
}

DEFUN ("bool-vector-p", Fbool_vector_p, Sbool_vector_p, 1, 1, 0,
       doc: /* Return t if OBJECT is a bool-vector.  */)
(Lisp_Object object)
{
  if (BOOL_VECTOR_P (object))
    return Qt;
  return Qnil;
}

DEFUN ("arrayp", Farrayp, Sarrayp, 1, 1, 0,
       doc: /* Return t if OBJECT is an array (string or vector).  */)
(Lisp_Object object)
{
  if (ARRAYP (object))
    return Qt;
  return Qnil;
}

DEFUN ("sequencep", Fsequencep, Ssequencep, 1, 1, 0,
       doc: /* Return t if OBJECT is a sequence (list or array).  */)
(register Lisp_Object object)
{
  if (CONSP (object) || NILP (object) || ARRAYP (object))
    return Qt;
  return Qnil;
}

DEFUN ("bufferp", Fbufferp, Sbufferp, 1, 1, 0,
       doc: /* Return t if OBJECT is an editor buffer.  */)
(Lisp_Object object)
{
  if (BUFFERP (object))
    return Qt;
  return Qnil;
}

DEFUN ("markerp", Fmarkerp, Smarkerp, 1, 1, 0,
       doc: /* Return t if OBJECT is a marker (editor pointer).  */)
(Lisp_Object object)
{
  if (MARKERP (object))
    return Qt;
  return Qnil;
}

DEFUN ("subrp", Fsubrp, Ssubrp, 1, 1, 0,
       doc: /* Return t if OBJECT is a built-in or native compiled Lisp function.

See also `primitive-function-p' and `native-comp-function-p'.  */)
(Lisp_Object object)
{
  if (SUBRP (object))
    return Qt;
  return Qnil;
}

DEFUN ("closurep", Fclosurep, Sclosurep,
       1, 1, 0,
       doc: /* Return t if OBJECT is a function of type `closure'.  */)
(Lisp_Object object)
{
  if (CLOSUREP (object))
    return Qt;
  return Qnil;
}

DEFUN ("byte-code-function-p", Fbyte_code_function_p, Sbyte_code_function_p,
       1, 1, 0,
       doc: /* Return t if OBJECT is a byte-compiled function object.  */)
(Lisp_Object object)
{
  if (CLOSUREP (object) && STRINGP (AREF (object, CLOSURE_CODE)))
    return Qt;
  return Qnil;
}

DEFUN ("interpreted-function-p", Finterpreted_function_p,
       Sinterpreted_function_p, 1, 1, 0,
       doc: /* Return t if OBJECT is a function of type `interpreted-function'.  */)
(Lisp_Object object)
{
  if (CLOSUREP (object) && CONSP (AREF (object, CLOSURE_CODE)))
    return Qt;
  return Qnil;
}

DEFUN ("module-function-p", Fmodule_function_p, Smodule_function_p, 1, 1, NULL,
       doc: /* Return t if OBJECT is a function loaded from a dynamic module.  */
       attributes: const)
(Lisp_Object object) { return MODULE_FUNCTIONP (object) ? Qt : Qnil; }

DEFUN ("char-or-string-p", Fchar_or_string_p, Schar_or_string_p, 1, 1, 0,
       doc: /* Return t if OBJECT is a character or a string.  */
       attributes: const)
(register Lisp_Object object)
{
  if (CHARACTERP (object) || STRINGP (object))
    return Qt;
  return Qnil;
}

DEFUN ("integerp", Fintegerp, Sintegerp, 1, 1, 0,
       doc: /* Return t if OBJECT is an integer.  */
       attributes: const)
(Lisp_Object object)
{
  if (INTEGERP (object))
    return Qt;
  return Qnil;
}

DEFUN ("integer-or-marker-p", Finteger_or_marker_p, Sinteger_or_marker_p, 1, 1, 0,
       doc: /* Return t if OBJECT is an integer or a marker (editor pointer).  */)
(register Lisp_Object object)
{
  if (MARKERP (object) || INTEGERP (object))
    return Qt;
  return Qnil;
}

DEFUN ("natnump", Fnatnump, Snatnump, 1, 1, 0,
       doc: /* Return t if OBJECT is a nonnegative integer.  */
       attributes: const)
(Lisp_Object object)
{
  return ((FIXNUMP (object)
             ? 0 <= XFIXNUM (object)
             : BIGNUMP (object) && 0 <= mpz_sgn (*xbignum_val (object)))
            ? Qt
            : Qnil);
}

DEFUN ("numberp", Fnumberp, Snumberp, 1, 1, 0,
       doc: /* Return t if OBJECT is a number (floating point or integer).  */
       attributes: const)
(Lisp_Object object)
{
  if (NUMBERP (object))
    return Qt;
  else
    return Qnil;
}

DEFUN ("number-or-marker-p", Fnumber_or_marker_p,
       Snumber_or_marker_p, 1, 1, 0,
       doc: /* Return t if OBJECT is a number or a marker.  */)
(Lisp_Object object)
{
  if (NUMBERP (object) || MARKERP (object))
    return Qt;
  return Qnil;
}

DEFUN ("floatp", Ffloatp, Sfloatp, 1, 1, 0,
       doc: /* Return t if OBJECT is a floating point number.  */
       attributes: const)
(Lisp_Object object)
{
  if (FLOATP (object))
    return Qt;
  return Qnil;
}

DEFUN ("threadp", Fthreadp, Sthreadp, 1, 1, 0,
       doc: /* Return t if OBJECT is a thread.  */)
(Lisp_Object object)
{
  if (THREADP (object))
    return Qt;
  return Qnil;
}

DEFUN ("mutexp", Fmutexp, Smutexp, 1, 1, 0,
       doc: /* Return t if OBJECT is a mutex.  */)
(Lisp_Object object)
{
  if (MUTEXP (object))
    return Qt;
  return Qnil;
}

DEFUN ("condition-variable-p", Fcondition_variable_p, Scondition_variable_p,
       1, 1, 0,
       doc: /* Return t if OBJECT is a condition variable.  */)
(Lisp_Object object)
{
  if (CONDVARP (object))
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

DEFUN ("indirect-function", Findirect_function, Sindirect_function, 1, 2, 0,
       doc: /* Return the function at the end of OBJECT's function chain.
If OBJECT is not a symbol, just return it.  Otherwise, follow all
function indirections to find the final function binding and return it.  */)
(Lisp_Object object, Lisp_Object noerror)
{
  UNUSED (noerror);
  return indirect_function (object);
}

DEFUN ("aref", Faref, Saref, 2, 2, 0,
       doc: /* Return the element of ARRAY at index IDX.
ARRAY may be a vector, a string, a char-table, a bool-vector, a record,
or a byte-code object.  IDX starts at 0.  */)
(register Lisp_Object array, Lisp_Object idx)
{
  register EMACS_INT idxval;

  CHECK_FIXNUM (idx);
  idxval = XFIXNUM (idx);
  if (STRINGP (array))
    {
      int c;
      ptrdiff_t idxval_byte;

      if (idxval < 0 || idxval >= SCHARS (array))
        args_out_of_range (array, idx);
      if (!STRING_MULTIBYTE (array))
        return make_fixnum ((unsigned char) SREF (array, idxval));
      idxval_byte = string_char_to_byte (array, idxval);

      c = STRING_CHAR (SDATA (array) + idxval_byte);
      return make_fixnum (c);
    }
  else if (BOOL_VECTOR_P (array))
    {
      TODO;
    }
  else if (CHAR_TABLE_P (array))
    {
      CHECK_CHARACTER (idx);
      return CHAR_TABLE_REF (array, idxval);
    }
  else
    {
      ptrdiff_t size = 0;
      if (VECTORP (array))
        size = ASIZE (array);
      else if (CLOSUREP (array) || RECORDP (array))
        size = PVSIZE (array);
      else
        wrong_type_argument (Qarrayp, array);

      if (idxval < 0 || idxval >= size)
        args_out_of_range (array, idx);
      return AREF (array, idxval);
    }
}

DEFUN ("aset", Faset, Saset, 3, 3, 0,
       doc: /* Store into the element of ARRAY at index IDX the value NEWELT.
Return NEWELT.  ARRAY may be a vector, a string, a char-table or a
bool-vector.  IDX starts at 0.  */)
(register Lisp_Object array, Lisp_Object idx, Lisp_Object newelt)
{
  register EMACS_INT idxval;

  CHECK_FIXNUM (idx);
  idxval = XFIXNUM (idx);
  if (!RECORDP (array))
    CHECK_ARRAY (array, Qarrayp);

  if (VECTORP (array))
    {
      CHECK_IMPURE (array, XVECTOR (array));
      if (idxval < 0 || idxval >= ASIZE (array))
        args_out_of_range (array, idx);
      ASET (array, idxval, newelt);
    }
  else if (BOOL_VECTOR_P (array))
    {
      TODO;
    }
  else if (CHAR_TABLE_P (array))
    {
      CHECK_CHARACTER (idx);
      CHAR_TABLE_SET (array, idxval, newelt);
    }
  else if (RECORDP (array))
    {
      CHECK_IMPURE (array, XVECTOR (array));
      if (idxval < 0 || idxval >= PVSIZE (array))
        args_out_of_range (array, idx);
      ASET (array, idxval, newelt);
    }
  else
    {
      CHECK_IMPURE (array, XSTRING (array));
      if (idxval < 0 || idxval >= SCHARS (array))
        args_out_of_range (array, idx);
      CHECK_CHARACTER (newelt);
      int c = XFIXNAT (newelt);
      ptrdiff_t idxval_byte;
      int prev_bytes;
      unsigned char workbuf[MAX_MULTIBYTE_LENGTH], *p0 = workbuf, *p1;

      if (STRING_MULTIBYTE (array))
        {
          idxval_byte = string_char_to_byte (array, idxval);
          p1 = SDATA (array) + idxval_byte;
          prev_bytes = BYTES_BY_CHAR_HEAD (*p1);
        }
      else if (SINGLE_BYTE_CHAR_P (c))
        {
          SSET (array, idxval, c);
          return newelt;
        }
      else
        {
          for (ptrdiff_t i = SBYTES (array) - 1; i >= 0; i--)
            if (!ASCII_CHAR_P (SREF (array, i)))
              args_out_of_range (array, newelt);
          STRING_SET_MULTIBYTE (array);
          idxval_byte = idxval;
          p1 = SDATA (array) + idxval_byte;
          prev_bytes = 1;
        }

      int new_bytes = CHAR_STRING (c, p0);
      if (prev_bytes != new_bytes)
        TODO; // p1 = resize_string_data (array, idxval_byte, prev_bytes,
              // new_bytes);

      do
        *p1++ = *p0++;
      while (--new_bytes != 0);
    }

  return newelt;
}

static Lisp_Object
check_integer_coerce_marker (Lisp_Object x)
{
  if (MARKERP (x))
    TODO; // return make_fixnum (marker_position (x));
  CHECK_TYPE (INTEGERP (x), Qinteger_or_marker_p, x);
  return x;
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
        i2 = mpz_cmp_d (*xbignum_val (num2), f1);
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
        i2 = mpz_sgn (*xbignum_val (num2));
    }
  else if (FLOATP (num2))
    {
      double f2 = XFLOAT_DATA (num2);
      if (isnan (f2))
        lt = eq = gt = false;
      else
        i1 = mpz_cmp_d (*xbignum_val (num1), f2);
    }
  else if (FIXNUMP (num2))
    i1 = mpz_sgn (*xbignum_val (num1));
  else
    i1 = mpz_cmp (*xbignum_val (num1), *xbignum_val (num2));

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
arithcompare_driver (ptrdiff_t nargs, Lisp_Object *args,
                     enum Arith_Comparison comparison)
{
  for (ptrdiff_t i = 1; i < nargs; i++)
    if (NILP (arithcompare (args[i - 1], args[i], comparison)))
      return Qnil;
  return Qt;
}

DEFUN ("=", Feqlsign, Seqlsign, 1, MANY, 0,
       doc: /* Return t if args, all numbers or markers, are equal.
usage: (= NUMBER-OR-MARKER &rest NUMBERS-OR-MARKERS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  return arithcompare_driver (nargs, args, ARITH_EQUAL);
}

DEFUN ("<", Flss, Slss, 1, MANY, 0,
       doc: /* Return t if each arg (a number or marker), is less than the next arg.
usage: (< NUMBER-OR-MARKER &rest NUMBERS-OR-MARKERS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  if (nargs == 2 && FIXNUMP (args[0]) && FIXNUMP (args[1]))
    return XFIXNUM (args[0]) < XFIXNUM (args[1]) ? Qt : Qnil;

  return arithcompare_driver (nargs, args, ARITH_LESS);
}

DEFUN (">", Fgtr, Sgtr, 1, MANY, 0,
       doc: /* Return t if each arg (a number or marker) is greater than the next arg.
usage: (> NUMBER-OR-MARKER &rest NUMBERS-OR-MARKERS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  if (nargs == 2 && FIXNUMP (args[0]) && FIXNUMP (args[1]))
    return XFIXNUM (args[0]) > XFIXNUM (args[1]) ? Qt : Qnil;

  return arithcompare_driver (nargs, args, ARITH_GRTR);
}

DEFUN ("<=", Fleq, Sleq, 1, MANY, 0,
       doc: /* Return t if each arg (a number or marker) is less than or equal to the next.
usage: (<= NUMBER-OR-MARKER &rest NUMBERS-OR-MARKERS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  if (nargs == 2 && FIXNUMP (args[0]) && FIXNUMP (args[1]))
    return XFIXNUM (args[0]) <= XFIXNUM (args[1]) ? Qt : Qnil;

  return arithcompare_driver (nargs, args, ARITH_LESS_OR_EQUAL);
}

DEFUN (">=", Fgeq, Sgeq, 1, MANY, 0,
       doc: /* Return t if each arg (a number or marker) is greater than or equal to the next.
usage: (>= NUMBER-OR-MARKER &rest NUMBERS-OR-MARKERS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  if (nargs == 2 && FIXNUMP (args[0]) && FIXNUMP (args[1]))
    return XFIXNUM (args[0]) >= XFIXNUM (args[1]) ? Qt : Qnil;

  return arithcompare_driver (nargs, args, ARITH_GRTR_OR_EQUAL);
}

DEFUN ("/=", Fneq, Sneq, 2, 2, 0,
       doc: /* Return t if first arg is not equal to second arg.  Both must be numbers or markers.  */)
(register Lisp_Object num1, Lisp_Object num2)
{
  return arithcompare (num1, num2, ARITH_NOTEQUAL);
}

uintmax_t
cons_to_unsigned (Lisp_Object c, uintmax_t max)
{
  bool valid = false;
  uintmax_t val UNINIT;

  if (FLOATP (c))
    {
      double d = XFLOAT_DATA (c);
      if (d >= 0 && d < 1.0 + max)
        {
          val = d;
          valid = val == d;
        }
    }
  else
    {
      Lisp_Object hi = CONSP (c) ? XCAR (c) : c;
      valid = INTEGERP (hi) && integer_to_uintmax (hi, &val);

      if (valid && CONSP (c))
        {
          uintmax_t top = val;
          Lisp_Object rest = XCDR (c);
          if (top <= UINTMAX_MAX >> 24 >> 16 && CONSP (rest)
              && FIXNATP (XCAR (rest)) && XFIXNAT (XCAR (rest)) < 1 << 24
              && FIXNATP (XCDR (rest)) && XFIXNAT (XCDR (rest)) < 1 << 16)
            {
              uintmax_t mid = XFIXNAT (XCAR (rest));
              val = top << 24 << 16 | mid << 16 | XFIXNAT (XCDR (rest));
            }
          else
            {
              valid = top <= UINTMAX_MAX >> 16;
              if (valid)
                {
                  if (CONSP (rest))
                    rest = XCAR (rest);
                  valid = FIXNATP (rest) && XFIXNAT (rest) < 1 << 16;
                  if (valid)
                    val = top << 16 | XFIXNAT (rest);
                }
            }
        }
    }

  if (!(valid && val <= max))
    error ("Not an in-range integer, integral float, or cons of integers");
  return val;
}

char *
fixnum_to_string (EMACS_INT number, char *buffer, char *end)
{
  EMACS_INT x = number;
  bool negative = x < 0;
  if (negative)
    x = -x;
  char *p = end;
  do
    {
      eassume (p > buffer && p - 1 < end);
      *--p = '0' + x % 10;
      x /= 10;
    }
  while (x);
  if (negative)
    *--p = '-';
  return p;
}

DEFUN ("string-to-number", Fstring_to_number, Sstring_to_number, 1, 2, 0,
       doc: /* Parse STRING as a decimal number and return the number.
Ignore leading spaces and tabs, and all trailing chars.  Return 0 if
STRING cannot be parsed as an integer or floating point number.

If BASE, interpret STRING as a number in that base.  If BASE isn't
present, base 10 is used.  BASE must be between 2 and 16 (inclusive).
If the base used is not 10, STRING is always parsed as an integer.  */)
(register Lisp_Object string, Lisp_Object base)
{
  int b;

  CHECK_STRING (string);

  if (NILP (base))
    b = 10;
  else
    {
      CHECK_FIXNUM (base);
      if (!(XFIXNUM (base) >= 2 && XFIXNUM (base) <= 16))
        xsignal1 (Qargs_out_of_range, base);
      b = XFIXNUM (base);
    }

  char *p = SSDATA (string);
  while (*p == ' ' || *p == '\t')
    p++;

  Lisp_Object val = string_to_number (p, b, 0);
  return ((IEEE_FLOATING_POINT ? NILP (val) : !NUMBERP (val)) ? make_fixnum (0)
                                                              : val);
}

enum arithop
{
  Aadd,
  Asub,
  Amult,
  Adiv,
  Alogand,
  Alogior,
  Alogxor
};

static Lisp_Object
arith_driver (enum arithop code, ptrdiff_t nargs, Lisp_Object *args,
              Lisp_Object val)
{
  eassume (2 <= nargs);

  ptrdiff_t argnum = 0;
  intmax_t accum = XFIXNUM_RAW (val);

  if (FIXNUMP (val))
    while (true)
      {
        argnum++;
        if (argnum == nargs)
          return make_int (accum);
        val = check_number_coerce_marker (args[argnum]);

        intmax_t next;
        if (!(INTEGERP (val) && integer_to_intmax (val, &next)))
          break;

        bool overflow;
        intmax_t a;
        switch (code)
          {
          case Aadd:
            overflow = ckd_add (&a, accum, next);
            break;
          case Amult:
            overflow = ckd_mul (&a, accum, next);
            break;
          case Asub:
            overflow = ckd_sub (&a, accum, next);
            break;
          case Adiv:
            if (next == 0)
              xsignal0 (Qarith_error);
            accum /= next;
            continue;
          case Alogand:
            accum &= next;
            continue;
          case Alogior:
            accum |= next;
            continue;
          case Alogxor:
            accum ^= next;
            continue;
          default:
            eassume (false);
          }
        if (overflow)
          break;
        accum = a;
      }

  TODO; // return (FLOATP (val)
        //  ? float_arith_driver (code, nargs, args, argnum, accum, val)
        //  : bignum_arith_driver (code, nargs, args, argnum, accum, val));
}

DEFUN ("+", Fplus, Splus, 0, MANY, 0,
       doc: /* Return sum of any number of arguments, which are numbers or markers.
usage: (+ &rest NUMBERS-OR-MARKERS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  if (nargs == 0)
    return make_fixnum (0);
  Lisp_Object a = check_number_coerce_marker (args[0]);
  return nargs == 1 ? a : arith_driver (Aadd, nargs, args, a);
}

DEFUN ("-", Fminus, Sminus, 0, MANY, 0,
       doc: /* Negate number or subtract numbers or markers and return the result.
With one arg, negates it.  With more than one arg,
subtracts all but the first from the first.
usage: (- &optional NUMBER-OR-MARKER &rest MORE-NUMBERS-OR-MARKERS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  if (nargs == 0)
    return make_fixnum (0);
  Lisp_Object a = check_number_coerce_marker (args[0]);
  if (nargs == 1)
    {
      if (FIXNUMP (a))
        return make_int (-XFIXNUM (a));
      if (FLOATP (a))
        return make_float (-XFLOAT_DATA (a));
      mpz_neg (mpz[0], *xbignum_val (a));
      return make_integer_mpz ();
    }
  return arith_driver (Asub, nargs, args, a);
}

DEFUN ("*", Ftimes, Stimes, 0, MANY, 0,
       doc: /* Return product of any number of arguments, which are numbers or markers.
usage: (* &rest NUMBERS-OR-MARKERS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  if (nargs == 0)
    return make_fixnum (1);
  Lisp_Object a = check_number_coerce_marker (args[0]);
  return nargs == 1 ? a : arith_driver (Amult, nargs, args, a);
}

DEFUN ("/", Fquo, Squo, 1, MANY, 0,
       doc: /* Divide number by divisors and return the result.
With two or more arguments, return first argument divided by the rest.
With one argument, return 1 divided by the argument.
The arguments must be numbers or markers.
usage: (/ NUMBER &rest DIVISORS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  Lisp_Object a = check_number_coerce_marker (args[0]);
  if (nargs == 1)
    {
      if (FIXNUMP (a))
        {
          if (XFIXNUM (a) == 0)
            xsignal0 (Qarith_error);
          return make_fixnum (1 / XFIXNUM (a));
        }
      if (FLOATP (a))
        {
          if (!IEEE_FLOATING_POINT && XFLOAT_DATA (a) == 0)
            xsignal0 (Qarith_error);
          return make_float (1 / XFLOAT_DATA (a));
        }
      return make_fixnum (0);
    }

  for (ptrdiff_t argnum = 2; argnum < nargs; argnum++)
    if (FLOATP (args[argnum]))
      TODO; // return floatop_arith_driver (Adiv, nargs, args, 0, 0, XFLOATINT
            // (a));
  return arith_driver (Adiv, nargs, args, a);
}

static Lisp_Object
integer_remainder (Lisp_Object num, Lisp_Object den, bool modulo)
{
  if (FIXNUMP (den))
    {
      EMACS_INT d = XFIXNUM (den);
      if (d == 0)
        xsignal0 (Qarith_error);

      EMACS_INT r;
      bool have_r = false;
      if (FIXNUMP (num))
        {
          r = XFIXNUM (num) % d;
          have_r = true;
        }
      else if ((unsigned long) eabs (d) <= ULONG_MAX)
        {
          TODO;
        }

      if (have_r)
        {
          if (modulo && (d < 0 ? r > 0 : r < 0))
            r += d;

          return make_fixnum (r);
        }
    }

  TODO;
}

DEFUN ("%", Frem, Srem, 2, 2, 0,
       doc: /* Return remainder of X divided by Y.
Both must be integers or markers.  */)
(Lisp_Object x, Lisp_Object y)
{
  x = check_integer_coerce_marker (x);
  y = check_integer_coerce_marker (y);
  return integer_remainder (x, y, false);
}

DEFUN ("mod", Fmod, Smod, 2, 2, 0,
       doc: /* Return X modulo Y.
The result falls between zero (inclusive) and Y (exclusive).
Both X and Y must be numbers or markers.  */)
(Lisp_Object x, Lisp_Object y)
{
  x = check_number_coerce_marker (x);
  y = check_number_coerce_marker (y);
  if (FLOATP (x) || FLOATP (y))
    TODO; // return fmod_float (x, y);
  return integer_remainder (x, y, true);
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

DEFUN ("1+", Fadd1, Sadd1, 1, 1, 0,
       doc: /* Return NUMBER plus one.  NUMBER may be a number or a marker.
Markers are converted to integers.  */)
(Lisp_Object number)
{
  number = check_number_coerce_marker (number);

  if (FIXNUMP (number))
    return make_int (XFIXNUM (number) + 1);
  if (FLOATP (number))
    return (make_float (1.0 + XFLOAT_DATA (number)));
  mpz_add_ui (mpz[0], *xbignum_val (number), 1);
  return make_integer_mpz ();
}

DEFUN ("1-", Fsub1, Ssub1, 1, 1, 0,
       doc: /* Return NUMBER minus one.  NUMBER may be a number or a marker.
Markers are converted to integers.  */)
(Lisp_Object number)
{
  number = check_number_coerce_marker (number);

  if (FIXNUMP (number))
    return make_int (XFIXNUM (number) - 1);
  if (FLOATP (number))
    return (make_float (-1.0 + XFLOAT_DATA (number)));
  mpz_sub_ui (mpz[0], *xbignum_val (number), 1);
  return make_integer_mpz ();
}

void
syms_of_data (void)
{
  DEFSYM (Qquote, "quote");
  DEFSYM (Qtop_level, "top-level");
  DEFSYM (Qerror_conditions, "error-conditions");
  DEFSYM (Qerror_message, "error-message");

  DEFSYM (Qcar, "car");
  DEFSYM (Qcdr, "cdr");
  DEFSYM (Qaref, "aref");
  DEFSYM (Qaset, "aset");

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
  DEFSYM (Qfixnump, "fixnump");
  DEFSYM (Qchar_table_p, "char-table-p");
  DEFSYM (Qinteger_or_marker_p, "integer-or-marker-p");
  DEFSYM (Qsequencep, "sequencep");
  DEFSYM (Qbyte_code_function_p, "byte-code-function-p");
  DEFSYM (Qbool_vector_p, "bool-vector-p");
  DEFSYM (Qchar_or_string_p, "char-or-string-p");

  DEFSYM (Qvoid_function, "void-function");
  DEFSYM (Qwrong_type_argument, "wrong-type-argument");
  DEFSYM (Qwrong_number_of_arguments, "wrong-number-of-arguments");
  DEFSYM (Qvoid_variable, "void-variable");
  DEFSYM (Qsetting_constant, "setting-constant");
  DEFSYM (Qcyclic_function_indirection, "cyclic-function-indirection");
  DEFSYM (Qinvalid_function, "invalid-function");
  DEFSYM (Qargs_out_of_range, "args-out-of-range");
  DEFSYM (Qarith_error, "arith-error");
  DEFSYM (Qoverflow_error, "overflow-error");
  DEFSYM (Qrange_error, "range-error");
  DEFSYM (Qcyclic_variable_indirection, "cyclic-variable-indirection");
  DEFSYM (Qexcessive_lisp_nesting, "excessive-lisp-nesting");
  DEFSYM (Qrecursion_error, "recursion-error");

  Lisp_Object error_tail = Fcons (Qerror, Qnil);

  Fput (Qerror, Qerror_conditions, error_tail);
  Fput (Qerror, Qerror_message, build_pure_c_string ("error"));

#define PUT_ERROR(sym, tail, msg)                       \
  Fput (sym, Qerror_conditions, pure_cons (sym, tail)); \
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
  PUT_ERROR (Qargs_out_of_range, error_tail, "Args out of range");

  Lisp_Object arith_tail = pure_cons (Qarith_error, error_tail);
  Fput (Qarith_error, Qerror_conditions, arith_tail);
  Fput (Qarith_error, Qerror_message, build_pure_c_string ("Arithmetic error"));

  PUT_ERROR (Qrange_error, arith_tail, "Arithmetic range error");
  PUT_ERROR (Qoverflow_error, Fcons (Qrange_error, arith_tail),
             "Arithmetic overflow error");

  PUT_ERROR (Qcyclic_variable_indirection, error_tail,
             "Symbol's chain of variable indirections contains a loop");

  Lisp_Object recursion_tail = pure_cons (Qrecursion_error, error_tail);
  Fput (Qrecursion_error, Qerror_conditions, recursion_tail);
  Fput (Qrecursion_error, Qerror_message,
        build_pure_c_string ("Excessive recursive calling error"));

  PUT_ERROR (Qexcessive_lisp_nesting, recursion_tail,
             "Lisp nesting exceeds `max-lisp-eval-depth'");

  defsubr (&Ssymbol_value);
  defsubr (&Sbare_symbol);
  defsubr (&Sdefault_boundp);
  defsubr (&Sdefault_value);
  defsubr (&Sset_default);
  defsubr (&Smake_variable_buffer_local);
  defsubr (&Scar);
  defsubr (&Scdr);
  defsubr (&Scar_safe);
  defsubr (&Scdr_safe);
  defsubr (&Slocal_variable_p);
  defsubr (&Slocal_variable_if_set_p);
  defsubr (&Sset);
  defsubr (&Sfset);
  defsubr (&Sdefalias);
  defsubr (&Ssetcar);
  defsubr (&Ssetcdr);
  defsubr (&Sboundp);
  defsubr (&Sfboundp);
  defsubr (&Sfmakunbound);
  defsubr (&Ssymbol_name);
  defsubr (&Ssymbol_function);
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
  defsubr (&Smultibyte_string_p);
  defsubr (&Schar_table_p);
  defsubr (&Svector_or_char_table_p);
  defsubr (&Sbool_vector_p);
  defsubr (&Sarrayp);
  defsubr (&Ssequencep);
  defsubr (&Sbufferp);
  defsubr (&Smarkerp);
  defsubr (&Ssubrp);
  defsubr (&Sclosurep);
  defsubr (&Sbyte_code_function_p);
  defsubr (&Sinterpreted_function_p);
  defsubr (&Smodule_function_p);
  defsubr (&Schar_or_string_p);
  defsubr (&Sintegerp);
  defsubr (&Sinteger_or_marker_p);
  defsubr (&Snatnump);
  defsubr (&Snumberp);
  defsubr (&Snumber_or_marker_p);
  defsubr (&Sfloatp);
  defsubr (&Sthreadp);
  defsubr (&Smutexp);
  defsubr (&Scondition_variable_p);
  defsubr (&Snull);
  defsubr (&Sindirect_function);
  defsubr (&Saref);
  defsubr (&Saset);
  defsubr (&Seq);
  defsubr (&Seqlsign);
  defsubr (&Slss);
  defsubr (&Sgtr);
  defsubr (&Sleq);
  defsubr (&Sgeq);
  defsubr (&Sneq);
  defsubr (&Sstring_to_number);
  defsubr (&Splus);
  defsubr (&Sminus);
  defsubr (&Stimes);
  defsubr (&Squo);
  defsubr (&Srem);
  defsubr (&Smod);
  defsubr (&Smax);
  defsubr (&Smin);
  defsubr (&Sadd1);
  defsubr (&Ssub1);
}
