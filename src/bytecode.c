#include "lisp.h"

Lisp_Object
exec_byte_code (Lisp_Object fun, ptrdiff_t args_template, ptrdiff_t nargs,
                Lisp_Object *args)
{
  UNUSED (args_template);
  UNUSED (nargs);
  UNUSED (args);
  UNUSED (fun);
  TODO;
}

DEFUN ("byte-code", Fbyte_code, Sbyte_code, 3, 3, 0,
       doc: /* Function used internally in byte-compiled code.
The first argument, BYTESTR, is a string of byte code;
the second, VECTOR, a vector of constants;
the third, MAXDEPTH, the maximum stack depth used in this function.
If the third argument is incorrect, Emacs may crash.  */)
(Lisp_Object bytestr, Lisp_Object vector, Lisp_Object maxdepth)
{
  if (!(STRINGP (bytestr) && VECTORP (vector) && FIXNATP (maxdepth)))
    error ("Invalid byte-code");

  if (STRING_MULTIBYTE (bytestr))
    {
      TODO; // bytestr = Fstring_as_unibyte (bytestr);
    }
  Lisp_Object fun = CALLN (Fmake_byte_code, Qnil, bytestr, vector, maxdepth);
  return exec_byte_code (fun, 0, 0, NULL);
}

DEFUN ("make-byte-code", Fmake_byte_code, Smake_byte_code, 4, MANY, 0,
       doc: /* Create a byte-code object with specified arguments as elements.
The arguments should be the ARGLIST, bytecode-string BYTE-CODE, constant
vector CONSTANTS, maximum stack size DEPTH, (optional) DOCSTRING,
and (optional) INTERACTIVE-SPEC.
The first four arguments are required; at most six have any
significance.
The ARGLIST can be either like the one of `lambda', in which case the arguments
will be dynamically bound before executing the byte code, or it can be an
integer of the form NNNNNNNRMMMMMMM where the 7bit MMMMMMM specifies the
minimum number of arguments, the 7-bit NNNNNNN specifies the maximum number
of arguments (ignoring &rest) and the R bit specifies whether there is a &rest
argument to catch the left-over arguments.  If such an integer is used, the
arguments will not be dynamically bound but will be instead pushed on the
stack before executing the byte-code.
usage: (make-byte-code ARGLIST BYTE-CODE CONSTANTS DEPTH &optional DOCSTRING INTERACTIVE-SPEC &rest ELEMENTS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  if (!((FIXNUMP (args[CLOSURE_ARGLIST]) || CONSP (args[CLOSURE_ARGLIST])
         || NILP (args[CLOSURE_ARGLIST]))
        && STRINGP (args[CLOSURE_CODE])
        && !STRING_MULTIBYTE (args[CLOSURE_CODE])
        && VECTORP (args[CLOSURE_CONSTANTS])
        && FIXNATP (args[CLOSURE_STACK_DEPTH])))
    error ("Invalid byte-code object");

  pin_string (args[CLOSURE_CODE]);

  Lisp_Object val = Fvector (nargs, args);
  XSETPVECTYPE (XVECTOR (val), PVEC_CLOSURE);
  return val;
}

void
syms_of_bytecode (void)
{
  defsubr (&Sbyte_code);
  defsubr (&Smake_byte_code);
}
