#include <stdlib.h>

#include "lisp.h"

#define CACHEABLE
#define clobbered_eassert(E) eassert (sizeof (E) != 0)

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
record_unwind_protect_intmax (void (*function) (intmax_t), intmax_t arg)
{
    specpdl_ptr->unwind_intmax.kind = SPECPDL_UNWIND_INTMAX;
    specpdl_ptr->unwind_intmax.func = function;
    specpdl_ptr->unwind_intmax.arg = arg;
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

INLINE specpdl_ref
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
    UNUSED(bindflag);
    eassert (unwinding || this_binding->kind >= SPECPDL_LET);
    switch (this_binding->kind)
    {
        case SPECPDL_UNWIND: TODO;
        case SPECPDL_UNWIND_ARRAY: TODO;
        case SPECPDL_UNWIND_PTR: TODO;
        case SPECPDL_UNWIND_INT: TODO;
        case SPECPDL_UNWIND_INTMAX:
            this_binding->unwind_intmax.func (this_binding->unwind_intmax.arg);
            break;
        case SPECPDL_UNWIND_VOID: TODO;
        case SPECPDL_UNWIND_EXCURSION: TODO;
        case SPECPDL_BACKTRACE:
        case SPECPDL_NOP:
            break;
        case SPECPDL_LET: TODO;
        case SPECPDL_LET_DEFAULT: TODO;
        case SPECPDL_LET_LOCAL: TODO;
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
    union specbinding *ptr=specpdl_ptr;
    union specbinding *pdl;
    for (pdl = first; pdl != ptr; pdl++)
    {
        switch (pdl->kind)
        {
            case SPECPDL_UNWIND: TODO;
            case SPECPDL_UNWIND_ARRAY: TODO;
            case SPECPDL_UNWIND_EXCURSION: TODO;
            case SPECPDL_BACKTRACE: TODO;
            case SPECPDL_LET_DEFAULT:
            case SPECPDL_LET_LOCAL:
                TODO;
            case SPECPDL_LET: TODO;
            case SPECPDL_UNWIND_PTR: TODO;
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
internal_catch (Lisp_Object tag,
                Lisp_Object (*func) (Lisp_Object), Lisp_Object arg)
{
    struct handler *c = push_handler (tag, CATCHER);
    if (! sys_setjmp (c->jmp))
    {
        Lisp_Object val = func (arg);
        eassert (handlerlist == c);
        handlerlist = c->next;
        return val;
    } else {
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
    } else {
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
        if (! last_time)
            handlerlist = handlerlist->next;
    }
    while (! last_time);

    eassert (handlerlist == catch);

    lisp_eval_depth = catch->f_lisp_eval_depth;
    // set_act_rec (current_thread, catch->act_rec);

    sys_longjmp (catch->jmp, 1);
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
        if (!NILP (Fmemq (handler, conditions))
            || EQ (handler, Qt))
            return handlers;
    }

    return Qnil;
}
static Lisp_Object
signal_or_quit (Lisp_Object error_symbol, Lisp_Object data, bool continuable)
{
    UNUSED(continuable);
    bool oom = NILP (error_symbol);
    Lisp_Object error
        = oom ? data
        : (!SYMBOLP (error_symbol) && NILP (data)) ? error_symbol
        : Fcons (error_symbol, data);
    Lisp_Object conditions;
    Lisp_Object real_error_symbol
        = CONSP (error) ? XCAR (error) : error_symbol;
    Lisp_Object clause = Qnil;
    struct handler *h;
    int skip;
    UNUSED(skip);

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
xsignal3 (Lisp_Object error_symbol, Lisp_Object arg1, Lisp_Object arg2, Lisp_Object arg3)
{
  xsignal (error_symbol, list3 (arg1, arg2, arg3));
}

Lisp_Object
indirect_function (Lisp_Object object)
{
  while (SYMBOLP (object) && !NILP (object))
    object = XSYMBOL (object)->u.s.function;
  return object;
}

Lisp_Object
eval_sub (Lisp_Object form)
{
    TODO_NELISP_LATER;

    if (SYMBOLP (form))
    {
#if TODO_NELISP_LATER_AND
        Lisp_Object lex_binding
            = Fassq (form, Vinternal_interpreter_environment);
        return !NILP (lex_binding) ? XCDR (lex_binding) : Fsymbol_value (form);
#else
        return Fsymbol_value(form);
#endif
    }

    if (!CONSP (form))
        return form;

    #if TODO_NELISP_LATER_ELSE
    int max_lisp_eval_depth = 1600;
    #endif
    if (++lisp_eval_depth > max_lisp_eval_depth){
        if (max_lisp_eval_depth < 100)
            max_lisp_eval_depth = 100;
        if (lisp_eval_depth > max_lisp_eval_depth)
            TODO;
    }

    Lisp_Object original_fun = XCAR (form);
    Lisp_Object original_args = XCDR (form);
    specpdl_ref count = record_in_backtrace (original_fun, &original_args, UNEVALLED);

    Lisp_Object fun, val;
    Lisp_Object argvals[8];
    fun = original_fun;
    if (!SYMBOLP (fun)) {
        TODO;
    } else if (!NILP (fun) && (fun = XSYMBOL (fun)->u.s.function, SYMBOLP (fun))) {
        fun = indirect_function (fun);
    }

    if (SUBRP (fun) && !NATIVE_COMP_FUNCTION_DYNP (fun))
    {
        Lisp_Object args_left = original_args;
        ptrdiff_t numargs = list_length (args_left);

        if (numargs < XSUBR (fun)->min_args
            || (XSUBR (fun)->max_args >= 0
            && XSUBR (fun)->max_args < numargs)) {
            TODO; //xsignal2 (Qwrong_number_of_arguments, original_fun, make_fixnum (numargs));
        } else if (XSUBR (fun)->max_args == UNEVALLED)
            val = (XSUBR (fun)->function.aUNEVALLED) (args_left);
        else if (XSUBR (fun)->max_args == MANY
                || XSUBR (fun)->max_args > 8) {

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
    } else {
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
                    val = (XSUBR (fun)->function.a3
                        (argvals[0], argvals[1], argvals[2]));
                    break;
                case 4:
                    val = (XSUBR (fun)->function.a4
                        (argvals[0], argvals[1], argvals[2], argvals[3]));
                    break;
                case 5:
                    val = (XSUBR (fun)->function.a5
                        (argvals[0], argvals[1], argvals[2], argvals[3],
                         argvals[4]));
                    break;
                case 6:
                    val = (XSUBR (fun)->function.a6
                        (argvals[0], argvals[1], argvals[2], argvals[3],
                         argvals[4], argvals[5]));
                    break;
                case 7:
                    val = (XSUBR (fun)->function.a7
                        (argvals[0], argvals[1], argvals[2], argvals[3],
                         argvals[4], argvals[5], argvals[6]));
                    break;
                case 8:
                    val = (XSUBR (fun)->function.a8
                        (argvals[0], argvals[1], argvals[2], argvals[3],
                         argvals[4], argvals[5], argvals[6], argvals[7]));
                    break;
                default:
                    emacs_abort ();
            }
        }
    } else if (CLOSUREP (fun)
        || NATIVE_COMP_FUNCTION_DYNP (fun)
        || MODULE_FUNCTIONP (fun))
        TODO;
    else {
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
        UNUSED(nargs);
        Lisp_Object sym = XCAR (tail);
        tail = XCDR (tail);
        if (!CONSP (tail))
            TODO; //xsignal2 (Qwrong_number_of_arguments, Qsetq, make_fixnum (nargs + 1));
        Lisp_Object arg = XCAR (tail);
        tail = XCDR (tail);
        val = eval_sub (arg);
#if TODO_NELISP_LATER_AND
         Lisp_Object lex_binding
         = (SYMBOLP (sym)
         ? Fassq (sym, Vinternal_interpreter_environment)
         : Qnil);
         if (!NILP (lex_binding))
         XSETCDR (lex_binding, val); /* SYM is lexically bound.  */
         else
#endif
         Fset (sym, val);	/* SYM is dynamically bound.  */
         }

    return val;
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


static void
init_eval_once_for_pdumper (void)
{
    enum { size = 50 };
    union specbinding *pdlvec = malloc ((size + 1) * sizeof *specpdl);
    specpdl = specpdl_ptr = pdlvec + 1;
    specpdl_end = specpdl + size;
}
void
init_eval_once (void)
{
    TODO_NELISP_LATER;
    init_eval_once_for_pdumper();
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
    defsubr (&Ssignal);
    defsubr (&Ssetq);
    defsubr (&Sprogn);
    defsubr (&Sif);
    defsubr (&Sor);
}
