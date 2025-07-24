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

static bool
match_regexps (Lisp_Object string, Lisp_Object regexps, bool ignore_case)
{
  ptrdiff_t val;
  for (; CONSP (regexps); regexps = XCDR (regexps))
    TODO;
  return true;
}

DEFUN ("all-completions", Fall_completions, Sall_completions, 2, 4, 0,
       doc: /* Search for partial matches of STRING in COLLECTION.

Test each possible completion specified by COLLECTION
to see if it begins with STRING.  The possible completions may be
strings or symbols.  Symbols are converted to strings before testing,
by using `symbol-name'.

The value is a list of all the possible completions that match STRING.

If COLLECTION is an alist, the keys (cars of elements) are the
possible completions.  If an element is not a cons cell, then the
element itself is the possible completion.
If COLLECTION is a hash-table, all the keys that are strings or symbols
are the possible completions.
If COLLECTION is an obarray, the names of all symbols in the obarray
are the possible completions.

COLLECTION can also be a function to do the completion itself.
It receives three arguments: STRING, PREDICATE and t.
Whatever it returns becomes the value of `all-completions'.

If optional third argument PREDICATE is non-nil, it must be a function
of one or two arguments, and is used to test each possible completion.
A possible completion is accepted only if PREDICATE returns non-nil.

The argument given to PREDICATE is either a string or a cons cell (whose
car is a string) from the alist, or a symbol from the obarray.
If COLLECTION is a hash-table, PREDICATE is called with two arguments:
the string key and the associated value.

To be acceptable, a possible completion must also match all the regexps
in `completion-regexp-list' (unless COLLECTION is a function, in
which case that function should itself handle `completion-regexp-list').

An obsolete optional fourth argument HIDE-SPACES is still accepted for
backward compatibility.  If non-nil, strings in COLLECTION that start
with a space are ignored unless STRING itself starts with a space.  */)
(Lisp_Object string, Lisp_Object collection, Lisp_Object predicate,
 Lisp_Object hide_spaces)
{
  Lisp_Object tail, elt, eltstring;
  Lisp_Object allmatches;
  if (VECTORP (collection))
    collection = check_obarray (collection);
  int type = (HASH_TABLE_P (collection)
                ? 3
                : (OBARRAYP (collection)
                     ? 2
                     : ((NILP (collection)
                         || (CONSP (collection) && !FUNCTIONP (collection)))
                          ? 1
                          : 0)));
  ptrdiff_t idx = 0;
  Lisp_Object bucket, tem, zero;

  CHECK_STRING (string);
  if (type == 0)
    return call3 (collection, string, predicate, Qt);
  allmatches = bucket = Qnil;
  zero = make_fixnum (0);

  tail = collection;
  obarray_iter_t obit;
  if (type == 2)
    obit = make_obarray_iter (XOBARRAY (collection));

  while (1)
    {
      if (type == 1)
        {
          if (!CONSP (tail))
            break;
          elt = XCAR (tail);
          eltstring = CONSP (elt) ? XCAR (elt) : elt;
          tail = XCDR (tail);
        }
      else if (type == 2)
        {
          if (obarray_iter_at_end (&obit))
            break;
          elt = eltstring = obarray_iter_symbol (&obit);
          obarray_iter_step (&obit);
        }
      else /* if (type == 3) */
        {
          while (idx < HASH_TABLE_SIZE (XHASH_TABLE (collection))
                 && hash_unused_entry_key_p (
                   HASH_KEY (XHASH_TABLE (collection), idx)))
            idx++;
          if (idx >= HASH_TABLE_SIZE (XHASH_TABLE (collection)))
            break;
          else
            elt = eltstring = HASH_KEY (XHASH_TABLE (collection), idx++);
        }

      if (SYMBOLP (eltstring))
        eltstring = Fsymbol_name (eltstring);

      if (STRINGP (eltstring) && SCHARS (string) <= SCHARS (eltstring)
          && (NILP (hide_spaces)
              || (SBYTES (string) > 0 && SREF (string, 0) == ' ')
              || SREF (eltstring, 0) != ' ')
          && (tem = Fcompare_strings (eltstring, zero,
                                      make_fixnum (SCHARS (string)), string,
                                      zero, make_fixnum (SCHARS (string)),
                                      completion_ignore_case ? Qt : Qnil),
              EQ (Qt, tem)))
        {
          if (!match_regexps (eltstring, Vcompletion_regexp_list,
                              completion_ignore_case))
            continue;

          if (!NILP (predicate))
            {
              if (EQ (predicate, Qcommandp))
                tem = Fcommandp (elt, Qnil);
              else
                {
                  if (type == 3)
                    tem
                      = call2 (predicate, elt,
                               HASH_VALUE (XHASH_TABLE (collection), idx - 1));
                  else
                    tem = call1 (predicate, elt);
                }
              if (NILP (tem))
                continue;
            }
          allmatches = Fcons (eltstring, allmatches);
        }
    }

  return Fnreverse (allmatches);
}

void
syms_of_minibuf (void)
{
  defsubr (&Sassoc_string);
  defsubr (&Sall_completions);

  DEFVAR_LISP ("minibuffer-prompt-properties", Vminibuffer_prompt_properties,
        doc: /* Text properties that are added to minibuffer prompts.
These are in addition to the basic `field' property, and stickiness
properties.  */);
  Vminibuffer_prompt_properties = list2 (Qread_only, Qt);

  DEFVAR_BOOL ("completion-ignore-case", completion_ignore_case,
        doc: /* Non-nil means don't consider case significant in completion.
For file-name completion, `read-file-name-completion-ignore-case'
controls the behavior, rather than this variable.
For buffer name completion, `read-buffer-completion-ignore-case'
controls the behavior, rather than this variable.  */);
  completion_ignore_case = 0;

  DEFVAR_LISP ("completion-regexp-list", Vcompletion_regexp_list,
        doc: /* List of regexps that should restrict possible completions.
The basic completion functions only consider a completion acceptable
if it matches all regular expressions in this list, with
`case-fold-search' bound to the value of `completion-ignore-case'.
See Info node `(elisp)Basic Completion', for a description of these
functions.

Do not set this variable to a non-nil value globally, as that is not
safe and will probably cause errors in completion commands.  This
variable should be only let-bound to non-nil values around calls to
basic completion functions like `try-completion' and `all-completions'.  */);
  Vcompletion_regexp_list = Qnil;
}
