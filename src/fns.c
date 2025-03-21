#include "lisp.h"
#include "character.h"

enum equal_kind
{
  EQUAL_NO_QUIT,
  EQUAL_PLAIN,
  EQUAL_INCLUDING_PROPERTIES
};

ptrdiff_t
list_length (Lisp_Object list)
{
  intptr_t i = 0;
  FOR_EACH_TAIL (list)
    i++;
  CHECK_LIST_END (list, list);
  return i;
}

INLINE EMACS_UINT
sxhash_combine (EMACS_UINT x, EMACS_UINT y)
{
  return (x << 4) + (x >> (EMACS_INT_WIDTH - 4)) + y;
}

EMACS_UINT
hash_string (char const *ptr, ptrdiff_t len)
{
  char const *p = ptr;
  char const *end = ptr + len;
  EMACS_UINT hash = len;
  ptrdiff_t step = max ((long) sizeof hash, ((end - p) >> 3));

  if (p + sizeof hash <= end)
    {
      do
        {
          EMACS_UINT c;
          memcpy (&c, p, sizeof hash);
          p += step;
          hash = sxhash_combine (hash, c);
        }
      while (p + sizeof hash <= end);
      EMACS_UINT c;
      memcpy (&c, end - sizeof c, sizeof c);
      hash = sxhash_combine (hash, c);
    }
  else
    {
      eassume (p <= end && end - p < (long) sizeof (EMACS_UINT));
      EMACS_UINT tail = 0;
#if EMACS_INT_MAX > INT32_MAX
      if (end - p >= 4)
        {
          uint32_t c;
          memcpy (&c, p, sizeof c);
          tail = (tail << (8 * sizeof c)) + c;
          p += sizeof c;
        }
#endif
      if (end - p >= 2)
        {
          uint16_t c;
          memcpy (&c, p, sizeof c);
          tail = (tail << (8 * sizeof c)) + c;
          p += sizeof c;
        }
      if (p < end)
        tail = (tail << 8) + (unsigned char) *p;
      hash = sxhash_combine (hash, tail);
    }

  return hash;
}
static Lisp_Object string_char_byte_cache_string;
static ptrdiff_t string_char_byte_cache_charpos;
static ptrdiff_t string_char_byte_cache_bytepos;
ptrdiff_t
string_char_to_byte (Lisp_Object string, ptrdiff_t char_index)
{
  ptrdiff_t i_byte;
  ptrdiff_t best_below, best_below_byte;
  ptrdiff_t best_above, best_above_byte;

  best_below = best_below_byte = 0;
  best_above = SCHARS (string);
  best_above_byte = SBYTES (string);
  if (best_above == best_above_byte)
    return char_index;

  if (BASE_EQ (string, string_char_byte_cache_string))
    {
      if (string_char_byte_cache_charpos < char_index)
        {
          best_below = string_char_byte_cache_charpos;
          best_below_byte = string_char_byte_cache_bytepos;
        }
      else
        {
          best_above = string_char_byte_cache_charpos;
          best_above_byte = string_char_byte_cache_bytepos;
        }
    }

  if (char_index - best_below < best_above - char_index)
    {
      unsigned char *p = SDATA (string) + best_below_byte;

      while (best_below < char_index)
        {
          p += BYTES_BY_CHAR_HEAD (*p);
          best_below++;
        }
      i_byte = p - SDATA (string);
    }
  else
    {
      unsigned char *p = SDATA (string) + best_above_byte;

      while (best_above > char_index)
        {
          p--;
          while (!CHAR_HEAD_P (*p))
            p--;
          best_above--;
        }
      i_byte = p - SDATA (string);
    }

  string_char_byte_cache_bytepos = i_byte;
  string_char_byte_cache_charpos = char_index;
  string_char_byte_cache_string = string;

  return i_byte;
}

Lisp_Object
plist_get (Lisp_Object plist, Lisp_Object prop)
{
  Lisp_Object tail = plist;
  FOR_EACH_TAIL_SAFE (tail)
    {
      if (!CONSP (XCDR (tail)))
        break;
      if (EQ (XCAR (tail), prop))
        return XCAR (XCDR (tail));
      tail = XCDR (tail);
    }
  return Qnil;
}
DEFUN ("get", Fget, Sget, 2, 2, 0,
       doc: /* Return the value of SYMBOL's PROPNAME property.
This is the last value stored with `(put SYMBOL PROPNAME VALUE)'.  */)
(Lisp_Object symbol, Lisp_Object propname)
{
  CHECK_SYMBOL (symbol);
#if TODO_NELISP_LATER_AND
  Lisp_Object propval
    = plist_get (CDR (Fassq (symbol, Voverriding_plist_environment)), propname);
  if (!NILP (propval))
    return propval;
#endif
  return plist_get (XSYMBOL (symbol)->u.s.plist, propname);
}

Lisp_Object
plist_put (Lisp_Object plist, Lisp_Object prop, Lisp_Object val)
{
  Lisp_Object prev = Qnil, tail = plist;
  FOR_EACH_TAIL (tail)
    {
      if (!CONSP (XCDR (tail)))
        break;

      if (EQ (XCAR (tail), prop))
        {
          Fsetcar (XCDR (tail), val);
          return plist;
        }

      prev = tail;
      tail = XCDR (tail);
    }
  CHECK_TYPE (NILP (tail), Qplistp, plist);
  Lisp_Object newcell
    = Fcons (prop, Fcons (val, NILP (prev) ? plist : XCDR (XCDR (prev))));
  if (NILP (prev))
    return newcell;
  Fsetcdr (XCDR (prev), newcell);
  return plist;
}
DEFUN ("put", Fput, Sput, 3, 3, 0,
       doc: /* Store SYMBOL's PROPNAME property with value VALUE.
It can be retrieved with `(get SYMBOL PROPNAME)'.  */)
(Lisp_Object symbol, Lisp_Object propname, Lisp_Object value)
{
  CHECK_SYMBOL (symbol);
  set_symbol_plist (symbol,
                    plist_put (XSYMBOL (symbol)->u.s.plist, propname, value));
  return value;
}

static bool
eq_comparable_value (Lisp_Object x)
{
  return SYMBOLP (x) || FIXNUMP (x);
}
DEFUN ("member", Fmember, Smember, 2, 2, 0,
       doc: /* Return non-nil if ELT is an element of LIST.  Comparison done with `equal'.
The value is actually the tail of LIST whose car is ELT.  */)
(Lisp_Object elt, Lisp_Object list)
{
  if (eq_comparable_value (elt))
    return Fmemq (elt, list);
  Lisp_Object tail = list;
  FOR_EACH_TAIL (tail)
    if (!NILP (Fequal (elt, XCAR (tail))))
      return tail;
  CHECK_LIST_END (tail, list);
  return Qnil;
}

DEFUN ("memq", Fmemq, Smemq, 2, 2, 0,
       doc: /* Return non-nil if ELT is an element of LIST.  Comparison done with `eq'.
The value is actually the tail of LIST whose car is ELT.  */)
(Lisp_Object elt, Lisp_Object list)
{
  Lisp_Object tail = list;
  FOR_EACH_TAIL (tail)
    if (EQ (XCAR (tail), elt))
      return tail;
  CHECK_LIST_END (tail, list);
  return Qnil;
}
DEFUN ("assq", Fassq, Sassq, 2, 2, 0,
       doc: /* Return non-nil if KEY is `eq' to the car of an element of ALIST.
The value is actually the first element of ALIST whose car is KEY.
Elements of ALIST that are not conses are ignored.  */)
(Lisp_Object key, Lisp_Object alist)
{
  Lisp_Object tail = alist;
  FOR_EACH_TAIL (tail)
    if (CONSP (XCAR (tail)) && EQ (XCAR (XCAR (tail)), key))
      return XCAR (tail);
  CHECK_LIST_END (tail, alist);
  return Qnil;
}

enum
{
  WORDS_PER_DOUBLE = (sizeof (double) / sizeof (EMACS_UINT)
                      + (sizeof (double) % sizeof (EMACS_UINT) != 0))
};
union double_and_words
{
  double val;
  EMACS_UINT word[WORDS_PER_DOUBLE];
};
static bool
same_float (Lisp_Object x, Lisp_Object y)
{
  union double_and_words xu = { .val = XFLOAT_DATA (x) },
                         yu = { .val = XFLOAT_DATA (y) };
  EMACS_UINT neql = 0;
  for (int i = 0; i < WORDS_PER_DOUBLE; i++)
    neql |= xu.word[i] ^ yu.word[i];
  return !neql;
}
bool equal_no_quit (Lisp_Object o1, Lisp_Object o2);
static bool
internal_equal (Lisp_Object o1, Lisp_Object o2, enum equal_kind equal_kind,
                int depth, Lisp_Object ht)
{
tail_recurse:
  if (depth > 10)
    {
      TODO;
    }

  o1 = maybe_remove_pos_from_symbol (o1);
  o2 = maybe_remove_pos_from_symbol (o2);

  if (BASE_EQ (o1, o2))
    return true;
  if (XTYPE (o1) != XTYPE (o2))
    return false;

  switch (XTYPE (o1))
    {
    case Lisp_Float:
      return same_float (o1, o2);

    case Lisp_Cons:
      if (equal_kind == EQUAL_NO_QUIT)
        for (; CONSP (o1); o1 = XCDR (o1))
          {
            if (!CONSP (o2))
              return false;
            if (!equal_no_quit (XCAR (o1), XCAR (o2)))
              return false;
            o2 = XCDR (o2);
            if (EQ (XCDR (o1), o2))
              return true;
          }
      else
        FOR_EACH_TAIL (o1)
          {
            if (!CONSP (o2))
              return false;
            if (!internal_equal (XCAR (o1), XCAR (o2), equal_kind, depth + 1,
                                 ht))
              return false;
            o2 = XCDR (o2);
            if (EQ (XCDR (o1), o2))
              return true;
          }
      depth++;
      goto tail_recurse;

    case Lisp_Vectorlike:
      {
        ptrdiff_t size = ASIZE (o1);
        if (ASIZE (o2) != size)
          return false;

        TODO;
        return true;
      }
      break;

    case Lisp_String:
      return (SCHARS (o1) == SCHARS (o2) && SBYTES (o1) == SBYTES (o2)
              && !memcmp (SDATA (o1), SDATA (o2), SBYTES (o1))
              && (equal_kind != EQUAL_INCLUDING_PROPERTIES || (TODO, false)));

    default:
      break;
    }

  return false;
}
bool
equal_no_quit (Lisp_Object o1, Lisp_Object o2)
{
  return internal_equal (o1, o2, EQUAL_NO_QUIT, 0, Qnil);
}
DEFUN ("equal", Fequal, Sequal, 2, 2, 0,
       doc: /* Return t if two Lisp objects have similar structure and contents.
They must have the same data type.
Conses are compared by comparing the cars and the cdrs.
Vectors and strings are compared element by element.
Numbers are compared via `eql', so integers do not equal floats.
\(Use `=' if you want integers and floats to be able to be equal.)
Symbols must match exactly.  */)
(Lisp_Object o1, Lisp_Object o2)
{
  return internal_equal (o1, o2, EQUAL_PLAIN, 0, Qnil) ? Qt : Qnil;
}

DEFUN ("nthcdr", Fnthcdr, Snthcdr, 2, 2, 0,
       doc: /* Take cdr N times on LIST, return the result.  */)
(Lisp_Object n, Lisp_Object list)
{
  Lisp_Object tail = list;

  CHECK_INTEGER (n);

  EMACS_INT num;
  if (FIXNUMP (n))
    {
      num = XFIXNUM (n);

      if (num <= SMALL_LIST_LEN_MAX)
        {
          for (; 0 < num; num--, tail = XCDR (tail))
            if (!CONSP (tail))
              {
                CHECK_LIST_END (tail, list);
                return Qnil;
              }
          return tail;
        }
    }
  else
    {
      TODO;
    }

  TODO;
}
DEFUN ("nth", Fnth, Snth, 2, 2, 0,
       doc: /* Return the Nth element of LIST.
N counts from zero.  If LIST is not that long, nil is returned.  */)
(Lisp_Object n, Lisp_Object list) { return Fcar (Fnthcdr (n, list)); }
DEFUN ("proper-list-p", Fproper_list_p, Sproper_list_p, 1, 1, 0,
       doc: /* Return OBJECT's length if it is a proper list, nil otherwise.
A proper list is neither circular nor dotted (i.e., its last cdr is nil).  */
       attributes: const)
(Lisp_Object object)
{
  intptr_t len = 0;
  Lisp_Object last_tail = object;
  Lisp_Object tail = object;
  FOR_EACH_TAIL_SAFE (tail)
    {
      len++;
      rarely_quit (len);
      last_tail = XCDR (tail);
    }
  if (!NILP (last_tail))
    return Qnil;
  return make_fixnum (len);
}

DEFUN ("nconc", Fnconc, Snconc, 0, MANY, 0,
       doc: /* Concatenate any number of lists by altering them.
Only the last argument is not altered, and need not be a list.
usage: (nconc &rest LISTS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  Lisp_Object val = Qnil;

  for (ptrdiff_t argnum = 0; argnum < nargs; argnum++)
    {
      Lisp_Object tem = args[argnum];
      if (NILP (tem))
        continue;

      if (NILP (val))
        val = tem;

      if (argnum + 1 == nargs)
        break;

      CHECK_CONS (tem);

      Lisp_Object tail UNINIT;
      FOR_EACH_TAIL (tem)
        tail = tem;

      tem = args[argnum + 1];
      Fsetcdr (tail, tem);
      if (NILP (tem))
        args[argnum + 1] = tail;
    }

  return val;
}
DEFUN ("nreverse", Fnreverse, Snreverse, 1, 1, 0,
       doc: /* Reverse order of items in a list, vector or string SEQ.
If SEQ is a list, it should be nil-terminated.
This function may destructively modify SEQ to produce the value.  */)
(Lisp_Object seq)
{
  if (NILP (seq))
    return seq;
  else if (CONSP (seq))
    {
      Lisp_Object prev, tail, next;

      for (prev = Qnil, tail = seq; CONSP (tail); tail = next)
        {
          next = XCDR (tail);
          if (BASE_EQ (next, seq))
            circular_list (seq);
          Fsetcdr (tail, prev);
          prev = tail;
        }
      CHECK_LIST_END (tail, seq);
      seq = prev;
    }
  else if (VECTORP (seq))
    {
      ptrdiff_t i, size = ASIZE (seq);

      for (i = 0; i < size / 2; i++)
        {
          Lisp_Object tem = AREF (seq, i);
          ASET (seq, i, AREF (seq, size - i - 1));
          ASET (seq, size - i - 1, tem);
        }
    }
  else if (BOOL_VECTOR_P (seq))
    {
      TODO;
    }
  else if (STRINGP (seq))
    TODO;
  else
    wrong_type_argument (Qarrayp, seq);
  return seq;
}

void
syms_of_fns (void)
{
  DEFSYM (Qplistp, "plistp");

  defsubr (&Sget);
  defsubr (&Smemq);
  defsubr (&Sassq);
  defsubr (&Sput);
  defsubr (&Smember);
  defsubr (&Sequal);
  defsubr (&Snthcdr);
  defsubr (&Snth);
  defsubr (&Sproper_list_p);
  defsubr (&Snconc);
  defsubr (&Snreverse);
}
