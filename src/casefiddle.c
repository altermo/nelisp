#include "lisp.h"
#include "character.h"
#include "syntax.h"

enum case_action
{
  CASE_UP,
  CASE_DOWN,
  CASE_CAPITALIZE,
  CASE_CAPITALIZE_UP
};

struct casing_context
{
  Lisp_Object titlecase_char_table;

  Lisp_Object specialcase_char_tables[3];

  enum case_action flag;

  bool inbuffer;

  bool inword;

  bool downcase_last;
};

static bool
case_ch_is_word (enum syntaxcode syntax)
{
  return syntax == Sword || (case_symbols_as_words && syntax == Ssymbol);
}

static void
prepare_casing_context (struct casing_context *ctx, enum case_action flag,
                        bool inbuffer)
{
  ctx->flag = flag;
  ctx->inbuffer = inbuffer;
  ctx->inword = false;
  ctx->titlecase_char_table
    = (flag < CASE_CAPITALIZE ? Qnil : uniprop_table (Qtitlecase));
  ctx->specialcase_char_tables[CASE_UP]
    = (flag == CASE_DOWN ? Qnil : uniprop_table (Qspecial_uppercase));
  ctx->specialcase_char_tables[CASE_DOWN]
    = (flag == CASE_UP ? Qnil : uniprop_table (Qspecial_lowercase));
  ctx->specialcase_char_tables[CASE_CAPITALIZE]
    = (flag < CASE_CAPITALIZE ? Qnil : uniprop_table (Qspecial_titlecase));

#if TODO_NELISP_LATER_AND
  if (NILP (XCHAR_TABLE (BVAR (current_buffer, downcase_table))->extras[1]))
    Fset_case_table (BVAR (current_buffer, downcase_table));

  if (inbuffer && flag >= CASE_CAPITALIZE)
    SETUP_BUFFER_SYNTAX_TABLE ();
#endif
}

struct casing_str_buf
{
  unsigned char data[max (6, MAX_MULTIBYTE_LENGTH)];
  unsigned char len_chars;
  unsigned char len_bytes;
};

static int
case_character_impl (struct casing_str_buf *buf, struct casing_context *ctx,
                     int ch)
{
  enum case_action flag;
  Lisp_Object prop;
  int cased;

  bool was_inword = ctx->inword;
  ctx->inword = case_ch_is_word (SYNTAX (ch))
                && (!ctx->inbuffer || was_inword || !syntax_prefix_flag_p (ch));

  if (ctx->flag == CASE_CAPITALIZE)
    flag = ctx->flag - was_inword;
  else if (ctx->flag != CASE_CAPITALIZE_UP)
    flag = ctx->flag;
  else if (!was_inword)
    flag = CASE_CAPITALIZE;
  else
    {
      cased = ch;
      goto done;
    }

  if (buf && !NILP (ctx->specialcase_char_tables[flag]))
    {
      prop = CHAR_TABLE_REF (ctx->specialcase_char_tables[flag], ch);
      if (STRINGP (prop))
        {
          struct Lisp_String *str = XSTRING (prop);
          if (STRING_BYTES (str) <= (long) sizeof buf->data)
            {
              buf->len_chars = str->u.s.size;
              buf->len_bytes = STRING_BYTES (str);
              memcpy (buf->data, str->u.s.data, buf->len_bytes);
              return 1;
            }
        }
    }

  if (flag == CASE_DOWN)
    {
      cased = downcase (ch);
      ctx->downcase_last = true;
    }
  else
    {
      bool cased_is_set = false;
      ctx->downcase_last = false;
      if (!NILP (ctx->titlecase_char_table))
        {
          prop = CHAR_TABLE_REF (ctx->titlecase_char_table, ch);
          if (CHARACTERP (prop))
            {
              cased = XFIXNAT (prop);
              cased_is_set = true;
            }
        }
      if (!cased_is_set)
        cased = upcase (ch);
    }

done:
  if (!buf)
    return cased;
  buf->len_chars = 1;
  buf->len_bytes = CHAR_STRING (cased, buf->data);
  return cased != ch;
}

enum
{
  GREEK_CAPITAL_LETTER_SIGMA = 0x03A3
};
enum
{
  GREEK_SMALL_LETTER_FINAL_SIGMA = 0x03C2
};

static inline int
case_single_character (struct casing_context *ctx, int ch)
{
  return case_character_impl (NULL, ctx, ch);
}

static bool
case_character (struct casing_str_buf *buf, struct casing_context *ctx, int ch,
                const unsigned char *next)
{
  bool was_inword = ctx->inword;
  bool changed = case_character_impl (buf, ctx, ch);

  if (was_inword && ch == GREEK_CAPITAL_LETTER_SIGMA && changed
      && (!next || !case_ch_is_word (SYNTAX (STRING_CHAR (next)))))
    {
      buf->len_bytes = CHAR_STRING (GREEK_SMALL_LETTER_FINAL_SIGMA, buf->data);
      buf->len_chars = 1;
    }

  return changed;
}

static inline int
make_char_unibyte (int c)
{
  return ASCII_CHAR_P (c) ? c : CHAR_TO_BYTE8 (c);
}

static Lisp_Object
do_casify_natnum (struct casing_context *ctx, Lisp_Object obj)
{
  int flagbits
    = (CHAR_ALT | CHAR_SUPER | CHAR_HYPER | CHAR_SHIFT | CHAR_CTL | CHAR_META);
  int ch = XFIXNAT (obj);

  if (!(0 <= ch && ch <= flagbits))
    return obj;

  int flags = ch & flagbits;
  ch = ch & ~flagbits;

#if TODO_NELISP_LATER_AND
  bool multibyte
    = (ch >= 256 || !NILP (BVAR (current_buffer, enable_multibyte_characters)));
#else
  bool multibyte = false;
#endif
  if (!multibyte)
    ch = make_char_multibyte (ch);
  int cased = case_single_character (ctx, ch);
  if (cased == ch)
    return obj;

  if (!multibyte)
    cased = make_char_unibyte (cased);
  return make_fixed_natnum (cased | flags);
}

static Lisp_Object
do_casify_multibyte_string (struct casing_context *ctx, Lisp_Object obj)
{
  verify (offsetof (struct casing_str_buf, data) == 0);

  ptrdiff_t size = SCHARS (obj), n;
  USE_SAFE_ALLOCA;
  if (ckd_mul (&n, size, MAX_MULTIBYTE_LENGTH)
      || ckd_add (&n, n, sizeof (struct casing_str_buf)))
    n = PTRDIFF_MAX;
  unsigned char *dst = SAFE_ALLOCA (n);
  unsigned char *dst_end = dst + n;
  unsigned char *o = dst;

  const unsigned char *src = SDATA (obj);

  for (n = 0; size; --size)
    {
      if ((unsigned long) (dst_end - o) < sizeof (struct casing_str_buf))
        string_overflow ();
      int ch = string_char_advance (&src);
      case_character ((struct casing_str_buf *) o, ctx, ch,
                      size > 1 ? src : NULL);
      n += ((struct casing_str_buf *) o)->len_chars;
      o += ((struct casing_str_buf *) o)->len_bytes;
    }
  eassert (o <= dst_end);
  obj = make_multibyte_string ((char *) dst, n, o - dst);
  SAFE_FREE ();
  return obj;
}

static int
ascii_casify_character (bool downcase, int c)
{
  Lisp_Object cased = CHAR_TABLE_REF (downcase ? uniprop_table (Qlowercase)
                                               : uniprop_table (Quppercase),
                                      c);
  return FIXNATP (cased) ? XFIXNAT (cased) : c;
}

static Lisp_Object
do_casify_unibyte_string (struct casing_context *ctx, Lisp_Object obj)
{
  ptrdiff_t i, size = SCHARS (obj);
  int ch, cased;

  obj = Fcopy_sequence (obj);
  for (i = 0; i < size; i++)
    {
      ch = make_char_multibyte (SREF (obj, i));
      cased = case_single_character (ctx, ch);
      if (ch == cased)
        continue;
      if (ASCII_CHAR_P (ch) && !SINGLE_BYTE_CHAR_P (cased))
        cased = ascii_casify_character (ctx->downcase_last, ch);
      SSET (obj, i, make_char_unibyte (cased));
    }
  return obj;
}

static Lisp_Object
casify_object (enum case_action flag, Lisp_Object obj)
{
  struct casing_context ctx;
  prepare_casing_context (&ctx, flag, false);

  if (FIXNATP (obj))
    return do_casify_natnum (&ctx, obj);
  else if (!STRINGP (obj))
    wrong_type_argument (Qchar_or_string_p, obj);
  else if (!SCHARS (obj))
    return obj;
  else if (STRING_MULTIBYTE (obj))
    return do_casify_multibyte_string (&ctx, obj);
  else
    return do_casify_unibyte_string (&ctx, obj);
}

DEFUN ("capitalize", Fcapitalize, Scapitalize, 1, 1, 0,
       doc: /* Convert argument to capitalized form and return that.
This means that each word's first character is converted to either
title case or upper case, and the rest to lower case.

The argument may be a character or string.  The result has the same
type.  (See `downcase' for further details about the type.)

The argument object is not altered--the value is a copy.  If argument
is a character, characters which map to multiple code points when
cased, e.g. ﬁ, are returned unchanged.  */)
(Lisp_Object obj) { return casify_object (CASE_CAPITALIZE, obj); }

void
syms_of_casefiddle (void)
{
  DEFSYM (Qbounds, "bounds");
  DEFSYM (Qidentity, "identity");
  DEFSYM (Qtitlecase, "titlecase");
  DEFSYM (Qlowercase, "lowercase");
  DEFSYM (Quppercase, "uppercase");
  DEFSYM (Qspecial_uppercase, "special-uppercase");
  DEFSYM (Qspecial_lowercase, "special-lowercase");
  DEFSYM (Qspecial_titlecase, "special-titlecase");

  DEFVAR_BOOL ("case-symbols-as-words", case_symbols_as_words,
               doc: /* If non-nil, case functions treat symbol syntax as part of words.

Functions such as `upcase-initials' and `replace-match' check or modify
the case pattern of sequences of characters.  Normally, these operate on
sequences of characters whose syntax is word constituent.  If this
variable is non-nil, then they operate on sequences of characters whose
syntax is either word constituent or symbol constituent.

This is useful for programming languages and styles where only the first
letter of a symbol's name is ever capitalized.*/);
  case_symbols_as_words = 0;
  DEFSYM (Qcase_symbols_as_words, "case-symbols-as-words");
  Fmake_variable_buffer_local (Qcase_symbols_as_words);

  defsubr (&Scapitalize);
}
