#ifndef EMACS_SYNTAX_H
#define EMACS_SYNTAX_H

#include "lisp.h"
#include "buffer.h"

// #define Vstandard_syntax_table BVAR (&buffer_defaults, syntax_table)
#define Vstandard_syntax_table buffer_defaults.syntax_table_

enum syntaxcode
{
  Swhitespace,
  Spunct,
  Sword,
  Ssymbol,
  Sopen,
  Sclose,
  Squote_,
  Sstring,
  Smath,
  Sescape,
  Scharquote,
  Scomment,
  Sendcomment,
  Sinherit,
  Scomment_fence,
  Sstring_fence,
  Smax_
};

struct gl_state_s
{
  Lisp_Object object;
  ptrdiff_t start;
  ptrdiff_t stop;
  bool use_global;
  Lisp_Object global_code;
  Lisp_Object current_syntax_table;
  Lisp_Object old_prop;
  ptrdiff_t b_property;
  ptrdiff_t e_property;
  bool e_property_truncated;
  INTERVAL forward_i;
  INTERVAL backward_i;
};

extern struct gl_state_s gl_state;

INLINE Lisp_Object
syntax_property_entry (int c, bool via_property)
{
  if (via_property)
    return (gl_state.use_global
              ? gl_state.global_code
              : CHAR_TABLE_REF (gl_state.current_syntax_table, c));
  return CHAR_TABLE_REF (BVAR (current_buffer, syntax_table), c);
}

INLINE int
syntax_property_with_flags (int c, bool via_property)
{
  Lisp_Object ent = syntax_property_entry (c, via_property);
  return CONSP (ent) ? XFIXNUM (XCAR (ent)) : Swhitespace;
}
INLINE enum syntaxcode
syntax_property (int c, bool via_property)
{
  return syntax_property_with_flags (c, via_property) & 0xff;
}
INLINE enum syntaxcode
SYNTAX (int c)
{
  return syntax_property (c, false);
}

extern bool syntax_prefix_flag_p (int c);

extern unsigned char const syntax_spec_code[0400];

#endif
