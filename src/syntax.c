#include "syntax.h"
#include "lisp.h"
#include "character.h"

#define SYNTAX_WITH_FLAGS(c) syntax_property_with_flags (c, 1)

static bool
SYNTAX_FLAGS_PREFIX (int flags)
{
  return (flags >> 20) & 1;
}
bool
syntax_prefix_flag_p (int c)
{
  return SYNTAX_FLAGS_PREFIX (SYNTAX_WITH_FLAGS (c));
}

struct gl_state_s gl_state;

unsigned char const syntax_spec_code[0400] = {
  0377,     0377, 0377,          0377,           0377,     0377,
  0377,     0377, 0377,          0377,           0377,     0377,
  0377,     0377, 0377,          0377,           0377,     0377,
  0377,     0377, 0377,          0377,           0377,     0377,
  0377,     0377, 0377,          0377,           0377,     0377,
  0377,     0377, Swhitespace,   Scomment_fence, Sstring,  0377,
  Smath,    0377, 0377,          Squote_,        Sopen,    Sclose,
  0377,     0377, 0377,          Swhitespace,    Spunct,   Scharquote,
  0377,     0377, 0377,          0377,           0377,     0377,
  0377,     0377, 0377,          0377,           0377,     0377,
  Scomment, 0377, Sendcomment,   0377,           Sinherit, 0377,
  0377,     0377, 0377,          0377,           0377,     0377, /* @, A ... */
  0377,     0377, 0377,          0377,           0377,     0377,
  0377,     0377, 0377,          0377,           0377,     0377,
  0377,     0377, 0377,          Sword,          0377,     0377,
  0377,     0377, Sescape,       0377,           0377,     Ssymbol,
  0377,     0377, 0377,          0377,           0377,     0377,
  0377,     0377, /* `, a, ... */
  0377,     0377, 0377,          0377,           0377,     0377,
  0377,     0377, 0377,          0377,           0377,     0377,
  0377,     0377, 0377,          Sword,          0377,     0377,
  0377,     0377, Sstring_fence, 0377,           0377,     0377
};

static Lisp_Object Vsyntax_code_object;

static void
SET_RAW_SYNTAX_ENTRY (Lisp_Object table, int c, Lisp_Object val)
{
  CHAR_TABLE_SET (table, c, val);
}

void
init_syntax_once (void)
{
  register int i, c;
  Lisp_Object temp;

  DEFSYM (Qsyntax_table, "syntax-table");

  Vsyntax_code_object = make_nil_vector (Smax_);
  for (i = 0; i < Smax_; i++)
    ASET (Vsyntax_code_object, i, list1 (make_fixnum (i)));

  Fput (Qsyntax_table, Qchar_table_extra_slots, make_fixnum (0));

  temp = AREF (Vsyntax_code_object, Swhitespace);

  Vstandard_syntax_table = Fmake_char_table (Qsyntax_table, temp);

  temp = AREF (Vsyntax_code_object, Spunct);
  for (i = 0; i <= ' ' - 1; i++)
    SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, i, temp);
  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, 0177, temp);

  temp = AREF (Vsyntax_code_object, Swhitespace);
  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, ' ', temp);
  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, '\t', temp);
  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, '\n', temp);
  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, 015, temp);
  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, 014, temp);

  temp = AREF (Vsyntax_code_object, Sword);
  for (i = 'a'; i <= 'z'; i++)
    SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, i, temp);
  for (i = 'A'; i <= 'Z'; i++)
    SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, i, temp);
  for (i = '0'; i <= '9'; i++)
    SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, i, temp);

  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, '$', temp);
  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, '%', temp);

  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, '(',
                        Fcons (make_fixnum (Sopen), make_fixnum (')')));
  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, ')',
                        Fcons (make_fixnum (Sclose), make_fixnum ('(')));
  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, '[',
                        Fcons (make_fixnum (Sopen), make_fixnum (']')));
  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, ']',
                        Fcons (make_fixnum (Sclose), make_fixnum ('[')));
  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, '{',
                        Fcons (make_fixnum (Sopen), make_fixnum ('}')));
  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, '}',
                        Fcons (make_fixnum (Sclose), make_fixnum ('{')));
  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, '"',
                        Fcons (make_fixnum (Sstring), Qnil));
  SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, '\\',
                        Fcons (make_fixnum (Sescape), Qnil));

  temp = AREF (Vsyntax_code_object, Ssymbol);
  for (i = 0; i < 10; i++)
    {
      c = "_-+*/&|<>="[i];
      SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, c, temp);
    }

  temp = AREF (Vsyntax_code_object, Spunct);
  for (i = 0; i < 12; i++)
    {
      c = ".,;:?!#@~^'`"[i];
      SET_RAW_SYNTAX_ENTRY (Vstandard_syntax_table, c, temp);
    }

  temp = AREF (Vsyntax_code_object, Sword);
  char_table_set_range (Vstandard_syntax_table, 0x80, MAX_CHAR, temp);
}

void
syms_of_syntax (void)
{
  staticpro (&Vsyntax_code_object);
}
