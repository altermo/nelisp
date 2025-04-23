#include <stdlib.h>

#include "lisp.h"

#define CACHEABLE
#define clobbered_eassert(E) eassert (sizeof (E) != 0)

static Lisp_Object funcall_lambda (Lisp_Object, ptrdiff_t, Lisp_Object *);
static Lisp_Object apply_lambda (Lisp_Object, Lisp_Object, specpdl_ref);

static Lisp_Object
specpdl_symbol (union specbinding *pdl)
{
  eassert (pdl->kind >= SPECPDL_LET);
  return pdl->let.symbol;
}
static enum specbind_tag
specpdl_kind (union specbinding *pdl)
{
  eassert (pdl->kind >= SPECPDL_LET);
  return pdl->let.kind;
}

static Lisp_Object
specpdl_old_value (union specbinding *pdl)
{
  eassert (pdl->kind >= SPECPDL_LET);
  return pdl->let.old_value;
}

void
grow_specpdl_allocation (void)
{
  eassert (specpdl_ptr == specpdl_end);

  specpdl_ref count = SPECPDL_INDEX ();
  ptrdiff_t max_size = PTRDIFF_MAX - 1000;
  union specbinding *pdlvec = specpdl - 1;
  ptrdiff_t size = specpdl_end - specpdl;
  ptrdiff_t pdlvecsize = size + 1;
  eassert (max_size > size);
  pdlvec = xpalloc (pdlvec, &pdlvecsize, 1, max_size + 1, sizeof *specpdl);
  specpdl = pdlvec + 1;
  specpdl_end = specpdl + pdlvecsize - 1;
  specpdl_ptr = specpdl_ref_to_ptr (count);
}
INLINE void
grow_specpdl (void)
{
  specpdl_ptr++;
  if (specpdl_ptr == specpdl_end)
    grow_specpdl_allocation ();
}
void
record_unwind_protect_ptr (void (*function) (void *), void *arg)
{
  specpdl_ptr->unwind_ptr.kind = SPECPDL_UNWIND_PTR;
  specpdl_ptr->unwind_ptr.func = function;
  specpdl_ptr->unwind_ptr.arg = arg;
  specpdl_ptr->unwind_ptr.mark = NULL;
  grow_specpdl ();
}
void
record_unwind_protect_intmax (void (*function) (intmax_t), intmax_t arg)
{
  specpdl_ptr->unwind_intmax.kind = SPECPDL_UNWIND_INTMAX;
  specpdl_ptr->unwind_intmax.func = function;
  specpdl_ptr->unwind_intmax.arg = arg;
  grow_specpdl ();
}
void
record_unwind_protect (void (*function) (Lisp_Object), Lisp_Object arg)
{
  specpdl_ptr->unwind.kind = SPECPDL_UNWIND;
  specpdl_ptr->unwind.func = function;
  specpdl_ptr->unwind.arg = arg;
  specpdl_ptr->unwind.eval_depth = lisp_eval_depth;
  grow_specpdl ();
}
void
record_unwind_protect_array (Lisp_Object *array, ptrdiff_t nelts)
{
  specpdl_ptr->unwind_array.kind = SPECPDL_UNWIND_ARRAY;
  specpdl_ptr->unwind_array.array = array;
  specpdl_ptr->unwind_array.nelts = nelts;
  grow_specpdl ();
}

specpdl_ref
record_in_backtrace (Lisp_Object function, Lisp_Object *args, ptrdiff_t nargs)
{
  specpdl_ref count = SPECPDL_INDEX ();

  eassert (nargs >= UNEVALLED);
  specpdl_ptr->bt.kind = SPECPDL_BACKTRACE;
  specpdl_ptr->bt.debug_on_exit = false;
  specpdl_ptr->bt.function = function;
  specpdl_ptr->bt.args = args;
  specpdl_ptr->bt.nargs = nargs;
  grow_specpdl ();

  return count;
}

void
record_unwind_protect_int (void (*function) (int), int arg)
{
  specpdl_ptr->unwind_int.kind = SPECPDL_UNWIND_INT;
  specpdl_ptr->unwind_int.func = function;
  specpdl_ptr->unwind_int.arg = arg;
  grow_specpdl ();
}

void
set_unwind_protect_ptr (specpdl_ref count, void (*func) (void *), void *arg)
{
  union specbinding *p = specpdl_ref_to_ptr (count);
  p->unwind_ptr.kind = SPECPDL_UNWIND_PTR;
  p->unwind_ptr.func = func;
  p->unwind_ptr.arg = arg;
  p->unwind_ptr.mark = NULL;
}

static void
set_backtrace_args (union specbinding *pdl, Lisp_Object *args, ptrdiff_t nargs)
{
  eassert (pdl->kind == SPECPDL_BACKTRACE);
  pdl->bt.args = args;
  pdl->bt.nargs = nargs;
}

static void
do_one_unbind (union specbinding *this_binding, bool unwinding,
               enum Set_Internal_Bind bindflag)
{
  KBOARD *kbdwhere = NULL;
  eassert (unwinding || this_binding->kind >= SPECPDL_LET);
  switch (this_binding->kind)
    {
    case SPECPDL_UNWIND:
      lisp_eval_depth = this_binding->unwind.eval_depth;
      this_binding->unwind.func (this_binding->unwind.arg);
      break;
    case SPECPDL_UNWIND_ARRAY:
      TODO;
    case SPECPDL_UNWIND_PTR:
      this_binding->unwind_ptr.func (this_binding->unwind_ptr.arg);
      break;
    case SPECPDL_UNWIND_INT:
      TODO;
    case SPECPDL_UNWIND_INTMAX:
      this_binding->unwind_intmax.func (this_binding->unwind_intmax.arg);
      break;
    case SPECPDL_UNWIND_VOID:
      TODO;
    case SPECPDL_UNWIND_EXCURSION:
      TODO;
    case SPECPDL_BACKTRACE:
    case SPECPDL_NOP:
      break;
    case SPECPDL_LET:
      {
        Lisp_Object sym = specpdl_symbol (this_binding);
        if (SYMBOLP (sym) && XSYMBOL (sym)->u.s.redirect == SYMBOL_PLAINVAL)
          {
            if (XSYMBOL (sym)->u.s.trapped_write == SYMBOL_UNTRAPPED_WRITE)
              SET_SYMBOL_VAL (XSYMBOL (sym), specpdl_old_value (this_binding));
            else
              set_internal (sym, specpdl_old_value (this_binding), Qnil,
                            bindflag);
            break;
          }
        if (SYMBOLP (sym) && XSYMBOL (sym)->u.s.redirect == SYMBOL_FORWARDED
            && XSYMBOL (sym)->u.s.trapped_write == SYMBOL_UNTRAPPED_WRITE)
          {
            if (BUFFER_OBJFWDP (SYMBOL_FWD (XSYMBOL (sym)))
                || KBOARD_OBJFWDP (SYMBOL_FWD (XSYMBOL (sym))))
              {
                TODO;
              }
            set_internal (sym, specpdl_old_value (this_binding), Qnil,
                          bindflag);
            break;
          }
        TODO;
      }
    case SPECPDL_LET_DEFAULT:
      set_default_internal (specpdl_symbol (this_binding),
                            specpdl_old_value (this_binding), bindflag,
                            kbdwhere);
      break;
    case SPECPDL_LET_LOCAL:
      TODO;
    }
}
Lisp_Object
unbind_to (specpdl_ref count, Lisp_Object value)
{
#if TODO_NELISP_LATER_AND
  Lisp_Object quitf = Vquit_flag;

  Vquit_flag = Qnil;
#endif

  while (specpdl_ptr != specpdl_ref_to_ptr (count))
    {
      union specbinding this_binding;
      this_binding = *--specpdl_ptr;

      do_one_unbind (&this_binding, true, SET_INTERNAL_UNBIND);
    }

#if TODO_NELISP_LATER_AND
  if (NILP (Vquit_flag) && !NILP (quitf))
    Vquit_flag = quitf;
#endif

  return value;
}

void
mark_specpdl (void)
{
  union specbinding *first = specpdl;
  union specbinding *ptr = specpdl_ptr;
  union specbinding *pdl;
  for (pdl = first; pdl != ptr; pdl++)
    {
      switch (pdl->kind)
        {
        case SPECPDL_UNWIND:
          TODO;
        case SPECPDL_UNWIND_ARRAY:
          TODO;
        case SPECPDL_UNWIND_EXCURSION:
          TODO;
        case SPECPDL_BACKTRACE:
          TODO;
        case SPECPDL_LET_DEFAULT:
        case SPECPDL_LET_LOCAL:
          TODO;
        case SPECPDL_LET:
          TODO;
        case SPECPDL_UNWIND_PTR:
          TODO;
        case SPECPDL_UNWIND_INT:
        case SPECPDL_UNWIND_INTMAX:
        case SPECPDL_UNWIND_VOID:
        case SPECPDL_NOP:
          break;
        default:
          emacs_abort ();
        }
    }
}

struct handler *
push_handler_nosignal (Lisp_Object tag_ch_val, enum handlertype handlertype)
{
  struct handler *CACHEABLE c = handlerlist->nextfree;
  if (!c)
    {
      c = malloc (sizeof *c);
      if (!c)
        return c;
      c->nextfree = NULL;
      handlerlist->nextfree = c;
    }
  c->type = handlertype;
  c->tag_or_ch = tag_ch_val;
  c->val = Qnil;
  c->next = handlerlist;
  c->f_lisp_eval_depth = lisp_eval_depth;
  c->pdlcount = SPECPDL_INDEX ();
  TODO_NELISP_LATER;
  // c->act_rec = get_act_rec (current_thread);
  // c->poll_suppress_count = poll_suppress_count;
  // c->interrupt_input_blocked = interrupt_input_blocked;
  handlerlist = c;
  return c;
}
struct handler *
push_handler (Lisp_Object tag_ch_val, enum handlertype handlertype)
{
  struct handler *c = push_handler_nosignal (tag_ch_val, handlertype);
  if (!c)
    memory_full (sizeof *c);
  return c;
}
Lisp_Object
internal_catch (Lisp_Object tag, Lisp_Object (*func) (Lisp_Object),
                Lisp_Object arg)
{
  struct handler *c = push_handler (tag, CATCHER);
  if (!sys_setjmp (c->jmp))
    {
      Lisp_Object val = func (arg);
      eassert (handlerlist == c);
      handlerlist = c->next;
      return val;
    }
  else
    {
      Lisp_Object val = handlerlist->val;
      clobbered_eassert (handlerlist == c);
      handlerlist = handlerlist->next;
      return val;
    }
}

Lisp_Object
internal_condition_case (Lisp_Object (*bfun) (void), Lisp_Object handlers,
                         Lisp_Object (*hfun) (Lisp_Object))
{
  struct handler *c = push_handler (handlers, CONDITION_CASE);
  if (sys_setjmp (c->jmp))
    {
      Lisp_Object val = handlerlist->val;
      clobbered_eassert (handlerlist == c);
      handlerlist = handlerlist->next;
      return hfun (val);
    }
  else
    {
      Lisp_Object val = bfun ();
      eassert (handlerlist == c);
      handlerlist = c->next;
      return val;
    }
}

static AVOID
unwind_to_catch (struct handler *catch, enum nonlocal_exit type,
                 Lisp_Object value)
{
  bool last_time;

  eassert (catch->next);

  catch->nonlocal_exit = type;
  catch->val = value;

  // set_poll_suppress_count (catch->poll_suppress_count);
  // unblock_input_to (catch->interrupt_input_blocked);

  do
    {
      unbind_to (handlerlist->pdlcount, Qnil);
      last_time = handlerlist == catch;
      if (!last_time)
        handlerlist = handlerlist->next;
    }
  while (!last_time);

  eassert (handlerlist == catch);

  lisp_eval_depth = catch->f_lisp_eval_depth;
  // set_act_rec (current_thread, catch->act_rec);

  sys_longjmp (catch->jmp, 1);
}

static void
do_specbind (struct Lisp_Symbol *sym, union specbinding *bind,
             Lisp_Object value, enum Set_Internal_Bind bindflag)
{
  UNUSED (bind);
  UNUSED (bindflag);
  switch (sym->u.s.redirect)
    {
    case SYMBOL_PLAINVAL:
      if (!sym->u.s.trapped_write)
        SET_SYMBOL_VAL (sym, value);
      else
        TODO;
      break;
    case SYMBOL_FORWARDED:
      if (BUFFER_OBJFWDP (SYMBOL_FWD (sym))
          && specpdl_kind (bind) == SPECPDL_LET_DEFAULT)
        {
          TODO;
          return;
        }
      FALLTHROUGH;
    case SYMBOL_LOCALIZED:
      set_internal (specpdl_symbol (bind), value, Qnil, bindflag);
      break;
    default:
      emacs_abort ();
    }
}
void
specbind (Lisp_Object symbol, Lisp_Object value)
{
  struct Lisp_Symbol *sym = XBARE_SYMBOL (symbol);
  switch (sym->u.s.redirect)
    {
    case SYMBOL_VARALIAS:
      TODO;
    case SYMBOL_PLAINVAL:
      specpdl_ptr->let.kind = SPECPDL_LET;
      specpdl_ptr->let.symbol = symbol;
      specpdl_ptr->let.old_value = SYMBOL_VAL (sym);
      specpdl_ptr->let.where.kbd = NULL;
      break;
    case SYMBOL_LOCALIZED:
    case SYMBOL_FORWARDED:
      {
        Lisp_Object ovalue = find_symbol_value (symbol);
        specpdl_ptr->let.kind = SPECPDL_LET_LOCAL;
        specpdl_ptr->let.symbol = symbol;
        specpdl_ptr->let.old_value = ovalue;

        eassert (sym->u.s.redirect != SYMBOL_LOCALIZED
                 || (BASE_EQ (SYMBOL_BLV (sym)->where, Fcurrent_buffer ())));

        if (sym->u.s.redirect == SYMBOL_LOCALIZED)
          {
            if (!blv_found (SYMBOL_BLV (sym)))
              specpdl_ptr->let.kind = SPECPDL_LET_DEFAULT;
          }
        else if (BUFFER_OBJFWDP (SYMBOL_FWD (sym)))
          {
            TODO;
          }
        else if (KBOARD_OBJFWDP (SYMBOL_FWD (sym)))
          {
            TODO;
          }
        else
          specpdl_ptr->let.kind = SPECPDL_LET;

        break;
      }
    default:
      emacs_abort ();
    }
  grow_specpdl ();
  do_specbind (sym, specpdl_ptr - 1, value, SET_INTERNAL_BIND);
}

static Lisp_Object
find_handler_clause (Lisp_Object handlers, Lisp_Object conditions)
{
  register Lisp_Object h;

  if (!CONSP (handlers))
    return handlers;

  for (h = handlers; CONSP (h); h = XCDR (h))
    {
      Lisp_Object handler = XCAR (h);
      if (!NILP (Fmemq (handler, conditions)) || EQ (handler, Qt))
        return handlers;
    }

  return Qnil;
}
static Lisp_Object
signal_or_quit (Lisp_Object error_symbol, Lisp_Object data, bool continuable)
{
  UNUSED (continuable);
  bool oom = NILP (error_symbol);
  Lisp_Object error = oom ? data
                      : (!SYMBOLP (error_symbol) && NILP (data))
                        ? error_symbol
                        : Fcons (error_symbol, data);
  Lisp_Object conditions;
  Lisp_Object real_error_symbol = CONSP (error) ? XCAR (error) : error_symbol;
  Lisp_Object clause = Qnil;
  struct handler *h;
  int skip;
  UNUSED (skip);

  conditions = Fget (real_error_symbol, Qerror_conditions);

  for (skip = 0, h = handlerlist; h; skip++, h = h->next)
    {
      switch (h->type)
        {
        case CATCHER_ALL:
          clause = Qt;
          break;
        case CATCHER:
          continue;
        case CONDITION_CASE:
          clause = find_handler_clause (h->tag_or_ch, conditions);
          break;
        case HANDLER_BIND:
          {
            TODO;
          }
        case SKIP_CONDITIONS:
          {
            int toskip = XFIXNUM (h->tag_or_ch);
            while (toskip-- >= 0)
              h = h->next;
            continue;
          }
        default:
          abort ();
        }
      if (!NILP (clause))
        break;
    }

  if (!NILP (clause))
    unwind_to_catch (h, NONLOCAL_EXIT_SIGNAL, error);
  TODO;
}
DEFUN ("signal", Fsignal, Ssignal, 2, 2, 0,
       doc: /* Signal an error.  Args are ERROR-SYMBOL and associated DATA.
This function does not return.

When `noninteractive' is non-nil (in particular, in batch mode), an
unhandled error calls `kill-emacs', which terminates the Emacs
session with a non-zero exit code.

An error symbol is a symbol with an `error-conditions' property
that is a list of condition names.  The symbol should be non-nil.
A handler for any of those names will get to handle this signal.
The symbol `error' should normally be one of them.

DATA should be a list.  Its elements are printed as part of the error message.
See Info anchor `(elisp)Definition of signal' for some details on how this
error message is constructed.
If the signal is handled, DATA is made available to the handler.
See also the function `condition-case'.  */
       attributes: noreturn)
(Lisp_Object error_symbol, Lisp_Object data)
{
  /* If they call us with nonsensical arguments, produce "peculiar error".  */
  if (NILP (error_symbol) && NILP (data))
    error_symbol = Qerror;
  signal_or_quit (error_symbol, data, false);
  eassume (false);
}

void
xsignal0 (Lisp_Object error_symbol)
{
  xsignal (error_symbol, Qnil);
}
void
xsignal1 (Lisp_Object error_symbol, Lisp_Object arg)
{
  xsignal (error_symbol, list1 (arg));
}
void
xsignal2 (Lisp_Object error_symbol, Lisp_Object arg1, Lisp_Object arg2)
{
  xsignal (error_symbol, list2 (arg1, arg2));
}
void
xsignal3 (Lisp_Object error_symbol, Lisp_Object arg1, Lisp_Object arg2,
          Lisp_Object arg3)
{
  xsignal (error_symbol, list3 (arg1, arg2, arg3));
}
void
signal_error (const char *s, Lisp_Object arg)
{
  if (NILP (Fproper_list_p (arg)))
    arg = list1 (arg);

  xsignal (Qerror, Fcons (build_string (s), arg));
}

void
overflow_error (void)
{
  xsignal0 (Qoverflow_error);
}

void
error (const char *m, ...)
{
  UNUSED (m);
  TODO;
}

Lisp_Object
indirect_function (Lisp_Object object)
{
  while (SYMBOLP (object) && !NILP (object))
    object = XSYMBOL (object)->u.s.function;
  return object;
}

DEFUN ("autoload", Fautoload, Sautoload, 2, 5, 0,
       doc: /* Define FUNCTION to autoload from FILE.
FUNCTION is a symbol; FILE is a file name string to pass to `load'.

Third arg DOCSTRING is documentation for the function.

Fourth arg INTERACTIVE if non-nil says function can be called
interactively.  If INTERACTIVE is a list, it is interpreted as a list
of modes the function is applicable for.

Fifth arg TYPE indicates the type of the object:
   nil or omitted says FUNCTION is a function,
   `keymap' says FUNCTION is really a keymap, and
   `macro' or t says FUNCTION is really a macro.

Third through fifth args give info about the real definition.
They default to nil.

If FUNCTION is already defined other than as an autoload,
this does nothing and returns nil.  */)
(Lisp_Object function, Lisp_Object file, Lisp_Object docstring,
 Lisp_Object interactive, Lisp_Object type)
{
  CHECK_SYMBOL (function);
  CHECK_STRING (file);

  if (!NILP (XSYMBOL (function)->u.s.function)
      && !AUTOLOADP (XSYMBOL (function)->u.s.function))
    return Qnil;

  if (!NILP (Vpurify_flag) && BASE_EQ (docstring, make_fixnum (0)))
    docstring = make_ufixnum (XHASH (function));
  return Fdefalias (function,
                    list5 (Qautoload, file, docstring, interactive, type),
                    Qnil);
}

Lisp_Object
eval_sub (Lisp_Object form)
{
  TODO_NELISP_LATER;

  if (SYMBOLP (form))
    {
      Lisp_Object lex_binding = Fassq (form, Vinternal_interpreter_environment);
      return !NILP (lex_binding) ? XCDR (lex_binding) : Fsymbol_value (form);
    }

  if (!CONSP (form))
    return form;

  if (++lisp_eval_depth > max_lisp_eval_depth)
    {
      if (max_lisp_eval_depth < 100)
        max_lisp_eval_depth = 100;
      if (lisp_eval_depth > max_lisp_eval_depth)
        TODO;
    }

  Lisp_Object original_fun = XCAR (form);
  Lisp_Object original_args = XCDR (form);
  specpdl_ref count
    = record_in_backtrace (original_fun, &original_args, UNEVALLED);

  Lisp_Object fun, val;
  Lisp_Object argvals[8];
  fun = original_fun;
  if (!SYMBOLP (fun))
    {
      TODO;
    }
  else if (!NILP (fun) && (fun = XSYMBOL (fun)->u.s.function, SYMBOLP (fun)))
    {
      fun = indirect_function (fun);
    }

  if (SUBRP (fun) && !NATIVE_COMP_FUNCTION_DYNP (fun))
    {
      Lisp_Object args_left = original_args;
      ptrdiff_t numargs = list_length (args_left);

      if (numargs < XSUBR (fun)->min_args
          || (XSUBR (fun)->max_args >= 0 && XSUBR (fun)->max_args < numargs))
        {
          TODO; // xsignal2 (Qwrong_number_of_arguments, original_fun,
                // make_fixnum (numargs));
        }
      else if (XSUBR (fun)->max_args == UNEVALLED)
        val = (XSUBR (fun)->function.aUNEVALLED) (args_left);
      else if (XSUBR (fun)->max_args == MANY || XSUBR (fun)->max_args > 8)
        {
          Lisp_Object *vals;
          ptrdiff_t argnum = 0;
          USE_SAFE_ALLOCA;

          SAFE_ALLOCA_LISP (vals, numargs);

          while (CONSP (args_left) && argnum < numargs)
            {
              Lisp_Object arg = XCAR (args_left);
              args_left = XCDR (args_left);
              vals[argnum++] = eval_sub (arg);
            }

          set_backtrace_args (specpdl_ref_to_ptr (count), vals, argnum);

          val = XSUBR (fun)->function.aMANY (argnum, vals);

          lisp_eval_depth--;
#if TODO_NELISP_LATER_AND
          if (backtrace_debug_on_exit (specpdl_ref_to_ptr (count)))
            val = call_debugger (list2 (Qexit, val));
#endif
          SAFE_FREE ();
          specpdl_ptr--;
          return val;
        }
      else
        {
          int i, maxargs = XSUBR (fun)->max_args;

          for (i = 0; i < maxargs; i++)
            {
              argvals[i] = eval_sub (Fcar (args_left));
              args_left = Fcdr (args_left);
            }

          set_backtrace_args (specpdl_ref_to_ptr (count), argvals, numargs);

          switch (i)
            {
            case 0:
              val = (XSUBR (fun)->function.a0 ());
              break;
            case 1:
              val = (XSUBR (fun)->function.a1 (argvals[0]));
              break;
            case 2:
              val = (XSUBR (fun)->function.a2 (argvals[0], argvals[1]));
              break;
            case 3:
              val = (XSUBR (fun)->function.a3 (argvals[0], argvals[1],
                                               argvals[2]));
              break;
            case 4:
              val = (XSUBR (fun)->function.a4 (argvals[0], argvals[1],
                                               argvals[2], argvals[3]));
              break;
            case 5:
              val
                = (XSUBR (fun)->function.a5 (argvals[0], argvals[1], argvals[2],
                                             argvals[3], argvals[4]));
              break;
            case 6:
              val = (XSUBR (fun)->function.a6 (argvals[0], argvals[1],
                                               argvals[2], argvals[3],
                                               argvals[4], argvals[5]));
              break;
            case 7:
              val
                = (XSUBR (fun)->function.a7 (argvals[0], argvals[1], argvals[2],
                                             argvals[3], argvals[4], argvals[5],
                                             argvals[6]));
              break;
            case 8:
              val
                = (XSUBR (fun)->function.a8 (argvals[0], argvals[1], argvals[2],
                                             argvals[3], argvals[4], argvals[5],
                                             argvals[6], argvals[7]));
              break;
            default:
              emacs_abort ();
            }
        }
    }
  else if (CLOSUREP (fun) || NATIVE_COMP_FUNCTION_DYNP (fun)
           || MODULE_FUNCTIONP (fun))
    return apply_lambda (fun, original_args, count);
  else
    {
      if (NILP (fun))
        xsignal1 (Qvoid_function, original_fun);
      TODO;
    }
  lisp_eval_depth--;
  specpdl_ptr--;
  return val;
}

DEFUN ("progn", Fprogn, Sprogn, 0, UNEVALLED, 0,
       doc: /* Eval BODY forms sequentially and return value of last one.
usage: (progn BODY...)  */)
(Lisp_Object body)
{
  Lisp_Object CACHEABLE val = Qnil;

  while (CONSP (body))
    {
      Lisp_Object form = XCAR (body);
      body = XCDR (body);
      val = eval_sub (form);
    }

  return val;
}

void
prog_ignore (Lisp_Object body)
{
  Fprogn (body);
}

Lisp_Object
funcall_subr (struct Lisp_Subr *subr, ptrdiff_t numargs, Lisp_Object *args)
{
  eassume (numargs >= 0);
  if (numargs >= subr->min_args)
    {
      /* Conforming call to finite-arity subr.  */
      ptrdiff_t maxargs = subr->max_args;
      if (numargs <= maxargs && maxargs <= 8)
        {
          Lisp_Object argbuf[8];
          Lisp_Object *a;
          if (numargs < maxargs)
            {
              eassume (maxargs <= (long) ARRAYELTS (argbuf));
              a = argbuf;
              memcpy (a, args, numargs * word_size);
              memclear (a + numargs, (maxargs - numargs) * word_size);
            }
          else
            a = args;
          switch (maxargs)
            {
            case 0:
              return subr->function.a0 ();
            case 1:
              return subr->function.a1 (a[0]);
            case 2:
              return subr->function.a2 (a[0], a[1]);
            case 3:
              return subr->function.a3 (a[0], a[1], a[2]);
            case 4:
              return subr->function.a4 (a[0], a[1], a[2], a[3]);
            case 5:
              return subr->function.a5 (a[0], a[1], a[2], a[3], a[4]);
            case 6:
              return subr->function.a6 (a[0], a[1], a[2], a[3], a[4], a[5]);
            case 7:
              return subr->function.a7 (a[0], a[1], a[2], a[3], a[4], a[5],
                                        a[6]);
            case 8:
              return subr->function.a8 (a[0], a[1], a[2], a[3], a[4], a[5],
                                        a[6], a[7]);
            }
          eassume (false);
        }

      /* Call to n-adic subr.  */
      if (maxargs == MANY || maxargs > 8)
        return subr->function.aMANY (numargs, args);
    }

  Lisp_Object fun;
  XSETSUBR (fun, subr);
  if (subr->max_args == UNEVALLED)
    xsignal1 (Qinvalid_function, fun);
  else
    xsignal2 (Qwrong_number_of_arguments, fun, make_fixnum (numargs));
}
static Lisp_Object
apply_lambda (Lisp_Object fun, Lisp_Object args, specpdl_ref count)
{
  Lisp_Object *arg_vector;
  Lisp_Object tem;
  USE_SAFE_ALLOCA;

  ptrdiff_t numargs = list_length (args);
  SAFE_ALLOCA_LISP (arg_vector, numargs);
  Lisp_Object args_left = args;

  for (ptrdiff_t i = 0; i < numargs; i++)
    {
      tem = Fcar (args_left), args_left = Fcdr (args_left);
      tem = eval_sub (tem);
      arg_vector[i] = tem;
    }

  set_backtrace_args (specpdl_ref_to_ptr (count), arg_vector, numargs);
  tem = funcall_lambda (fun, numargs, arg_vector);

  lisp_eval_depth--;
#if TODO_NELISP_LATER_AND
  if (backtrace_debug_on_exit (specpdl_ref_to_ptr (count)))
    tem = call_debugger (list2 (Qexit, tem));
#endif
  SAFE_FREE ();
  specpdl_ptr--;
  return tem;
}
static Lisp_Object
funcall_lambda (Lisp_Object fun, ptrdiff_t nargs, Lisp_Object *arg_vector)
{
  Lisp_Object syms_left, lexenv;

  if (CONSP (fun))
    {
      lexenv = Qnil;
      syms_left = XCDR (fun);
      if (CONSP (syms_left))
        syms_left = XCAR (syms_left);
      else
        xsignal1 (Qinvalid_function, fun);
    }
  else if (CLOSUREP (fun))
    {
      syms_left = AREF (fun, CLOSURE_ARGLIST);
      if (FIXNUMP (syms_left))
        return exec_byte_code (fun, XFIXNUM (syms_left), nargs, arg_vector);
      lexenv = CONSP (AREF (fun, CLOSURE_CODE)) ? AREF (fun, CLOSURE_CONSTANTS)
                                                : Qnil;
    }
  else
    emacs_abort ();

  specpdl_ref count = SPECPDL_INDEX ();
  ptrdiff_t i = 0;
  bool optional = false;
  bool rest = false;
  bool previous_rest = false;
  for (; CONSP (syms_left); syms_left = XCDR (syms_left))
    {
      maybe_quit ();

      Lisp_Object next = maybe_remove_pos_from_symbol (XCAR (syms_left));
      if (!BARE_SYMBOL_P (next))
        xsignal1 (Qinvalid_function, fun);

      if (BASE_EQ (next, Qand_rest))
        {
          if (rest || previous_rest)
            xsignal1 (Qinvalid_function, fun);
          rest = 1;
          previous_rest = true;
        }
      else if (BASE_EQ (next, Qand_optional))
        {
          if (optional || rest || previous_rest)
            xsignal1 (Qinvalid_function, fun);
          optional = 1;
        }
      else
        {
          Lisp_Object arg;
          if (rest)
            {
              arg = Flist (nargs - i, &arg_vector[i]);
              i = nargs;
            }
          else if (i < nargs)
            arg = arg_vector[i++];
          else if (!optional)
            xsignal2 (Qwrong_number_of_arguments, fun, make_fixnum (nargs));
          else
            arg = Qnil;

          /* Bind the argument.  */
          if (!NILP (lexenv))
            lexenv = Fcons (Fcons (next, arg), lexenv);
          else
            specbind (next, arg);
          previous_rest = false;
        }
    }

  if (!NILP (syms_left) || previous_rest)
    xsignal1 (Qinvalid_function, fun);
  else if (i < nargs)
    xsignal2 (Qwrong_number_of_arguments, fun, make_fixnum (nargs));

  if (!EQ (lexenv, Vinternal_interpreter_environment))
    specbind (Qinternal_interpreter_environment, lexenv);

  Lisp_Object val;
  if (CONSP (fun))
    val = Fprogn (XCDR (XCDR (fun)));
  else if (NATIVE_COMP_FUNCTIONP (fun))
    {
      eassert (NATIVE_COMP_FUNCTION_DYNP (fun));
      val = XSUBR (fun)->function.a0 ();
    }
  else
    {
      eassert (CLOSUREP (fun));
      val = CONSP (AREF (fun, CLOSURE_CODE)) ? Fprogn (AREF (fun, CLOSURE_CODE))
                                             : exec_byte_code (fun, 0, 0, NULL);
    }

  return unbind_to (count, val);
}

bool
FUNCTIONP (Lisp_Object object)
{
  if (SYMBOLP (object) && !NILP (Ffboundp (object)))
    {
      object = Findirect_function (object, Qt);

      if (CONSP (object) && EQ (XCAR (object), Qautoload))
        {
          /* Autoloaded symbols are functions, except if they load
             macros or keymaps.  */
          for (int i = 0; i < 4 && CONSP (object); i++)
            object = XCDR (object);

          return !(CONSP (object) && !NILP (XCAR (object)));
        }
    }

  if (SUBRP (object))
    return XSUBR (object)->max_args != UNEVALLED;
  else if (CLOSUREP (object) || MODULE_FUNCTIONP (object))
    return true;
  else if (CONSP (object))
    {
      Lisp_Object car = XCAR (object);
      return EQ (car, Qlambda);
    }
  else
    return false;
}
DEFUN ("functionp", Ffunctionp, Sfunctionp, 1, 1, 0,
       doc: /* Return t if OBJECT is a function.

An object is a function if it is callable via `funcall'; this includes
symbols with function bindings, but excludes macros and special forms.

Ordinarily return nil if OBJECT is not a function, although t might be
returned in rare cases.  */)
(Lisp_Object object)
{
  if (FUNCTIONP (object))
    return Qt;
  return Qnil;
}

Lisp_Object
funcall_general (Lisp_Object fun, ptrdiff_t numargs, Lisp_Object *args)
{
  Lisp_Object original_fun = fun;
  if (SYMBOLP (fun) && !NILP (fun)
      && (fun = XSYMBOL (fun)->u.s.function, SYMBOLP (fun)))
    fun = indirect_function (fun);

  if (SUBRP (fun) && !NATIVE_COMP_FUNCTION_DYNP (fun))
    return funcall_subr (XSUBR (fun), numargs, args);
  else if (CLOSUREP (fun) || NATIVE_COMP_FUNCTION_DYNP (fun)
           || MODULE_FUNCTIONP (fun))
    return funcall_lambda (fun, numargs, args);
  else
    {
      if (NILP (fun))
        xsignal1 (Qvoid_function, original_fun);
      if (!CONSP (fun))
        xsignal1 (Qinvalid_function, original_fun);
      Lisp_Object funcar = XCAR (fun);
      if (!SYMBOLP (funcar))
        xsignal1 (Qinvalid_function, original_fun);
      if (EQ (funcar, Qlambda))
        TODO;
      else if (EQ (funcar, Qautoload))
        TODO;
      else
        xsignal1 (Qinvalid_function, original_fun);
    }
}

DEFUN ("funcall", Ffuncall, Sfuncall, 1, MANY, 0,
       doc: /* Call first argument as a function, passing remaining arguments to it.
Return the value that function returns.
Thus, (funcall \\='cons \\='x \\='y) returns (x . y).
usage: (funcall FUNCTION &rest ARGUMENTS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  specpdl_ref count;
  UNUSED (count);

  maybe_quit ();

  if (++lisp_eval_depth > max_lisp_eval_depth)
    {
      if (max_lisp_eval_depth < 100)
        max_lisp_eval_depth = 100;
      if (lisp_eval_depth > max_lisp_eval_depth)
        xsignal1 (Qexcessive_lisp_nesting, make_fixnum (lisp_eval_depth));
    }

  count = record_in_backtrace (args[0], &args[1], nargs - 1);

  maybe_gc ();

#if TODO_NELISP_LATER_AND
  if (debug_on_next_call)
    do_debug_on_call (Qlambda, count);
#endif

  Lisp_Object val = funcall_general (args[0], nargs - 1, args + 1);

  lisp_eval_depth--;
#if TODO_NELISP_LATER_AND
  if (backtrace_debug_on_exit (specpdl_ref_to_ptr (count)))
    val = call_debugger (list2 (Qexit, val));
#endif
  specpdl_ptr--;
  return val;
}

DEFUN ("setq", Fsetq, Ssetq, 0, UNEVALLED, 0,
       doc: /* Set each SYM to the value of its VAL.
The symbols SYM are variables; they are literal (not evaluated).
The values VAL are expressions; they are evaluated.
Thus, (setq x (1+ y)) sets `x' to the value of `(1+ y)'.
The second VAL is not computed until after the first SYM is set, and so on;
each VAL can use the new value of variables set earlier in the `setq'.
The return value of the `setq' form is the value of the last VAL.
usage: (setq [SYM VAL]...)  */)
(Lisp_Object args)
{
  Lisp_Object val = args, tail = args;

  for (EMACS_INT nargs = 0; CONSP (tail); nargs += 2)
    {
      UNUSED (nargs);
      Lisp_Object sym = XCAR (tail);
      tail = XCDR (tail);
      if (!CONSP (tail))
        TODO; // xsignal2 (Qwrong_number_of_arguments, Qsetq, make_fixnum (nargs
              // + 1));
      Lisp_Object arg = XCAR (tail);
      tail = XCDR (tail);
      val = eval_sub (arg);
      Lisp_Object lex_binding
        = (SYMBOLP (sym) ? Fassq (sym, Vinternal_interpreter_environment)
                         : Qnil);
      if (!NILP (lex_binding))
        XSETCDR (lex_binding, val);
      else
        Fset (sym, val);
    }

  return val;
}

DEFUN ("let", Flet, Slet, 1, UNEVALLED, 0,
       doc: /* Bind variables according to VARLIST then eval BODY.
The value of the last form in BODY is returned.
Each element of VARLIST is a symbol (which is bound to nil)
or a list (SYMBOL VALUEFORM) (which binds SYMBOL to the value of VALUEFORM).
All the VALUEFORMs are evalled before any symbols are bound.
usage: (let VARLIST BODY...)  */)
(Lisp_Object args)
{
  Lisp_Object *temps, tem, lexenv;
  Lisp_Object elt;
  specpdl_ref count = SPECPDL_INDEX ();
  ptrdiff_t argnum;
  USE_SAFE_ALLOCA;

  Lisp_Object varlist = XCAR (args);

  EMACS_INT varlist_len = list_length (varlist);
  SAFE_ALLOCA_LISP (temps, varlist_len);
  ptrdiff_t nvars = varlist_len;

  for (argnum = 0; argnum < nvars && CONSP (varlist); argnum++)
    {
      maybe_quit ();
      elt = XCAR (varlist);
      varlist = XCDR (varlist);
      if (SYMBOLP (elt))
        temps[argnum] = Qnil;
      else if (!NILP (Fcdr (Fcdr (elt))))
        signal_error ("`let' bindings can have only one value-form", elt);
      else
        temps[argnum] = eval_sub (Fcar (Fcdr (elt)));
    }
  nvars = argnum;

  lexenv = Vinternal_interpreter_environment;

  varlist = XCAR (args);
  for (argnum = 0; argnum < nvars && CONSP (varlist); argnum++)
    {
      elt = XCAR (varlist);
      varlist = XCDR (varlist);
      Lisp_Object var
        = maybe_remove_pos_from_symbol (SYMBOLP (elt) ? elt : Fcar (elt));
      CHECK_TYPE (BARE_SYMBOL_P (var), Qsymbolp, var);
      tem = temps[argnum];

      if (!NILP (lexenv) && !XBARE_SYMBOL (var)->u.s.declared_special
          && NILP (Fmemq (var, Vinternal_interpreter_environment)))
        lexenv = Fcons (Fcons (var, tem), lexenv);
      else
        specbind (var, tem);
    }

  if (!EQ (lexenv, Vinternal_interpreter_environment))
    specbind (Qinternal_interpreter_environment, lexenv);

  elt = Fprogn (XCDR (args));
  return SAFE_FREE_UNBIND_TO (count, elt);
}

static bool
lexbound_p (Lisp_Object symbol)
{
  union specbinding *pdl = specpdl_ptr;
  while (pdl > specpdl)
    {
      switch ((--pdl)->kind)
        {
        case SPECPDL_LET_DEFAULT:
        case SPECPDL_LET:
          if (BASE_EQ (specpdl_symbol (pdl), Qinternal_interpreter_environment))
            {
              Lisp_Object env = specpdl_old_value (pdl);
              if (CONSP (env) && !NILP (Fassq (symbol, env)))
                return true;
            }
          break;

        default:
          break;
        }
    }
  return false;
}

DEFUN ("internal--define-uninitialized-variable",
       Finternal__define_uninitialized_variable,
       Sinternal__define_uninitialized_variable, 1, 2, 0,
       doc: /* Define SYMBOL as a variable, with DOC as its docstring.
This is like `defvar' and `defconst' but without affecting the variable's
value.  */)
(Lisp_Object symbol, Lisp_Object doc)
{
  if (!XSYMBOL (symbol)->u.s.declared_special && lexbound_p (symbol))
    xsignal2 (Qerror,
              build_string ("Defining as dynamic an already lexical var"),
              symbol);

  XSYMBOL (symbol)->u.s.declared_special = true;
  if (!NILP (doc))
    {
      if (!NILP (Vpurify_flag))
        TODO;
      Fput (symbol, Qvariable_documentation, doc);
    }
  LOADHIST_ATTACH (symbol);
  return Qnil;
}

static Lisp_Object
defvar (Lisp_Object sym, Lisp_Object initvalue, Lisp_Object docstring,
        bool eval)
{
  UNUSED (eval);
  UNUSED (initvalue);
  Lisp_Object tem;

  CHECK_SYMBOL (sym);

  tem = Fdefault_boundp (sym);

  Finternal__define_uninitialized_variable (sym, docstring);

  if (NILP (tem))
    Fset_default (sym, eval ? eval_sub (initvalue) : initvalue);
  else
    {
      TODO;
    }
  return sym;
}
DEFUN ("defvar", Fdefvar, Sdefvar, 1, UNEVALLED, 0,
       doc: /* Define SYMBOL as a variable, and return SYMBOL.
You are not required to define a variable in order to use it, but
defining it lets you supply an initial value and documentation, which
can be referred to by the Emacs help facilities and other programming
tools.

If SYMBOL's value is void and the optional argument INITVALUE is
provided, INITVALUE is evaluated and the result used to set SYMBOL's
value.  If SYMBOL is buffer-local, its default value is what is set;
buffer-local values are not affected.  If INITVALUE is missing,
SYMBOL's value is not set.

If INITVALUE is provided, the `defvar' form also declares the variable
as \"special\", so that it is always dynamically bound even if
`lexical-binding' is t.  If INITVALUE is missing, the form marks the
variable \"special\" locally (i.e., within the current
lexical scope, or the current file, if the form is at top-level),
and does nothing if `lexical-binding' is nil.

If SYMBOL is let-bound, then this form does not affect the local let
binding but the toplevel default binding instead, like
`set-toplevel-default-binding`.
(`defcustom' behaves similarly in this respect.)

The optional argument DOCSTRING is a documentation string for the
variable.

To define a user option, use `defcustom' instead of `defvar'.

To define a buffer-local variable, use `defvar-local'.
usage: (defvar SYMBOL &optional INITVALUE DOCSTRING)  */)
(Lisp_Object args)
{
  Lisp_Object sym, tail;

  sym = XCAR (args);
  tail = XCDR (args);

  CHECK_SYMBOL (sym);

  if (!NILP (tail))
    {
      if (!NILP (XCDR (tail)) && !NILP (XCDR (XCDR (tail))))
        error ("Too many arguments");
      Lisp_Object exp = XCAR (tail);
      tail = XCDR (tail);
      return defvar (sym, exp, CAR (tail), true);
    }
  else if (!NILP (Vinternal_interpreter_environment)
           && (SYMBOLP (sym) && !XSYMBOL (sym)->u.s.declared_special))
    Vinternal_interpreter_environment
      = Fcons (sym, Vinternal_interpreter_environment);
  else
    {
    }

  return sym;
}

DEFUN ("defconst", Fdefconst, Sdefconst, 2, UNEVALLED, 0,
       doc: /* Define SYMBOL as a constant variable.
This declares that neither programs nor users should ever change the
value.  This constancy is not actually enforced by Emacs Lisp, but
SYMBOL is marked as a special variable so that it is never lexically
bound.

The `defconst' form always sets the value of SYMBOL to the result of
evalling INITVALUE.  If SYMBOL is buffer-local, its default value is
what is set; buffer-local values are not affected.  If SYMBOL has a
local binding, then this form sets the local binding's value.
However, you should normally not make local bindings for variables
defined with this form.

The optional DOCSTRING specifies the variable's documentation string.
usage: (defconst SYMBOL INITVALUE [DOCSTRING])  */)
(Lisp_Object args)
{
  Lisp_Object sym, tem;

  sym = XCAR (args);
  CHECK_SYMBOL (sym);
  Lisp_Object docstring = Qnil;
  if (!NILP (XCDR (XCDR (args))))
    {
      if (!NILP (XCDR (XCDR (XCDR (args)))))
        error ("Too many arguments");
      docstring = XCAR (XCDR (XCDR (args)));
    }
  tem = eval_sub (XCAR (XCDR (args)));
  return Fdefconst_1 (sym, tem, docstring);
}

DEFUN ("defconst-1", Fdefconst_1, Sdefconst_1, 2, 3, 0,
       doc: /* Like `defconst' but as a function.
More specifically, behaves like (defconst SYM 'INITVALUE DOCSTRING).  */)
(Lisp_Object sym, Lisp_Object initvalue, Lisp_Object docstring)
{
  CHECK_SYMBOL (sym);
  Lisp_Object tem = initvalue;
  Finternal__define_uninitialized_variable (sym, docstring);
  if (!NILP (Vpurify_flag))
    tem = Fpurecopy (tem);
  Fset_default (sym, tem);
  Fput (sym, Qrisky_local_variable, Qt);
  return sym;
}

DEFUN ("defvaralias", Fdefvaralias, Sdefvaralias, 2, 3, 0,
       doc: /* Make NEW-ALIAS a variable alias for symbol BASE-VARIABLE.
Aliased variables always have the same value; setting one sets the other.
Third arg DOCSTRING, if non-nil, is documentation for NEW-ALIAS.  If it is
omitted or nil, NEW-ALIAS gets the documentation string of BASE-VARIABLE,
or of the variable at the end of the chain of aliases, if BASE-VARIABLE is
itself an alias.  If NEW-ALIAS is bound, and BASE-VARIABLE is not,
then the value of BASE-VARIABLE is set to that of NEW-ALIAS.
The return value is BASE-VARIABLE.

If the resulting chain of variable definitions would contain a loop,
signal a `cyclic-variable-indirection' error.  */)
(Lisp_Object new_alias, Lisp_Object base_variable, Lisp_Object docstring)
{
  CHECK_SYMBOL (new_alias);
  CHECK_SYMBOL (base_variable);

  if (SYMBOL_CONSTANT_P (new_alias))
    error ("Cannot make a constant an alias: %s",
           SDATA (SYMBOL_NAME (new_alias)));

  struct Lisp_Symbol *sym = XSYMBOL (new_alias);

  /* Ensure non-circularity.  */
  struct Lisp_Symbol *s = XSYMBOL (base_variable);
  for (;;)
    {
      if (s == sym)
        xsignal1 (Qcyclic_variable_indirection, base_variable);
      if (s->u.s.redirect != SYMBOL_VARALIAS)
        break;
      s = SYMBOL_ALIAS (s);
    }

  switch (sym->u.s.redirect)
    {
    case SYMBOL_FORWARDED:
      error ("Cannot make a built-in variable an alias: %s",
             SDATA (SYMBOL_NAME (new_alias)));
    case SYMBOL_LOCALIZED:
      error ("Don't know how to make a buffer-local variable an alias: %s",
             SDATA (SYMBOL_NAME (new_alias)));
    case SYMBOL_PLAINVAL:
    case SYMBOL_VARALIAS:
      break;
    default:
      emacs_abort ();
    }

  if (NILP (Fboundp (base_variable)))
    set_internal (base_variable, find_symbol_value (new_alias), Qnil,
                  SET_INTERNAL_BIND);
  else if (!NILP (Fboundp (new_alias))
           && !EQ (find_symbol_value (new_alias),
                   find_symbol_value (base_variable)))
    {
      TODO;
    }

  {
    union specbinding *p;

    for (p = specpdl_ptr; p > specpdl;)
      if ((--p)->kind >= SPECPDL_LET && (EQ (new_alias, specpdl_symbol (p))))
        error ("Don't know how to make a let-bound variable an alias: %s",
               SDATA (SYMBOL_NAME (new_alias)));
  }

  if (sym->u.s.trapped_write == SYMBOL_TRAPPED_WRITE)
    TODO; // notify_variable_watchers (new_alias, base_variable, Qdefvaralias,
          // Qnil);

  sym->u.s.declared_special = true;
  XSYMBOL (base_variable)->u.s.declared_special = true;
  sym->u.s.redirect = SYMBOL_VARALIAS;
  SET_SYMBOL_ALIAS (sym, XSYMBOL (base_variable));
  sym->u.s.trapped_write = XSYMBOL (base_variable)->u.s.trapped_write;
  LOADHIST_ATTACH (new_alias);
  Fput (new_alias, Qvariable_documentation, docstring);

  return base_variable;
}

DEFUN ("or", For, Sor, 0, UNEVALLED, 0,
       doc: /* Eval args until one of them yields non-nil, then return that value.
The remaining args are not evalled at all.
If all args return nil, return nil.
usage: (or CONDITIONS...)  */)
(Lisp_Object args)
{
  Lisp_Object val = Qnil;

  while (CONSP (args))
    {
      Lisp_Object arg = XCAR (args);
      args = XCDR (args);
      val = eval_sub (arg);
      if (!NILP (val))
        break;
    }

  return val;
}
DEFUN ("and", Fand, Sand, 0, UNEVALLED, 0,
       doc: /* Eval args until one of them yields nil, then return nil.
The remaining args are not evalled at all.
If no arg yields nil, return the last arg's value.
usage: (and CONDITIONS...)  */)
(Lisp_Object args)
{
  Lisp_Object val = Qt;

  while (CONSP (args))
    {
      Lisp_Object arg = XCAR (args);
      args = XCDR (args);
      val = eval_sub (arg);
      if (NILP (val))
        break;
    }

  return val;
}

DEFUN ("if", Fif, Sif, 2, UNEVALLED, 0,
       doc: /* If COND yields non-nil, do THEN, else do ELSE...
Returns the value of THEN or the value of the last of the ELSE's.
THEN must be one expression, but ELSE... can be zero or more expressions.
If COND yields nil, and there are no ELSE's, the value is nil.
usage: (if COND THEN ELSE...)  */)
(Lisp_Object args)
{
  Lisp_Object cond;

  cond = eval_sub (XCAR (args));

  if (!NILP (cond))
    return eval_sub (Fcar (XCDR (args)));
  return Fprogn (Fcdr (XCDR (args)));
}

DEFUN ("while", Fwhile, Swhile, 1, UNEVALLED, 0,
       doc: /* If TEST yields non-nil, eval BODY... and repeat.
The order of execution is thus TEST, BODY, TEST, BODY and so on
until TEST returns nil.

The value of a `while' form is always nil.

usage: (while TEST BODY...)  */)
(Lisp_Object args)
{
  Lisp_Object test, body;

  test = XCAR (args);
  body = XCDR (args);
  while (!NILP (eval_sub (test)))
    {
      maybe_quit ();
      prog_ignore (body);
    }

  return Qnil;
}

DEFUN ("quote", Fquote, Squote, 1, UNEVALLED, 0,
       doc: /* Return the argument, without evaluating it.  `(quote x)' yields `x'.
Warning: `quote' does not construct its return value, but just returns
the value that was pre-constructed by the Lisp reader (see info node
`(elisp)Printed Representation').
This means that \\='(a . b) is not identical to (cons \\='a \\='b): the former
does not cons.  Quoting should be reserved for constants that will
never be modified by side-effects, unless you like self-modifying code.
See the common pitfall in info node `(elisp)Rearrangement' for an example
of unexpected results when a quoted object is modified.
usage: (quote ARG)  */)
(Lisp_Object args)
{
  if (!NILP (XCDR (args)))
    xsignal2 (Qwrong_number_of_arguments, Qquote, (TODO, NULL));
  return XCAR (args);
}

static void
init_eval_once_for_pdumper (void)
{
  enum
  {
    size = 50
  };
  union specbinding *pdlvec = malloc ((size + 1) * sizeof *specpdl);
  specpdl = specpdl_ptr = pdlvec + 1;
  specpdl_end = specpdl + size;
}
void
init_eval_once (void)
{
  TODO_NELISP_LATER;
  init_eval_once_for_pdumper ();
}

void
init_eval (void)
{
  TODO_NELISP_LATER;
  specpdl_ptr = specpdl;
  {
    handlerlist_sentinel = xzalloc (sizeof (struct handler));
    handlerlist = handlerlist_sentinel->nextfree = handlerlist_sentinel;
    struct handler *c = push_handler (Qunbound, CATCHER);
    eassert (c == handlerlist_sentinel);
    handlerlist_sentinel->nextfree = NULL;
    handlerlist_sentinel->next = NULL;
  }
  lisp_eval_depth = 0;
}

void
syms_of_eval (void)
{
  DEFSYM (Qautoload, "autoload");

  DEFSYM (Qand_rest, "&rest");
  DEFSYM (Qand_optional, "&optional");

  DEFVAR_INT ("max-lisp-eval-depth", max_lisp_eval_depth,
                doc: /* Limit on depth in `eval', `apply' and `funcall' before error.

This limit serves to catch infinite recursions for you before they cause
actual stack overflow in C, which would be fatal for Emacs.
You can safely make it considerably larger than its default value,
if that proves inconveniently small.  However, if you increase it too far,
Emacs could overflow the real C stack, and crash.  */);
  max_lisp_eval_depth = 1600;

  DEFSYM (Qinternal_interpreter_environment,
            "internal-interpreter-environment");
  DEFVAR_LISP ("internal-interpreter-environment",
                 Vinternal_interpreter_environment,
                 doc: /* If non-nil, the current lexical environment of the lisp interpreter.
When lexical binding is not being used, this variable is nil.
A value of `(t)' indicates an empty environment, otherwise it is an
alist of active lexical bindings.  */);
  Vinternal_interpreter_environment = Qnil;
  /* Don't export this variable to Elisp, so no one can mess with it
   (Just imagine if someone makes it buffer-local).  */
  Funintern (Qinternal_interpreter_environment, Qnil);

  defsubr (&Ssignal);
  defsubr (&Sautoload);
  defsubr (&Sfunctionp);
  defsubr (&Sfuncall);
  defsubr (&Ssetq);
  defsubr (&Slet);
  defsubr (&Sinternal__define_uninitialized_variable);
  defsubr (&Sdefvar);
  defsubr (&Sdefconst);
  defsubr (&Sdefconst_1);
  defsubr (&Sdefvaralias);
  defsubr (&Sprogn);
  defsubr (&Sif);
  defsubr (&Swhile);
  defsubr (&Sor);
  defsubr (&Sand);
  defsubr (&Squote);
}
