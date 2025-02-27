#ifndef EMACS_EVAL_C
#define EMACS_EVAL_C

#include "alloc.c"
#include "fns.c"
#include "lisp.h"

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

    Lisp_Object original_fun = XCAR (form);
    Lisp_Object original_args = XCDR (form);
    specpdl_ref count = record_in_backtrace (original_fun, &original_args, UNEVALLED);

    Lisp_Object fun, val;
    Lisp_Object argvals[8];
    fun = original_fun;
    if (!SYMBOLP (fun)) {
        TODO;
    } else if (!NILP (fun) && (fun = XSYMBOL (fun)->u.s.function, SYMBOLP (fun))) {
        TODO;
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
        TODO;
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
    } else {
        TODO;
    }
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
syms_of_eval (void)
{
    defsubr (&Ssetq);
}

#endif
