#include "lisp.h"
#include "buffer.h"
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

  defsubr (&Sstring_match);
  defsubr (&Smatch_beginning);
  defsubr (&Smatch_end);
  defsubr (&Smatch_data);
  defsubr (&Sset_match_data);
  syms_of_search_for_pdumper ();
}
