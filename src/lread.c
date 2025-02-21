#ifndef EMACS_LREAD_C
#define EMACS_LREAD_C

#include "lisp.h"
#include "alloc.c"

static Lisp_Object initial_obarray;

#define OBARRAY_SIZE 15121

static void
define_symbol (Lisp_Object sym, char const *str)
{
    ptrdiff_t len = strlen (str);
#if TODO_NELISP_LATER_AND
    Lisp_Object string = make_pure_c_string (str, len);
#else
    Lisp_Object string = make_unibyte_string(str, len);
#endif
    init_symbol (sym, string);
#if TODO_NELISP_LATER_AND
    if (! BASE_EQ (sym, Qunbound))
    {
        Lisp_Object bucket = oblookup (initial_obarray, str, len, len);
        eassert (FIXNUMP (bucket));
        intern_sym (sym, initial_obarray, bucket);
    }
#endif
}

void
init_obarray_once (void) {
#if TODO_NELISP_LATER_AND
    Vobarray = make_vector (OBARRAY_SIZE, make_fixnum (0));
    initial_obarray = Vobarray;
#endif
    for (unsigned long i = 0; i < ARRAYELTS (lispsym); i++)
        define_symbol (builtin_lisp_symbol (i), defsym_name[i]);
}

/*
void
syms_of_lread (void) {
    DEFVAR_LISP ("obarray", Vobarray,
                 doc: / * Symbol table for use by `intern' and `read'.
It is a vector whose length ought to be prime for best results.
The vector's contents don't make sense if examined from Lisp programs;
to find all the symbols in an obarray, use `mapatoms'.  * /);
}
*/
#endif
