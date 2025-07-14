#include "lisp.h"

DEFUN ("assoc-string", Fassoc_string, Sassoc_string, 2, 3, 0,
       doc: /* Like `assoc' but specifically for strings (and symbols).

This returns the first element of LIST whose car matches the string or
symbol KEY, or nil if no match exists.  When performing the
comparison, symbols are first converted to strings, and unibyte
strings to multibyte.  If the optional arg CASE-FOLD is non-nil, both
KEY and the elements of LIST are upcased for comparison.

Unlike `assoc', KEY can also match an entry in LIST consisting of a
single string, rather than a cons cell whose car is a string.  */)
(register Lisp_Object key, Lisp_Object list, Lisp_Object case_fold)
{
  register Lisp_Object tail;

  if (SYMBOLP (key))
    key = Fsymbol_name (key);

  for (tail = list; CONSP (tail); tail = XCDR (tail))
    {
      register Lisp_Object elt, tem, thiscar;
      elt = XCAR (tail);
      thiscar = CONSP (elt) ? XCAR (elt) : elt;
      if (SYMBOLP (thiscar))
        thiscar = Fsymbol_name (thiscar);
      else if (!STRINGP (thiscar))
        continue;
      tem = Fcompare_strings (thiscar, make_fixnum (0), Qnil, key,
                              make_fixnum (0), Qnil, case_fold);
      if (EQ (tem, Qt))
        return elt;
      maybe_quit ();
    }
  return Qnil;
}

void
syms_of_minibuf (void)
{
  defsubr (&Sassoc_string);

  DEFVAR_LISP ("minibuffer-prompt-properties", Vminibuffer_prompt_properties,
        doc: /* Text properties that are added to minibuffer prompts.
These are in addition to the basic `field' property, and stickiness
properties.  */);
  Vminibuffer_prompt_properties = list2 (Qread_only, Qt);
}
