#ifndef EMACS_LREAD_C
#define EMACS_LREAD_C

#include "lisp.h"
#include "alloc.c"
#include "fns.c"

static Lisp_Object initial_obarray;
static size_t oblookup_last_bucket_number;

#define OBARRAY_SIZE 15121

Lisp_Object
check_obarray (Lisp_Object obarray) {
#if TODO_NELISP_LATER_AND
  if (!fatal_error_in_progress
      && (!VECTORP (obarray) || ASIZE (obarray) == 0))
    {
      /* If Vobarray is now invalid, force it to be valid.  */
      if (EQ (Vobarray, obarray)) Vobarray = initial_obarray;
      wrong_type_argument (Qvectorp, obarray);
    }
#endif
  return obarray;
}

Lisp_Object
oblookup (Lisp_Object obarray, register const char *ptr, ptrdiff_t size, ptrdiff_t size_byte) {
    size_t hash;
    size_t obsize;
    register Lisp_Object tail;
    Lisp_Object bucket, tem;
    obarray = check_obarray (obarray);
    obsize = gc_asize (obarray);
    hash = hash_string (ptr, size_byte) % obsize;
    bucket = AREF (obarray, hash);
    oblookup_last_bucket_number = hash;
    if (BASE_EQ (bucket, make_fixnum (0)))
        ;
    else if (!SYMBOLP (bucket)) {
        TODO
    } else {
        for (tail = bucket; ; XSETSYMBOL (tail, XSYMBOL (tail)->u.s.next))
        {
            if (SBYTES (SYMBOL_NAME (tail)) == size_byte
                && SCHARS (SYMBOL_NAME (tail)) == size
                && !memcmp (SDATA (SYMBOL_NAME (tail)), ptr, size_byte))
                return tail;
            else if (XSYMBOL (tail)->u.s.next == 0)
                break;
        }
    }
    XSETINT (tem, hash);
    return tem;
}
static Lisp_Object
intern_sym (Lisp_Object sym, Lisp_Object obarray, Lisp_Object index) {
  Lisp_Object *ptr;

  XSYMBOL (sym)->u.s.interned = (EQ (obarray, initial_obarray)
				 ? SYMBOL_INTERNED_IN_INITIAL_OBARRAY
				 : SYMBOL_INTERNED);
  if (SREF (SYMBOL_NAME (sym), 0) == ':' && EQ (obarray, initial_obarray))
    {
      make_symbol_constant (sym);
      XSYMBOL (sym)->u.s.redirect = SYMBOL_PLAINVAL;
      XSYMBOL (sym)->u.s.declared_special = true;
      SET_SYMBOL_VAL (XSYMBOL (sym), sym);
    }
  ptr = aref_addr (obarray, XFIXNUM (index));
  set_symbol_next (sym, SYMBOLP (*ptr) ? XSYMBOL (*ptr) : NULL);
  *ptr = sym;
  return sym;
}

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
    if (! BASE_EQ (sym, Qunbound))
    {
        Lisp_Object bucket = oblookup (initial_obarray, str, len, len);
        eassert (FIXNUMP (bucket));
        intern_sym (sym, initial_obarray, bucket);
    }
}

void
init_obarray_once (void) {
    Vobarray = make_vector (OBARRAY_SIZE, make_fixnum (0));
    initial_obarray = Vobarray;
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
