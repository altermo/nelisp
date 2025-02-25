#ifndef EMACS_EVAL_C
#define EMACS_EVAL_C

#include "fns.c"
#include "lisp.h"

Lisp_Object
eval_sub (Lisp_Object form)
{
    TODO_NELISP_LATER

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

            #if TODO_NELISP_LATER_AND
            set_backtrace_args (specpdl_ref_to_ptr (count), argvals, numargs);
            #endif

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
        TODO
    }
    return val;
}

#endif
