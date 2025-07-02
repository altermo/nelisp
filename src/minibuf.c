#include "lisp.h"

void
syms_of_minibuf (void)
{
  DEFVAR_LISP ("minibuffer-prompt-properties", Vminibuffer_prompt_properties,
        doc: /* Text properties that are added to minibuffer prompts.
These are in addition to the basic `field' property, and stickiness
properties.  */);
  Vminibuffer_prompt_properties = list2 (Qread_only, Qt);
}
