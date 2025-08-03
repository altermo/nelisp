#include "lisp.h"
#include "buffer.h"
#include "character.h"
#include "charset.h"
#include "regex-emacs.h"

#define REGEXP_CACHE_SIZE 20

struct regexp_cache
{
  struct regexp_cache *next;
  Lisp_Object regexp, f_whitespace_regexp;
  Lisp_Object syntax_table;
  struct re_pattern_buffer buf;
  char fastmap[0400];
  bool posix;
  bool busy;
};

static struct regexp_cache searchbufs[REGEXP_CACHE_SIZE];

static struct regexp_cache *searchbuf_head;

Lisp_Object re_match_object;

static AVOID
matcher_overflow (void)
{
  error ("Stack overflow in regexp matcher");
}

DEFUN ("regexp-quote", Fregexp_quote, Sregexp_quote, 1, 1, 0,
       doc: /* Return a regexp string which matches exactly STRING and nothing else.  */)
(Lisp_Object string)
{
  char *in, *out, *end;
  char *temp;
  ptrdiff_t backslashes_added = 0;

  CHECK_STRING (string);

  USE_SAFE_ALLOCA;
  SAFE_NALLOCA (temp, 2, SBYTES (string));

  in = SSDATA (string);
  end = in + SBYTES (string);
  out = temp;

  for (; in != end; in++)
    {
      if (*in == '[' || *in == '*' || *in == '.' || *in == '\\' || *in == '?'
          || *in == '+' || *in == '^' || *in == '$')
        *out++ = '\\', backslashes_added++;
      *out++ = *in;
    }

  Lisp_Object result
    = (backslashes_added > 0
         ? make_specified_string (temp, SCHARS (string) + backslashes_added,
                                  out - temp, STRING_MULTIBYTE (string))
         : string);
  SAFE_FREE ();
  return result;
}

static void
compile_pattern_1 (struct regexp_cache *cp, Lisp_Object pattern,
                   Lisp_Object translate, bool posix)
{
  const char *whitespace_regexp;
  char *val;

  eassert (!cp->busy);
  cp->regexp = Qnil;
  cp->buf.translate = translate;
  cp->posix = posix;
  cp->buf.multibyte = STRING_MULTIBYTE (pattern);
  cp->buf.charset_unibyte = charset_unibyte;
  if (STRINGP (Vsearch_spaces_regexp))
    cp->f_whitespace_regexp = Vsearch_spaces_regexp;
  else
    cp->f_whitespace_regexp = Qnil;

  whitespace_regexp
    = STRINGP (Vsearch_spaces_regexp) ? SSDATA (Vsearch_spaces_regexp) : NULL;

  val = (char *) re_compile_pattern (SSDATA (pattern), SBYTES (pattern), posix,
                                     whitespace_regexp, &cp->buf);

  cp->syntax_table = cp->buf.used_syntax ? (TODO, Qt) : Qt;

  if (val)
    xsignal1 (Qinvalid_regexp, build_string (val));

  cp->regexp = Fcopy_sequence (pattern);
}

void
clear_regexp_cache (void)
{
  int i;

  for (i = 0; i < REGEXP_CACHE_SIZE; ++i)
    if (!searchbufs[i].busy && !BASE_EQ (searchbufs[i].syntax_table, Qt))
      searchbufs[i].regexp = Qnil;
}

static struct regexp_cache *
compile_pattern (Lisp_Object pattern, struct re_registers *regp,
                 Lisp_Object translate, bool posix, bool multibyte)
{
  struct regexp_cache *cp, **cpp, **lru_nonbusy;

  for (cpp = &searchbuf_head, lru_nonbusy = NULL;; cpp = &cp->next)
    {
      cp = *cpp;
      if (!cp->busy)
        lru_nonbusy = cpp;
      if (NILP (cp->regexp))
        goto compile_it;
      if (SCHARS (cp->regexp) == SCHARS (pattern) && !cp->busy
          && STRING_MULTIBYTE (cp->regexp) == STRING_MULTIBYTE (pattern)
          && !NILP (Fstring_equal (cp->regexp, pattern))
          && BASE_EQ (cp->buf.translate, translate) && cp->posix == posix
          && (BASE_EQ (cp->syntax_table, Qt) || (TODO, false))
          && !NILP (Fequal (cp->f_whitespace_regexp, Vsearch_spaces_regexp))
          && cp->buf.charset_unibyte == charset_unibyte)
        break;

      if (cp->next == 0)
        {
          if (!lru_nonbusy)
            error ("Too much matching reentrancy");
          cpp = lru_nonbusy;
          cp = *cpp;
        compile_it:
          eassert (!cp->busy);
          compile_pattern_1 (cp, pattern, translate, posix);
          break;
        }
    }

  *cpp = cp->next;
  cp->next = searchbuf_head;
  searchbuf_head = cp;

  if (regp)
    re_set_registers (&cp->buf, regp, regp->num_regs, regp->start, regp->end);

  cp->buf.target_multibyte = multibyte;
  return cp;
}

static void
unfreeze_pattern (void *arg)
{
  struct regexp_cache *searchbuf = arg;
  searchbuf->busy = false;
}

static void
freeze_pattern (struct regexp_cache *searchbuf)
{
  eassert (!searchbuf->busy);
  record_unwind_protect_ptr (unfreeze_pattern, searchbuf);
  searchbuf->busy = true;
}

static Lisp_Object
string_match_1 (Lisp_Object regexp, Lisp_Object string, Lisp_Object start,
                bool posix, bool modify_data)
{
  ptrdiff_t val;
  EMACS_INT pos;
  ptrdiff_t pos_byte, i;
  bool modify_match_data = NILP (Vinhibit_changing_match_data) && modify_data;

  if (running_asynch_code)
    TODO; // save_search_regs ();

  CHECK_STRING (regexp);
  CHECK_STRING (string);

  if (NILP (start))
    pos = 0, pos_byte = 0;
  else
    {
      ptrdiff_t len = SCHARS (string);

      CHECK_FIXNUM (start);
      pos = XFIXNUM (start);
      if (pos < 0 && -pos <= len)
        pos = len + pos;
      else if (0 > pos || pos > len)
        args_out_of_range (string, start);
      pos_byte = string_char_to_byte (string, pos);
    }

  set_char_table_extras (BVAR (current_buffer, case_canon_table), 2,
                         BVAR (current_buffer, case_eqv_table));

  specpdl_ref count = SPECPDL_INDEX ();
  struct regexp_cache *cache_entry
    = compile_pattern (regexp, modify_match_data ? &search_regs : NULL,
                       (!NILP (Vcase_fold_search)
                          ? BVAR (current_buffer, case_canon_table)
                          : Qnil),
                       posix, STRING_MULTIBYTE (string));
  freeze_pattern (cache_entry);
  re_match_object = string;
  val = re_search (&cache_entry->buf, SSDATA (string), SBYTES (string),
                   pos_byte, SBYTES (string) - pos_byte,
                   (modify_match_data ? &search_regs : NULL));
  unbind_to (count, Qnil);

  if (modify_match_data)
    last_thing_searched = Qt;

  if (val == -2)
    matcher_overflow ();
  if (val < 0)
    return Qnil;

  if (modify_match_data)
    for (i = 0; i < search_regs.num_regs; i++)
      if (search_regs.start[i] >= 0)
        {
          search_regs.start[i]
            = string_byte_to_char (string, search_regs.start[i]);
          search_regs.end[i] = string_byte_to_char (string, search_regs.end[i]);
        }

  return make_fixnum (string_byte_to_char (string, val));
}

DEFUN ("string-match", Fstring_match, Sstring_match, 2, 4, 0,
       doc: /* Return index of start of first match for REGEXP in STRING, or nil.
Matching ignores case if `case-fold-search' is non-nil.
If third arg START is non-nil, start search at that index in STRING.

If INHIBIT-MODIFY is non-nil, match data is not changed.

If INHIBIT-MODIFY is nil or missing, match data is changed, and
`match-end' and `match-beginning' give indices of substrings matched
by parenthesis constructs in the pattern.  You can use the function
`match-string' to extract the substrings matched by the parenthesis
constructions in REGEXP.  For index of first char beyond the match, do
(match-end 0).  */)
(Lisp_Object regexp, Lisp_Object string, Lisp_Object start,
 Lisp_Object inhibit_modify)
{
  return string_match_1 (regexp, string, start, 0, NILP (inhibit_modify));
}

static Lisp_Object
match_limit (Lisp_Object num, bool beginningp)
{
  EMACS_INT n;

  CHECK_FIXNUM (num);
  n = XFIXNUM (num);
  if (n < 0)
    args_out_of_range (num, make_fixnum (0));
  if (search_regs.num_regs <= 0)
    error ("No match data, because no search succeeded");
  if (n >= search_regs.num_regs || search_regs.start[n] < 0)
    return Qnil;
  return (
    make_fixnum ((beginningp) ? search_regs.start[n] : search_regs.end[n]));
}

static void
set_search_regs (ptrdiff_t beg_byte, ptrdiff_t nbytes)
{
  ptrdiff_t i;

  if (!NILP (Vinhibit_changing_match_data))
    return;

  if (search_regs.num_regs == 0)
    {
      search_regs.start = xmalloc (2 * sizeof *search_regs.start);
      search_regs.end = xmalloc (2 * sizeof *search_regs.end);
      search_regs.num_regs = 2;
    }

  for (i = 1; i < search_regs.num_regs; i++)
    {
      search_regs.start[i] = -1;
      search_regs.end[i] = -1;
    }

  search_regs.start[0] = BYTE_TO_CHAR (beg_byte);
  search_regs.end[0] = BYTE_TO_CHAR (beg_byte + nbytes);
  XSETBUFFER (last_thing_searched, current_buffer);
}

static bool
trivial_regexp_p (Lisp_Object regexp)
{
  ptrdiff_t len = SBYTES (regexp);
  unsigned char *s = SDATA (regexp);
  while (--len >= 0)
    {
      switch (*s++)
        {
        case '.':
        case '*':
        case '+':
        case '?':
        case '[':
        case '^':
        case '$':
          return 0;
        case '\\':
          if (--len < 0)
            return 0;
          switch (*s++)
            {
            case '|':
            case '(':
            case ')':
            case '`':
            case '\'':
            case 'b':
            case 'B':
            case '<':
            case '>':
            case 'w':
            case 'W':
            case 's':
            case 'S':
            case '=':
            case '{':
            case '}':
            case '_':
            case 'c':
            case 'C':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
              return 0;
            }
        }
    }
  return 1;
}

static void
freeze_buffer_relocation (void)
{
}

static struct re_registers search_regs_1;

static EMACS_INT
search_buffer_re (Lisp_Object string, ptrdiff_t pos, ptrdiff_t pos_byte,
                  ptrdiff_t lim, ptrdiff_t lim_byte, EMACS_INT n,
                  Lisp_Object trt, Lisp_Object inverse_trt, bool posix)
{
  ptrdiff_t s1;

  bool preserve_match_data = NILP (Vinhibit_changing_match_data);

  struct regexp_cache *cache_entry
    = compile_pattern (string,
                       preserve_match_data ? &search_regs : &search_regs_1, trt,
                       posix,
                       !NILP (
                         BVAR (current_buffer, enable_multibyte_characters)));
  struct re_pattern_buffer *bufp = &cache_entry->buf;

  maybe_quit ();

  s1 = ZV_BYTE - BEG_BYTE;
  // TODO: somehow implement chunkwise(linewise) search system
  //  instead of copying the whole buffer into a char[]
  unsigned char p1[s1];
  unrecoverable_error = true;
  buffer_memcpy (p1, 1, s1);

  specpdl_ref count = SPECPDL_INDEX ();
  freeze_buffer_relocation ();
  freeze_pattern (cache_entry);

  while (n < 0)
    {
      ptrdiff_t val;

      re_match_object = Qnil;
      val = re_search (bufp, (char *) p1, s1, pos_byte - BEGV_BYTE,
                       lim_byte - pos_byte,
                       preserve_match_data ? &search_regs : &search_regs_1);
      if (val == -2)
        {
          unbind_to (count, Qnil);
          matcher_overflow ();
        }
      if (val >= 0)
        {
          if (preserve_match_data)
            {
              pos_byte = search_regs.start[0] + BEGV_BYTE;
              for (ptrdiff_t i = 0; i < search_regs.num_regs; i++)
                if (search_regs.start[i] >= 0)
                  {
                    search_regs.start[i]
                      = BYTE_TO_CHAR (search_regs.start[i] + BEGV_BYTE);
                    search_regs.end[i]
                      = BYTE_TO_CHAR (search_regs.end[i] + BEGV_BYTE);
                  }
              XSETBUFFER (last_thing_searched, current_buffer);

              pos = search_regs.start[0];
            }
          else
            {
              pos_byte = search_regs_1.start[0] + BEGV_BYTE;

              pos = BYTE_TO_CHAR (search_regs_1.start[0] + BEGV_BYTE);
            }
        }
      else
        {
          unbind_to (count, Qnil);
          return (n);
        }
      n++;
      maybe_quit ();
    }
  while (n > 0)
    {
      ptrdiff_t val;

      re_match_object = Qnil;
      val = re_search (bufp, (char *) p1, s1, pos_byte - BEGV_BYTE,
                       lim_byte - pos_byte,
                       preserve_match_data ? &search_regs : &search_regs_1);
      if (val == -2)
        {
          unbind_to (count, Qnil);
          matcher_overflow ();
        }
      if (val >= 0)
        {
          if (preserve_match_data)
            {
              pos_byte = search_regs.end[0] + BEGV_BYTE;
              for (ptrdiff_t i = 0; i < search_regs.num_regs; i++)
                if (search_regs.start[i] >= 0)
                  {
                    search_regs.start[i]
                      = BYTE_TO_CHAR (search_regs.start[i] + BEGV_BYTE);
                    search_regs.end[i]
                      = BYTE_TO_CHAR (search_regs.end[i] + BEGV_BYTE);
                  }
              XSETBUFFER (last_thing_searched, current_buffer);
              pos = search_regs.end[0];
            }
          else
            {
              pos_byte = search_regs_1.end[0] + BEGV_BYTE;
              pos = BYTE_TO_CHAR (search_regs_1.end[0] + BEGV_BYTE);
            }
        }
      else
        {
          unbind_to (count, Qnil);
          return (0 - n);
        }
      n--;
      maybe_quit ();
    }
  unbind_to (count, Qnil);
  return (pos);
}

EMACS_INT
search_buffer (Lisp_Object string, ptrdiff_t pos, ptrdiff_t pos_byte,
               ptrdiff_t lim, ptrdiff_t lim_byte, EMACS_INT n, bool RE,
               Lisp_Object trt, Lisp_Object inverse_trt, bool posix)
{
  if (running_asynch_code)
    TODO; // save_search_regs ();

  if (n == 0 || SCHARS (string) == 0)
    {
      set_search_regs (pos_byte, 0);
      return pos;
    }

  if (RE && !(trivial_regexp_p (string) && NILP (Vsearch_spaces_regexp)))
    pos = search_buffer_re (string, pos, pos_byte, lim, lim_byte, n, trt,
                            inverse_trt, posix);
  else
    TODO; // pos = search_buffer_non_re (string, pos, pos_byte, lim, lim_byte,
          // n, RE,
  //                             trt, inverse_trt, posix);

  return pos;
}

static Lisp_Object
search_command (Lisp_Object string, Lisp_Object bound, Lisp_Object noerror,
                Lisp_Object count, int direction, bool RE, bool posix)
{
  EMACS_INT np;
  EMACS_INT lim;
  ptrdiff_t lim_byte;
  EMACS_INT n = direction;

  if (!NILP (count))
    {
      CHECK_FIXNUM (count);
      n *= XFIXNUM (count);
    }

  CHECK_STRING (string);
  if (NILP (bound))
    {
      if (n > 0)
        lim = ZV, lim_byte = ZV_BYTE;
      else
        lim = BEGV, lim_byte = BEGV_BYTE;
    }
  else
    {
      lim = fix_position (bound);
      if (n > 0 ? lim < PT : lim > PT)
        error ("Invalid search bound (wrong side of point)");
      if (lim > ZV)
        lim = ZV, lim_byte = ZV_BYTE;
      else if (lim < BEGV)
        lim = BEGV, lim_byte = BEGV_BYTE;
      else
        lim_byte = CHAR_TO_BYTE (lim);
    }

  set_char_table_extras (BVAR (current_buffer, case_canon_table), 2,
                         BVAR (current_buffer, case_eqv_table));

  np = search_buffer (string, PT, PT_BYTE, lim, lim_byte, n, RE,
                      (!NILP (Vcase_fold_search)
                         ? BVAR (current_buffer, case_canon_table)
                         : Qnil),
                      (!NILP (Vcase_fold_search)
                         ? BVAR (current_buffer, case_eqv_table)
                         : Qnil),
                      posix);
  if (np <= 0)
    {
      if (NILP (noerror))
        TODO; // xsignal1 (Qsearch_failed, string);

      if (!EQ (noerror, Qt))
        {
          eassert (BEGV <= lim && lim <= ZV);
          TODO; // SET_PT_BOTH (lim, lim_byte);
          return Qnil;
        }
      else
        return Qnil;
    }

  eassert (BEGV <= np && np <= ZV);
  SET_PT (np);

  return make_fixnum (np);
}

DEFUN ("re-search-forward", Fre_search_forward, Sre_search_forward, 1, 4,
       "sRE search: ",
       doc: /* Search forward from point for regular expression REGEXP.
Set point to the end of the occurrence found, and return point.
The optional second argument BOUND is a buffer position that bounds
  the search.  The match found must not end after that position.  A
  value of nil means search to the end of the accessible portion of
  the buffer.
The optional third argument NOERROR indicates how errors are handled
  when the search fails: if it is nil or omitted, emit an error; if
  it is t, simply return nil and do nothing; if it is neither nil nor
  t, move to the limit of search and return nil.
The optional fourth argument COUNT is a number that indicates the
  search direction and the number of occurrences to search for.  If it
  is positive, search forward for COUNT successive occurrences; if it
  is negative, search backward, instead of forward, for -COUNT
  occurrences.  A value of nil means the same as 1.
With COUNT positive/negative, the match found is the COUNTth/-COUNTth
  one in the buffer located entirely after/before the origin of the
  search.

Search case-sensitivity is determined by the value of the variable
`case-fold-search', which see.

See also the functions `match-beginning', `match-end', `match-string',
and `replace-match'.  */)
(Lisp_Object regexp, Lisp_Object bound, Lisp_Object noerror, Lisp_Object count)
{
  return search_command (regexp, bound, noerror, count, 1, true, false);
}

DEFUN ("replace-match", Freplace_match, Sreplace_match, 1, 5, 0,
       doc: /* Replace text matched by last search with NEWTEXT.
Leave point at the end of the replacement text.

If optional second arg FIXEDCASE is non-nil, do not alter the case of
the replacement text.  Otherwise, maybe capitalize the whole text, or
maybe just word initials, based on the replaced text.  If the replaced
text has only capital letters and has at least one multiletter word,
convert NEWTEXT to all caps.  Otherwise if all words are capitalized
in the replaced text, capitalize each word in NEWTEXT.  Note that
what exactly is a word is determined by the syntax tables in effect
in the current buffer, and the variable `case-symbols-as-words'.

If optional third arg LITERAL is non-nil, insert NEWTEXT literally.
Otherwise treat `\\' as special:
  `\\&' in NEWTEXT means substitute original matched text.
  `\\N' means substitute what matched the Nth `\\(...\\)'.
       If Nth parens didn't match, substitute nothing.
  `\\\\' means insert one `\\'.
  `\\?' is treated literally
       (for compatibility with `query-replace-regexp').
  Any other character following `\\' signals an error.
Case conversion does not apply to these substitutions.

If optional fourth argument STRING is non-nil, it should be a string
to act on; this should be the string on which the previous match was
done via `string-match'.  In this case, `replace-match' creates and
returns a new string, made by copying STRING and replacing the part of
STRING that was matched (the original STRING itself is not altered).

The optional fifth argument SUBEXP specifies a subexpression;
it says to replace just that subexpression with NEWTEXT,
rather than replacing the entire matched text.
This is, in a vague sense, the inverse of using `\\N' in NEWTEXT;
`\\N' copies subexp N into NEWTEXT, but using N as SUBEXP puts
NEWTEXT in place of subexp N.
This is useful only after a regular expression search or match,
since only regular expressions have distinguished subexpressions.  */)
(Lisp_Object newtext, Lisp_Object fixedcase, Lisp_Object literal,
 Lisp_Object string, Lisp_Object subexp)
{
  enum
  {
    nochange,
    all_caps,
    cap_initial
  } case_action;
  ptrdiff_t pos, pos_byte;
  bool some_multiletter_word;
  bool some_lowercase;
  bool some_uppercase;
  bool some_nonuppercase_initial;
  int c, prevc;
  ptrdiff_t sub;
  ptrdiff_t opoint, newpoint;
  UNUSED (newpoint);

  CHECK_STRING (newtext);

  if (!NILP (string))
    CHECK_STRING (string);

  if (NILP (literal) && !memchr (SSDATA (newtext), '\\', SBYTES (newtext)))
    literal = Qt;

  case_action = nochange;

  ptrdiff_t num_regs = search_regs.num_regs;
  if (num_regs <= 0)
    error ("`replace-match' called before any match found");

  sub = !NILP (subexp) ? check_integer_range (subexp, 0, num_regs - 1) : 0;
  ptrdiff_t sub_start = search_regs.start[sub];
  ptrdiff_t sub_end = search_regs.end[sub];
  eassert (sub_start <= sub_end);

  if (!(NILP (string) ? BEGV <= sub_start && sub_end <= ZV
                      : 0 <= sub_start && sub_end <= SCHARS (string)))
    {
      if (sub_start < 0)
        xsignal2 (Qerror,
                  build_string ("replace-match subexpression does not exist"),
                  subexp);
      args_out_of_range (make_fixnum (sub_start), make_fixnum (sub_end));
    }

  if (NILP (fixedcase))
    TODO;

  if (!NILP (string))
    {
      Lisp_Object before, after;

      before = Fsubstring (string, make_fixnum (0), make_fixnum (sub_start));
      after = Fsubstring (string, make_fixnum (sub_end), Qnil);

      if (NILP (literal))
        {
          ptrdiff_t lastpos = 0;
          ptrdiff_t lastpos_byte = 0;
          UNUSED (lastpos_byte);

          Lisp_Object accum;
          Lisp_Object middle;
          ptrdiff_t length = SBYTES (newtext);

          accum = Qnil;

          for (pos_byte = 0, pos = 0; pos_byte < length;)
            {
              ptrdiff_t substart = -1;
              ptrdiff_t subend = 0;
              bool delbackslash = 0;

              c = fetch_string_char_advance (newtext, &pos, &pos_byte);

              if (c == '\\')
                {
                  c = fetch_string_char_advance (newtext, &pos, &pos_byte);

                  if (c == '&')
                    {
                      substart = sub_start;
                      subend = sub_end;
                    }
                  else if (c >= '1' && c <= '9')
                    {
                      if (c - '0' < num_regs && search_regs.start[c - '0'] >= 0)
                        {
                          substart = search_regs.start[c - '0'];
                          subend = search_regs.end[c - '0'];
                        }
                      else
                        {
                          substart = 0;
                          subend = 0;
                        }
                    }
                  else if (c == '\\')
                    delbackslash = 1;
                  else if (c != '?')
                    error ("Invalid use of `\\' in replacement text");
                }
              if (substart >= 0)
                {
                  if (pos - 2 != lastpos)
                    middle = substring_both (newtext, lastpos, lastpos_byte,
                                             pos - 2, pos_byte - 2);
                  else
                    middle = Qnil;
                  accum = concat3 (accum, middle,
                                   Fsubstring (string, make_fixnum (substart),
                                               make_fixnum (subend)));
                  lastpos = pos;
                  lastpos_byte = pos_byte;
                }
              else if (delbackslash)
                {
                  middle = substring_both (newtext, lastpos, lastpos_byte,
                                           pos - 1, pos_byte - 1);

                  accum = concat2 (accum, middle);
                  lastpos = pos;
                  lastpos_byte = pos_byte;
                }
            }

          if (pos != lastpos)
            middle
              = substring_both (newtext, lastpos, lastpos_byte, pos, pos_byte);
          else
            middle = Qnil;

          newtext = concat2 (accum, middle);
        }

      if (case_action == all_caps)
        newtext = Fupcase (newtext);
      else if (case_action == cap_initial)
        newtext = Fupcase_initials (newtext);

      return concat3 (before, newtext, after);
    }

  TODO;
}

DEFUN ("match-beginning", Fmatch_beginning, Smatch_beginning, 1, 1, 0,
       doc: /* Return position of start of text matched by last search.
SUBEXP, a number, specifies the parenthesized subexpression in the last
  regexp for which to return the start position.
Value is nil if SUBEXPth subexpression didn't match, or there were fewer
  than SUBEXP subexpressions.
SUBEXP zero means the entire text matched by the whole regexp or whole
  string.

Return value is undefined if the last search failed.  */)
(Lisp_Object subexp) { return match_limit (subexp, 1); }

DEFUN ("match-end", Fmatch_end, Smatch_end, 1, 1, 0,
       doc: /* Return position of end of text matched by last search.
SUBEXP, a number, specifies the parenthesized subexpression in the last
  regexp for which to return the start position.
Value is nil if SUBEXPth subexpression didn't match, or there were fewer
  than SUBEXP subexpressions.
SUBEXP zero means the entire text matched by the whole regexp or whole
  string.

Return value is undefined if the last search failed.  */)
(Lisp_Object subexp) { return match_limit (subexp, 0); }

DEFUN ("match-data", Fmatch_data, Smatch_data, 0, 3, 0,
       doc: /* Return a list of positions that record text matched by the last search.
Element 2N of the returned list is the position of the beginning of the
match of the Nth subexpression; it corresponds to `(match-beginning N)';
element 2N + 1 is the position of the end of the match of the Nth
subexpression; it corresponds to `(match-end N)'.  See `match-beginning'
and `match-end'.
If the last search was on a buffer, all the elements are by default
markers or nil (nil when the Nth pair didn't match); they are integers
or nil if the search was on a string.  But if the optional argument
INTEGERS is non-nil, the elements that represent buffer positions are
always integers, not markers, and (if the search was on a buffer) the
buffer itself is appended to the list as one additional element.

Use `set-match-data' to reinstate the match data from the elements of
this list.

Note that non-matching optional groups at the end of the regexp are
elided instead of being represented with two `nil's each.  For instance:

  (progn
    (string-match "^\\(a\\)?\\(b\\)\\(c\\)?$" "b")
    (match-data))
  => (0 1 nil nil 0 1)

If REUSE is a list, store the value in REUSE by destructively modifying it.
If REUSE is long enough to hold all the values, its length remains the
same, and any unused elements are set to nil.  If REUSE is not long
enough, it is extended.  Note that if REUSE is long enough and INTEGERS
is non-nil, no consing is done to make the return value; this minimizes GC.

If optional third argument RESEAT is non-nil, any previous markers on the
REUSE list will be modified to point to nowhere.

Return value is undefined if the last search failed.  */)
(Lisp_Object integers, Lisp_Object reuse, Lisp_Object reseat)
{
  Lisp_Object tail, prev;
  Lisp_Object *data;
  ptrdiff_t i, len;

  if (!NILP (reseat))
    for (tail = reuse; CONSP (tail); tail = XCDR (tail))
      if (MARKERP (XCAR (tail)))
        {
          TODO;
        }

  if (NILP (last_thing_searched))
    return Qnil;

  prev = Qnil;

  USE_SAFE_ALLOCA;
  SAFE_NALLOCA (data, 1, 2 * search_regs.num_regs + 1);

  len = 0;
  for (i = 0; i < search_regs.num_regs; i++)
    {
      ptrdiff_t start = search_regs.start[i];
      if (start >= 0)
        {
          if (BASE_EQ (last_thing_searched, Qt) || !NILP (integers))
            {
              XSETFASTINT (data[2 * i], start);
              XSETFASTINT (data[2 * i + 1], search_regs.end[i]);
            }
          else if (BUFFERP (last_thing_searched))
            {
              TODO;
            }
          else
            emacs_abort ();

          len = 2 * i + 2;
        }
      else
        data[2 * i] = data[2 * i + 1] = Qnil;
    }

  if (BUFFERP (last_thing_searched) && !NILP (integers))
    {
      data[len] = last_thing_searched;
      len++;
    }

  if (!CONSP (reuse))
    reuse = Flist (len, data);
  else
    {
      for (i = 0, tail = reuse; CONSP (tail); i++, tail = XCDR (tail))
        {
          if (i < len)
            XSETCAR (tail, data[i]);
          else
            XSETCAR (tail, Qnil);
          prev = tail;
        }

      if (i < len)
        XSETCDR (prev, Flist (len - i, data + i));
    }

  SAFE_FREE ();
  return reuse;
}

DEFUN ("set-match-data", Fset_match_data, Sset_match_data, 1, 2, 0,
       doc: /* Set internal data on last search match from elements of LIST.
LIST should have been created by calling `match-data' previously.

If optional arg RESEAT is non-nil, make markers on LIST point nowhere.  */)
(register Lisp_Object list, Lisp_Object reseat)
{
  ptrdiff_t i;
  register Lisp_Object marker;

  if (running_asynch_code)
    TODO; // save_search_regs ();

  CHECK_LIST (list);

  last_thing_searched = Qt;

  {
    ptrdiff_t length = list_length (list) / 2;

    if (length > search_regs.num_regs)
      {
        ptrdiff_t num_regs = search_regs.num_regs;
        search_regs.start
          = xpalloc (search_regs.start, &num_regs, length - num_regs,
                     min (PTRDIFF_MAX, UINT_MAX), sizeof *search_regs.start);
        search_regs.end
          = xrealloc (search_regs.end, num_regs * sizeof *search_regs.end);

        for (i = search_regs.num_regs; i < num_regs; i++)
          search_regs.start[i] = -1;

        search_regs.num_regs = num_regs;
      }

    for (i = 0; CONSP (list); i++)
      {
        marker = XCAR (list);
        if (BUFFERP (marker))
          {
            last_thing_searched = marker;
            break;
          }
        if (i >= length)
          break;
        if (NILP (marker))
          {
            search_regs.start[i] = -1;
            list = XCDR (list);
          }
        else
          {
            Lisp_Object from;
            Lisp_Object m;

            m = marker;
            if (MARKERP (marker))
              TODO;

            CHECK_FIXNUM_COERCE_MARKER (marker);
            from = marker;

            if (!NILP (reseat) && MARKERP (m))
              {
                TODO; // unchain_marker (XMARKER (m));
                XSETCAR (list, Qnil);
              }

            if ((list = XCDR (list), !CONSP (list)))
              break;

            m = marker = XCAR (list);

            if (MARKERP (marker) && (TODO, false))
              XSETFASTINT (marker, 0);

            CHECK_FIXNUM_COERCE_MARKER (marker);
            if (PTRDIFF_MIN <= XFIXNUM (from) && XFIXNUM (from) <= PTRDIFF_MAX
                && PTRDIFF_MIN <= XFIXNUM (marker)
                && XFIXNUM (marker) <= PTRDIFF_MAX)
              {
                search_regs.start[i] = XFIXNUM (from);
                search_regs.end[i] = XFIXNUM (marker);
              }
            else
              {
                search_regs.start[i] = -1;
              }

            if (!NILP (reseat) && MARKERP (m))
              {
                TODO; // unchain_marker (XMARKER (m));
                XSETCAR (list, Qnil);
              }
          }
        list = XCDR (list);
      }

    for (; i < search_regs.num_regs; i++)
      search_regs.start[i] = -1;
  }

  return Qnil;
}

DEFUN ("match-data--translate", Fmatch_data__translate, Smatch_data__translate,
       1, 1, 0,
       doc: /* Add N to all positions in the match data.  Internal.  */)
(Lisp_Object n)
{
  CHECK_FIXNUM (n);
  EMACS_INT delta = XFIXNUM (n);
  if (!NILP (last_thing_searched))
    for (ptrdiff_t i = 0; i < search_regs.num_regs; i++)
      if (search_regs.start[i] >= 0)
        {
          search_regs.start[i] = max (0, search_regs.start[i] + delta);
          search_regs.end[i] = max (0, search_regs.end[i] + delta);
        }
  return Qnil;
}

static void
unwind_set_match_data (Lisp_Object list)
{
  Fset_match_data (list, Qt);
}

void
record_unwind_save_match_data (void)
{
  record_unwind_protect (unwind_set_match_data, Fmatch_data (Qnil, Qnil, Qnil));
}

static void
syms_of_search_for_pdumper (void)
{
  for (int i = 0; i < REGEXP_CACHE_SIZE; ++i)
    {
      searchbufs[i].buf.allocated = 100;
      searchbufs[i].buf.buffer = xmalloc (100);
      searchbufs[i].buf.fastmap = searchbufs[i].fastmap;
      searchbufs[i].regexp = Qnil;
      searchbufs[i].f_whitespace_regexp = Qnil;
      searchbufs[i].busy = false;
      searchbufs[i].syntax_table = Qnil;
      searchbufs[i].next
        = (i == REGEXP_CACHE_SIZE - 1 ? 0 : &searchbufs[i + 1]);
    }
  searchbuf_head = &searchbufs[0];
}

void
syms_of_search (void)
{
  for (int i = 0; i < REGEXP_CACHE_SIZE; ++i)
    {
      staticpro (&searchbufs[i].regexp);
      staticpro (&searchbufs[i].f_whitespace_regexp);
      staticpro (&searchbufs[i].syntax_table);
    }

  DEFSYM (Qinvalid_regexp, "invalid-regexp");

  Fput (Qinvalid_regexp, Qerror_conditions,
        pure_list (Qinvalid_regexp, Qerror));
  Fput (Qinvalid_regexp, Qerror_message,
        build_pure_c_string ("Invalid regexp"));

  re_match_object = Qnil;
  staticpro (&re_match_object);

  DEFVAR_LISP ("search-spaces-regexp", Vsearch_spaces_regexp,
      doc: /* Regexp to substitute for bunches of spaces in regexp search.
Some commands use this for user-specified regexps.
Spaces that occur inside character classes or repetition operators
or other such regexp constructs are not replaced with this.
A value of nil (which is the normal value) means treat spaces
literally.  Note that a value with capturing groups can change the
numbering of existing capture groups in unexpected ways.  */);
  Vsearch_spaces_regexp = Qnil;

  DEFVAR_LISP ("inhibit-changing-match-data", Vinhibit_changing_match_data,
      doc: /* Internal use only.
If non-nil, the primitive searching and matching functions
such as `looking-at', `string-match', `re-search-forward', etc.,
do not set the match data.  The proper way to use this variable
is to bind it with `let' around a small expression.  */);
  Vinhibit_changing_match_data = Qnil;

  defsubr (&Sregexp_quote);
  defsubr (&Sstring_match);
  defsubr (&Sre_search_forward);
  defsubr (&Sreplace_match);
  defsubr (&Smatch_beginning);
  defsubr (&Smatch_end);
  defsubr (&Smatch_data);
  defsubr (&Sset_match_data);
  defsubr (&Smatch_data__translate);
  syms_of_search_for_pdumper ();
}
