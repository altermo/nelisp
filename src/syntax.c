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

void
RE_SETUP_SYNTAX_TABLE_FOR_OBJECT (Lisp_Object object, ptrdiff_t frombyte)
{
  SETUP_BUFFER_SYNTAX_TABLE ();
  gl_state.object = object;
  if (BUFFERP (gl_state.object))
    {
      TODO;
    }
  else if (NILP (gl_state.object))
    {
      TODO;
    }
  else if (EQ (gl_state.object, Qt))
    {
      gl_state.b_property = 0;
      gl_state.e_property = PTRDIFF_MAX;
    }
  else
    {
      gl_state.b_property = 0;
      gl_state.e_property = 1 + SCHARS (gl_state.object);
    }
  if (parse_sexp_lookup_properties)
    TODO;
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

static void
SET_RAW_SYNTAX_ENTRY_RANGE (Lisp_Object table, Lisp_Object range,
                            Lisp_Object val)
{
  Fset_char_table_range (table, range, val);
}

static void
check_syntax_table (Lisp_Object obj)
{
  CHECK_TYPE (CHAR_TABLE_P (obj)
                && EQ (XCHAR_TABLE (obj)->purpose, Qsyntax_table),
              Qsyntax_table_p, obj);
}

DEFUN ("standard-syntax-table", Fstandard_syntax_table,
   Sstandard_syntax_table, 0, 0, 0,
       doc: /* Return the standard syntax table.
This is the one used for new buffers.  */)
(void) { return Vstandard_syntax_table; }

DEFUN ("string-to-syntax", Fstring_to_syntax, Sstring_to_syntax, 1, 1, 0,
       doc: /* Convert a syntax descriptor STRING into a raw syntax descriptor.
STRING should be a string of the form allowed as argument of
`modify-syntax-entry'.  The return value is a raw syntax descriptor: a
cons cell (CODE . MATCHING-CHAR) which can be used, for example, as
the value of a `syntax-table' text property.  */)
(Lisp_Object string)
{
  const unsigned char *p;
  int val;
  Lisp_Object match;

  CHECK_STRING (string);

  p = SDATA (string);
  val = syntax_spec_code[*p++];
  if (val == 0377)
    error ("Invalid syntax description letter: %c", p[-1]);

  if (val == Sinherit)
    return Qnil;

  if (*p)
    {
      int len, character = string_char_and_length (p, &len);
      XSETINT (match, character);
      if (XFIXNAT (match) == ' ')
        match = Qnil;
      p += len;
    }
  else
    match = Qnil;

  while (*p)
    switch (*p++)
      {
      case '1':
        val |= 1 << 16;
        break;

      case '2':
        val |= 1 << 17;
        break;

      case '3':
        val |= 1 << 18;
        break;

      case '4':
        val |= 1 << 19;
        break;

      case 'p':
        val |= 1 << 20;
        break;

      case 'b':
        val |= 1 << 21;
        break;

      case 'n':
        val |= 1 << 22;
        break;

      case 'c':
        val |= 1 << 23;
        break;
      }

  if (val < ASIZE (Vsyntax_code_object) && NILP (match))
    return AREF (Vsyntax_code_object, val);
  else
    return Fcons (make_fixnum (val), match);
}

DEFUN ("modify-syntax-entry", Fmodify_syntax_entry, Smodify_syntax_entry, 2, 3,
  "cSet syntax for character: \nsSet syntax for %s to: ",
       doc: /* Set syntax for character CHAR according to string NEWENTRY.
The syntax is changed only for table SYNTAX-TABLE, which defaults to
 the current buffer's syntax table.
CHAR may be a cons (MIN . MAX), in which case, syntaxes of all characters
in the range MIN to MAX are changed.
The first character of NEWENTRY should be one of the following:
  Space or -  whitespace syntax.    w   word constituent.
  _           symbol constituent.   .   punctuation.
  (           open-parenthesis.     )   close-parenthesis.
  "           string quote.         \\   escape.
  $           paired delimiter.     \\='   expression quote or prefix operator.
  <           comment starter.      >   comment ender.
  /           character-quote.      @   inherit from parent table.
  |           generic string fence. !   generic comment fence.

Only single-character comment start and end sequences are represented thus.
Two-character sequences are represented as described below.
The second character of NEWENTRY is the matching parenthesis,
 used only if the first character is `(' or `)'.
Any additional characters are flags.
Defined flags are the characters 1, 2, 3, 4, b, p, and n.
 1 means CHAR is the start of a two-char comment start sequence.
 2 means CHAR is the second character of such a sequence.
 3 means CHAR is the start of a two-char comment end sequence.
 4 means CHAR is the second character of such a sequence.

There can be several orthogonal comment sequences.  This is to support
language modes such as C++.  By default, all comment sequences are of style
a, but you can set the comment sequence style to b (on the second character
of a comment-start, and the first character of a comment-end sequence) and/or
c (on any of its chars) using this flag:
 b means CHAR is part of comment sequence b.
 c means CHAR is part of comment sequence c.
 n means CHAR is part of a nestable comment sequence.

 p means CHAR is a prefix character for `backward-prefix-chars';
   such characters are treated as whitespace when they occur
   between expressions.
usage: (modify-syntax-entry CHAR NEWENTRY &optional SYNTAX-TABLE)  */)
(Lisp_Object c, Lisp_Object newentry, Lisp_Object syntax_table)
{
  if (CONSP (c))
    {
      CHECK_CHARACTER_CAR (c);
      CHECK_CHARACTER_CDR (c);
    }
  else
    CHECK_CHARACTER (c);

  if (NILP (syntax_table))
    syntax_table = BVAR (current_buffer, syntax_table);
  else
    check_syntax_table (syntax_table);

  newentry = Fstring_to_syntax (newentry);
  if (CONSP (c))
    SET_RAW_SYNTAX_ENTRY_RANGE (syntax_table, c, newentry);
  else
    SET_RAW_SYNTAX_ENTRY (syntax_table, XFIXNUM (c), newentry);

  clear_regexp_cache ();

  return Qnil;
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
  DEFSYM (Qsyntax_table_p, "syntax-table-p");

  staticpro (&Vsyntax_code_object);

  staticpro (&gl_state.object);
  staticpro (&gl_state.global_code);
  staticpro (&gl_state.current_syntax_table);
  staticpro (&gl_state.old_prop);

  DEFVAR_BOOL ("parse-sexp-lookup-properties", parse_sexp_lookup_properties,
        doc: /* Non-nil means `forward-sexp', etc., obey `syntax-table' property.
Otherwise, that text property is simply ignored.
See the info node `(elisp)Syntax Properties' for a description of the
`syntax-table' property.  */);

  defsubr (&Sstandard_syntax_table);
  defsubr (&Sstring_to_syntax);
  defsubr (&Smodify_syntax_entry);
}
