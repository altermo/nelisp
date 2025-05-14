#include "lisp.h"
#include "disptab.h"

static bool
default_to_grave_quoting_style (void)
{
  if (!text_quoting_flag)
    return true;
  if (!DISP_TABLE_P (Vstandard_display_table))
    return false;
  TODO;
}

DEFUN ("text-quoting-style", Ftext_quoting_style,
       Stext_quoting_style, 0, 0, 0,
       doc: /* Return the current effective text quoting style.
If the variable `text-quoting-style' is `grave', `straight' or
`curve', just return that value.  If it is nil (the default), return
`grave' if curved quotes cannot be displayed (for instance, on a
terminal with no support for these characters), otherwise return
`curve'.  Any other value is treated as `curve'.

Note that in contrast to the variable `text-quoting-style', this
function will never return nil.  */)
(void)
{
  if (NILP (Vtext_quoting_style) ? default_to_grave_quoting_style ()
                                 : EQ (Vtext_quoting_style, Qgrave))
    return Qgrave;

  else if (EQ (Vtext_quoting_style, Qstraight))
    return Qstraight;

  else
    return Qcurve;
}

void
syms_of_doc (void)
{
  DEFSYM (Qfunction_documentation, "function-documentation");
  DEFSYM (Qgrave, "grave");
  DEFSYM (Qstraight, "straight");
  DEFSYM (Qcurve, "curve");

  DEFVAR_LISP ("text-quoting-style", Vtext_quoting_style,
               doc: /* Style to use for single quotes in help and messages.

The value of this variable determines substitution of grave accents
and apostrophes in help output (but not for display of Info
manuals) and in functions like `message' and `format-message', but not
in `format'.

The value should be one of these symbols:
  `curve':    quote with curved single quotes ‘like this’.
  `straight': quote with straight apostrophes \\='like this\\='.
  `grave':    quote with grave accent and apostrophe \\=`like this\\=';
        i.e., do not alter the original quote marks.
  nil:        like `curve' if curved single quotes are displayable,
        and like `grave' otherwise.  This is the default.

You should never read the value of this variable directly from a Lisp
program.  Use the function `text-quoting-style' instead, as that will
compute the correct value for the current terminal in the nil case.  */);
  Vtext_quoting_style = Qnil;

  DEFVAR_BOOL ("internal--text-quoting-flag", text_quoting_flag,
        doc: /* If nil, a nil `text-quoting-style' is treated as `grave'.  */);

  defsubr (&Stext_quoting_style);
}
