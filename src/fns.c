#include "lisp.h"
#include "bignum.h"
#include "character.h"
#include "composite.h"
#include "intervals.h"
#include "puresize.h"

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

static Lisp_Object
sort_list (Lisp_Object list, Lisp_Object predicate, Lisp_Object keyfunc,
           bool reverse, bool inplace)
{
  ptrdiff_t length = list_length (list);
  if (length < 2)
    return inplace ? list : list1 (XCAR (list));
  else
    {
      Lisp_Object *result;
      USE_SAFE_ALLOCA;
      SAFE_ALLOCA_LISP (result, length);
      Lisp_Object tail = list;
      for (ptrdiff_t i = 0; i < length; i++)
        {
          result[i] = Fcar (tail);
          tail = XCDR (tail);
        }
      tim_sort (predicate, keyfunc, result, length, reverse);

      if (inplace)
        {
          ptrdiff_t i = 0;
          tail = list;
          while (CONSP (tail))
            {
              XSETCAR (tail, result[i]);
              tail = XCDR (tail);
              i++;
            }
        }
      else
        {
          list = Qnil;
          for (ptrdiff_t i = length - 1; i >= 0; i--)
            list = Fcons (result[i], list);
        }
      SAFE_FREE ();
      return list;
    }
}

/* Stably sort VECTOR in-place ordered by PREDICATE and KEYFUNC,
   optionally reversed.  */
static Lisp_Object
sort_vector (Lisp_Object vector, Lisp_Object predicate, Lisp_Object keyfunc,
             bool reverse)
{
  ptrdiff_t length = ASIZE (vector);
  if (length >= 2)
    tim_sort (predicate, keyfunc, XVECTOR (vector)->contents, length, reverse);
  return vector;
}

DEFUN ("sort", Fsort, Ssort, 1, MANY, 0,
       doc: /* Sort SEQ, stably, and return the sorted sequence.
SEQ should be a list or vector.
Optional arguments are specified as keyword/argument pairs.  The following
arguments are defined:

:key FUNC -- FUNC is a function that takes a single element from SEQ and
  returns the key value to be used in comparison.  If absent or nil,
  `identity' is used.

:lessp FUNC -- FUNC is a function that takes two arguments and returns
  non-nil if the first element should come before the second.
  If absent or nil, `value<' is used.

:reverse BOOL -- if BOOL is non-nil, the sorting order implied by FUNC is
  reversed.  This does not affect stability: equal elements still retain
  their order in the input sequence.

:in-place BOOL -- if BOOL is non-nil, SEQ is sorted in-place and returned.
  Otherwise, a sorted copy of SEQ is returned and SEQ remains unmodified;
  this is the default.

For compatibility, the calling convention (sort SEQ LESSP) can also be used;
in this case, sorting is always done in-place.

usage: (sort SEQ &key KEY LESSP REVERSE IN-PLACE)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  Lisp_Object seq = args[0];
  Lisp_Object key = Qnil;
  Lisp_Object lessp = Qnil;
  bool inplace = false;
  bool reverse = false;
  if (nargs == 2)
    {
      lessp = args[1];
      inplace = true;
    }
  else if ((nargs & 1) == 0)
    error ("Invalid argument list");
  else
    for (ptrdiff_t i = 1; i < nargs - 1; i += 2)
      {
        if (EQ (args[i], QCkey))
          key = args[i + 1];
        else if (EQ (args[i], QClessp))
          lessp = args[i + 1];
        else if (EQ (args[i], QCin_place))
          inplace = !NILP (args[i + 1]);
        else if (EQ (args[i], QCreverse))
          reverse = !NILP (args[i + 1]);
        else
          signal_error ("Invalid keyword argument", args[i]);
      }

  if (CONSP (seq))
    return sort_list (seq, lessp, key, reverse, inplace);
  else if (NILP (seq))
    return seq;
  else if (VECTORP (seq))
    return sort_vector (inplace ? seq : Fcopy_sequence (seq), lessp, key,
                        reverse);
  else
    wrong_type_argument (Qlist_or_vector_p, seq);
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
DEFUN ("plist-get", Fplist_get, Splist_get, 2, 3, 0,
       doc: /* Extract a value from a property list.
PLIST is a property list, which is a list of the form
\(PROP1 VALUE1 PROP2 VALUE2...).

This function returns the value corresponding to the given PROP, or
nil if PROP is not one of the properties on the list.  The comparison
with PROP is done using PREDICATE, which defaults to `eq'.

This function doesn't signal an error if PLIST is invalid.  */)
(Lisp_Object plist, Lisp_Object prop, Lisp_Object predicate)
{
  if (NILP (predicate))
    return plist_get (plist, prop);

  Lisp_Object tail = plist;
  FOR_EACH_TAIL_SAFE (tail)
    {
      if (!CONSP (XCDR (tail)))
        break;
      if (!NILP (call2 (predicate, XCAR (tail), prop)))
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
DEFUN ("plist-put", Fplist_put, Splist_put, 3, 4, 0,
       doc: /* Change value in PLIST of PROP to VAL.
PLIST is a property list, which is a list of the form
\(PROP1 VALUE1 PROP2 VALUE2 ...).

The comparison with PROP is done using PREDICATE, which defaults to `eq'.

If PROP is already a property on the list, its value is set to VAL,
otherwise the new PROP VAL pair is added.  The new plist is returned;
use `(setq x (plist-put x prop val))' to be sure to use the new value.
The PLIST is modified by side effects.  */)
(Lisp_Object plist, Lisp_Object prop, Lisp_Object val, Lisp_Object predicate)
{
  if (NILP (predicate))
    return plist_put (plist, prop, val);
  Lisp_Object prev = Qnil, tail = plist;
  FOR_EACH_TAIL (tail)
    {
      if (!CONSP (XCDR (tail)))
        break;

      if (!NILP (call2 (predicate, XCAR (tail), prop)))
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
Lisp_Object
assq_no_quit (Lisp_Object key, Lisp_Object alist)
{
  for (; !NILP (alist); alist = XCDR (alist))
    if (CONSP (XCAR (alist)) && EQ (XCAR (XCAR (alist)), key))
      return XCAR (alist);
  return Qnil;
}

DEFUN ("assoc", Fassoc, Sassoc, 2, 3, 0,
       doc: /* Return non-nil if KEY is equal to the car of an element of ALIST.
The value is actually the first element of ALIST whose car equals KEY.

Equality is defined by the function TESTFN, defaulting to `equal'.
TESTFN is called with 2 arguments: a car of an alist element and KEY.  */)
(Lisp_Object key, Lisp_Object alist, Lisp_Object testfn)
{
  if (eq_comparable_value (key) && NILP (testfn))
    return Fassq (key, alist);
  Lisp_Object tail = alist;
  FOR_EACH_TAIL (tail)
    {
      Lisp_Object car = XCAR (tail);
      if (!CONSP (car))
        continue;
      if ((NILP (testfn)
             ? (EQ (XCAR (car), key) || !NILP (Fequal (XCAR (car), key)))
             : !NILP (call2 (testfn, XCAR (car), key))))
        return car;
    }
  CHECK_LIST_END (tail, alist);
  return Qnil;
}

DEFUN ("rassq", Frassq, Srassq, 2, 2, 0,
       doc: /* Return non-nil if KEY is `eq' to the cdr of an element of ALIST.
The value is actually the first element of ALIST whose cdr is KEY.  */)
(Lisp_Object key, Lisp_Object alist)
{
  Lisp_Object tail = alist;
  FOR_EACH_TAIL (tail)
    if (CONSP (XCAR (tail)) && EQ (XCDR (XCAR (tail)), key))
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

        if (BIGNUMP (o1))
          return mpz_cmp (*xbignum_val (o1), *xbignum_val (o2)) == 0;
        if (OVERLAYP (o1))
          {
            TODO;
          }
        if (MARKERP (o1))
          {
            TODO;
          }
        if (BOOL_VECTOR_P (o1))
          {
            EMACS_INT size = bool_vector_size (o1);
            return (size == bool_vector_size (o2)
                    && !memcmp (bool_vector_data (o1), bool_vector_data (o2),
                                bool_vector_bytes (size)));
          }

#ifdef HAVE_TREE_SITTER
        if (TS_NODEP (o1))
          return treesit_node_eq (o1, o2);
#endif
        if (SYMBOL_WITH_POS_P (o1))
          {
            eassert (!symbols_with_pos_enabled);
            return (
              BASE_EQ (XSYMBOL_WITH_POS_SYM (o1), XSYMBOL_WITH_POS_SYM (o2))
              && BASE_EQ (XSYMBOL_WITH_POS_POS (o1),
                          XSYMBOL_WITH_POS_POS (o2)));
          }

        if (size & PSEUDOVECTOR_FLAG)
          {
            if (((size & PVEC_TYPE_MASK) >> PSEUDOVECTOR_AREA_BITS)
                < PVEC_CLOSURE)
              return false;
            size &= PSEUDOVECTOR_SIZE_MASK;
          }
        for (ptrdiff_t i = 0; i < size; i++)
          {
            Lisp_Object v1, v2;
            v1 = AREF (o1, i);
            v2 = AREF (o2, i);
            if (!internal_equal (v1, v2, equal_kind, depth + 1, ht))
              return false;
          }
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
DEFUN ("eql", Feql, Seql, 2, 2, 0,
       doc: /* Return t if the two args are `eq' or are indistinguishable numbers.
Integers with the same value are `eql'.
Floating-point values with the same sign, exponent and fraction are `eql'.
This differs from numeric comparison: (eql 0.0 -0.0) returns nil and
\(eql 0.0e+NaN 0.0e+NaN) returns t, whereas `=' does the opposite.  */)
(Lisp_Object obj1, Lisp_Object obj2)
{
  if (FLOATP (obj1))
    return FLOATP (obj2) && same_float (obj1, obj2) ? Qt : Qnil;
  else if (BIGNUMP (obj1))
    return ((BIGNUMP (obj2)
             && mpz_cmp (*xbignum_val (obj1), *xbignum_val (obj2)) == 0)
              ? Qt
              : Qnil);
  else
    return EQ (obj1, obj2) ? Qt : Qnil;
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

DEFUN ("delq", Fdelq, Sdelq, 2, 2, 0,
       doc: /* Delete members of LIST which are `eq' to ELT, and return the result.
More precisely, this function skips any members `eq' to ELT at the
front of LIST, then removes members `eq' to ELT from the remaining
sublist by modifying its list structure, then returns the resulting
list.

Write `(setq foo (delq element foo))' to be sure of correctly changing
the value of a list `foo'.  See also `remq', which does not modify the
argument.  */)
(Lisp_Object elt, Lisp_Object list)
{
  Lisp_Object prev = Qnil, tail = list;

  FOR_EACH_TAIL (tail)
    {
      Lisp_Object tem = XCAR (tail);
      if (EQ (elt, tem))
        {
          if (NILP (prev))
            list = XCDR (tail);
          else
            Fsetcdr (prev, XCDR (tail));
        }
      else
        prev = tail;
    }
  CHECK_LIST_END (tail, list);
  return list;
}

static EMACS_INT
mapcar1 (EMACS_INT leni, Lisp_Object *vals, Lisp_Object fn, Lisp_Object seq)
{
  if (NILP (seq))
    return 0;
  else if (CONSP (seq))
    {
      Lisp_Object tail = seq;
      for (ptrdiff_t i = 0; i < leni; i++)
        {
          if (!CONSP (tail))
            return i;
          Lisp_Object dummy = call1 (fn, XCAR (tail));
          if (vals)
            vals[i] = dummy;
          tail = XCDR (tail);
        }
    }
  else if (VECTORP (seq) || CLOSUREP (seq))
    {
      for (ptrdiff_t i = 0; i < leni; i++)
        {
          Lisp_Object dummy = call1 (fn, AREF (seq, i));
          if (vals)
            vals[i] = dummy;
        }
    }
  else if (STRINGP (seq))
    {
      ptrdiff_t i_byte = 0;

      for (ptrdiff_t i = 0; i < leni;)
        {
          ptrdiff_t i_before = i;
          int c = fetch_string_char_advance (seq, &i, &i_byte);
          Lisp_Object dummy = call1 (fn, make_fixnum (c));
          if (vals)
            vals[i_before] = dummy;
        }
    }
  else
    {
      eassert (BOOL_VECTOR_P (seq));
      for (EMACS_INT i = 0; i < leni; i++)
        {
          TODO; // Lisp_Object dummy = call1 (fn, bool_vector_ref (seq, i));
          // if (vals)
          // vals[i] = dummy;
        }
    }

  return leni;
}

DEFUN ("mapconcat", Fmapconcat, Smapconcat, 2, 3, 0,
       doc: /* Apply FUNCTION to each element of SEQUENCE, and concat the results as strings.
In between each pair of results, stick in SEPARATOR.  Thus, " " as
  SEPARATOR results in spaces between the values returned by FUNCTION.

SEQUENCE may be a list, a vector, a bool-vector, or a string.

Optional argument SEPARATOR must be a string, a vector, or a list of
characters; nil stands for the empty string.

FUNCTION must be a function of one argument, and must return a value
  that is a sequence of characters: either a string, or a vector or
  list of numbers that are valid character codepoints; nil is treated
  as an empty string.  */)
(Lisp_Object function, Lisp_Object sequence, Lisp_Object separator)
{
  USE_SAFE_ALLOCA;
  EMACS_INT leni = XFIXNAT (Flength (sequence));
  if (CHAR_TABLE_P (sequence))
    wrong_type_argument (Qlistp, sequence);
  EMACS_INT args_alloc = 2 * leni - 1;
  if (args_alloc < 0)
    return empty_unibyte_string;
  Lisp_Object *args;
  SAFE_ALLOCA_LISP (args, args_alloc);
  if (EQ (function, Qidentity))
    {
      if (CONSP (sequence))
        {
          Lisp_Object src = sequence;
          Lisp_Object *dst = args;
          do
            {
              *dst++ = XCAR (src);
              src = XCDR (src);
            }
          while (!NILP (src));
          goto concat;
        }
      else if (VECTORP (sequence))
        {
          memcpy (args, XVECTOR (sequence)->contents, leni * sizeof *args);
          goto concat;
        }
    }
  ptrdiff_t nmapped = mapcar1 (leni, args, function, sequence);
  eassert (nmapped == leni);

concat:;
  ptrdiff_t nargs = args_alloc;
  if (NILP (separator) || (STRINGP (separator) && SCHARS (separator) == 0))
    nargs = leni;
  else
    {
      for (ptrdiff_t i = leni - 1; i > 0; i--)
        args[i + i] = args[i];

      for (ptrdiff_t i = 1; i < nargs; i += 2)
        args[i] = separator;
    }

  Lisp_Object ret = Fconcat (nargs, args);
  SAFE_FREE ();
  return ret;
}

DEFUN ("mapcar", Fmapcar, Smapcar, 2, 2, 0,
       doc: /* Apply FUNCTION to each element of SEQUENCE, and make a list of the results.
The result is a list just as long as SEQUENCE.
SEQUENCE may be a list, a vector, a bool-vector, or a string.  */)
(Lisp_Object function, Lisp_Object sequence)
{
  USE_SAFE_ALLOCA;
  EMACS_INT leni = XFIXNAT (Flength (sequence));
  if (CHAR_TABLE_P (sequence))
    wrong_type_argument (Qlistp, sequence);
  Lisp_Object *args;
  SAFE_ALLOCA_LISP (args, leni);
  ptrdiff_t nmapped = mapcar1 (leni, args, function, sequence);
  Lisp_Object ret = Flist (nmapped, args);
  SAFE_FREE ();
  return ret;
}

DEFUN ("mapc", Fmapc, Smapc, 2, 2, 0,
       doc: /* Apply FUNCTION to each element of SEQUENCE for side effects only.
Unlike `mapcar', don't accumulate the results.  Return SEQUENCE.
SEQUENCE may be a list, a vector, a bool-vector, or a string.  */)
(Lisp_Object function, Lisp_Object sequence)
{
  register EMACS_INT leni;

  leni = XFIXNAT (Flength (sequence));
  if (CHAR_TABLE_P (sequence))
    wrong_type_argument (Qlistp, sequence);
  mapcar1 (leni, 0, function, sequence);

  return sequence;
}

Lisp_Object
nconc2 (Lisp_Object s1, Lisp_Object s2)
{
  return CALLN (Fnconc, s1, s2);
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
DEFUN ("delete", Fdelete, Sdelete, 2, 2, 0,
       doc: /* Delete members of SEQ which are `equal' to ELT, and return the result.
SEQ must be a sequence (i.e. a list, a vector, or a string).
The return value is a sequence of the same type.

If SEQ is a list, this behaves like `delq', except that it compares
with `equal' instead of `eq'.  In particular, it may remove elements
by altering the list structure.

If SEQ is not a list, deletion is never performed destructively;
instead this function creates and returns a new vector or string.

Write `(setq foo (delete element foo))' to be sure of correctly
changing the value of a sequence `foo'.  See also `remove', which
does not modify the argument.  */)
(Lisp_Object elt, Lisp_Object seq)
{
  if (NILP (seq))
    ;
  else if (CONSP (seq))
    {
      Lisp_Object prev = Qnil, tail = seq;

      FOR_EACH_TAIL (tail)
        {
          if (!NILP (Fequal (elt, XCAR (tail))))
            {
              if (NILP (prev))
                seq = XCDR (tail);
              else
                Fsetcdr (prev, XCDR (tail));
            }
          else
            prev = tail;
        }
      CHECK_LIST_END (tail, seq);
    }
  else if (VECTORP (seq))
    {
      ptrdiff_t n = 0;
      ptrdiff_t size = ASIZE (seq);
      USE_SAFE_ALLOCA;
      Lisp_Object *kept = SAFE_ALLOCA (size * sizeof *kept);

      for (ptrdiff_t i = 0; i < size; i++)
        {
          kept[n] = AREF (seq, i);
          n += NILP (Fequal (AREF (seq, i), elt));
        }

      if (n != size)
        seq = Fvector (n, kept);

      SAFE_FREE ();
    }
  else if (STRINGP (seq))
    {
      if (!CHARACTERP (elt))
        return seq;

      ptrdiff_t i, ibyte, nchars, nbytes, cbytes;
      int c;

      for (i = nchars = nbytes = ibyte = 0; i < SCHARS (seq);
           ++i, ibyte += cbytes)
        {
          if (STRING_MULTIBYTE (seq))
            {
              c = STRING_CHAR (SDATA (seq) + ibyte);
              cbytes = CHAR_BYTES (c);
            }
          else
            {
              c = SREF (seq, i);
              cbytes = 1;
            }

          if (c != XFIXNUM (elt))
            {
              ++nchars;
              nbytes += cbytes;
            }
        }

      if (nchars != SCHARS (seq))
        {
          Lisp_Object tem;

          tem = make_uninit_multibyte_string (nchars, nbytes);
          if (!STRING_MULTIBYTE (seq))
            STRING_SET_UNIBYTE (tem);

          for (i = nchars = nbytes = ibyte = 0; i < SCHARS (seq);
               ++i, ibyte += cbytes)
            {
              if (STRING_MULTIBYTE (seq))
                {
                  c = STRING_CHAR (SDATA (seq) + ibyte);
                  cbytes = CHAR_BYTES (c);
                }
              else
                {
                  c = SREF (seq, i);
                  cbytes = 1;
                }

              if (c != XFIXNUM (elt))
                {
                  unsigned char *from = SDATA (seq) + ibyte;
                  unsigned char *to = SDATA (tem) + nbytes;
                  ptrdiff_t n;

                  ++nchars;
                  nbytes += cbytes;

                  for (n = cbytes; n--;)
                    *to++ = *from++;
                }
            }

          seq = tem;
        }
    }
  else
    wrong_type_argument (Qsequencep, seq);

  return seq;
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

DEFUN ("reverse", Freverse, Sreverse, 1, 1, 0,
       doc: /* Return the reversed copy of list, vector, or string SEQ.
See also the function `nreverse', which is used more often.  */)
(Lisp_Object seq)
{
  Lisp_Object new;

  if (NILP (seq))
    return Qnil;
  else if (CONSP (seq))
    {
      new = Qnil;
      FOR_EACH_TAIL (seq)
        new = Fcons (XCAR (seq), new);
      CHECK_LIST_END (seq, seq);
    }
  else if (VECTORP (seq))
    {
      ptrdiff_t i, size = ASIZE (seq);

      new = make_uninit_vector (size);
      for (i = 0; i < size; i++)
        ASET (new, i, AREF (seq, size - i - 1));
    }
  else if (BOOL_VECTOR_P (seq))
    {
      TODO;
    }
  else if (STRINGP (seq))
    {
      ptrdiff_t size = SCHARS (seq), bytes = SBYTES (seq);

      if (size == bytes)
        {
          ptrdiff_t i;

          new = make_uninit_string (size);
          for (i = 0; i < size; i++)
            SSET (new, i, SREF (seq, size - i - 1));
        }
      else
        {
          unsigned char *p, *q;

          new = make_uninit_multibyte_string (size, bytes);
          p = SDATA (seq), q = SDATA (new) + bytes;
          while (q > SDATA (new))
            {
              int len, ch = string_char_and_length (p, &len);
              p += len, q -= len;
              CHAR_STRING (ch, q);
            }
        }
    }
  else
    wrong_type_argument (Qsequencep, seq);
  return new;
}

DEFUN ("length", Flength, Slength, 1, 1, 0,
       doc: /* Return the length of vector, list or string SEQUENCE.
A byte-code function object is also allowed.

If the string contains multibyte characters, this is not necessarily
the number of bytes in the string; it is the number of characters.
To get the number of bytes, use `string-bytes'.

If the length of a list is being computed to compare to a (small)
number, the `length<', `length>' and `length=' functions may be more
efficient.  */)
(Lisp_Object sequence)
{
  EMACS_INT val;

  if (STRINGP (sequence))
    val = SCHARS (sequence);
  else if (CONSP (sequence))
    val = list_length (sequence);
  else if (NILP (sequence))
    val = 0;
  else if (VECTORP (sequence))
    val = ASIZE (sequence);
  else if (CHAR_TABLE_P (sequence))
    val = MAX_CHAR;
  else if (BOOL_VECTOR_P (sequence))
    TODO; // val = bool_vector_size (sequence);
  else if (CLOSUREP (sequence) || RECORDP (sequence))
    val = PVSIZE (sequence);
  else
    wrong_type_argument (Qsequencep, sequence);

  return make_fixnum (val);
}

DEFUN ("string-search", Fstring_search, Sstring_search, 2, 3, 0,
       doc: /* Search for the string NEEDLE in the string HAYSTACK.
The return value is the position of the first occurrence of NEEDLE in
HAYSTACK, or nil if no match was found.

The optional START-POS argument says where to start searching in
HAYSTACK and defaults to zero (start at the beginning).
It must be between zero and the length of HAYSTACK, inclusive.

Case is always significant and text properties are ignored. */)
(register Lisp_Object needle, Lisp_Object haystack, Lisp_Object start_pos)
{
  ptrdiff_t start_byte = 0, haybytes;
  char *res, *haystart;
  EMACS_INT start = 0;

  CHECK_STRING (needle);
  CHECK_STRING (haystack);

  if (!NILP (start_pos))
    {
      CHECK_FIXNUM (start_pos);
      start = XFIXNUM (start_pos);
      if (start < 0 || start > SCHARS (haystack))
        xsignal1 (Qargs_out_of_range, start_pos);
      start_byte = string_char_to_byte (haystack, start);
    }

  if (SCHARS (needle) > SCHARS (haystack) - start)
    return Qnil;

  haystart = SSDATA (haystack) + start_byte;
  haybytes = SBYTES (haystack) - start_byte;

  if (STRING_MULTIBYTE (haystack)
        ? (STRING_MULTIBYTE (needle) || SCHARS (haystack) == SBYTES (haystack)
           || string_ascii_p (needle))
        : (!STRING_MULTIBYTE (needle) || SCHARS (needle) == SBYTES (needle)))
    {
      if (STRING_MULTIBYTE (haystack) && STRING_MULTIBYTE (needle)
          && SCHARS (haystack) == SBYTES (haystack)
          && SCHARS (needle) != SBYTES (needle))
        return Qnil;
      else
        res = memmem (haystart, haybytes, SSDATA (needle), SBYTES (needle));
    }
  else if (STRING_MULTIBYTE (haystack))
    {
      TODO; // Lisp_Object multi_needle = string_to_multibyte (needle);
      // res = memmem (haystart, haybytes, SSDATA (multi_needle),
      //               SBYTES (multi_needle));
    }
  else
    {
      ptrdiff_t nbytes = SBYTES (needle);
      for (ptrdiff_t i = 0; i < nbytes; i++)
        {
          int c = SREF (needle, i);
          if (CHAR_BYTE8_HEAD_P (c))
            i++; /* Skip raw byte.  */
          else if (!ASCII_CHAR_P (c))
            return Qnil;
        }
      TODO; // Lisp_Object uni_needle = Fstring_to_unibyte (needle);
      // res
      //   = memmem (haystart, haybytes, SSDATA (uni_needle), SBYTES
      //   (uni_needle));
    }

  if (!res)
    return Qnil;

  return make_int (string_byte_to_char (haystack, res - SSDATA (haystack)));
}

DEFUN ("string-equal", Fstring_equal, Sstring_equal, 2, 2, 0,
       doc: /* Return t if two strings have identical contents.
Case is significant, but text properties are ignored.
Symbols are also allowed; their print names are used instead.

See also `string-equal-ignore-case'.  */)
(register Lisp_Object s1, Lisp_Object s2)
{
  if (SYMBOLP (s1))
    s1 = SYMBOL_NAME (s1);
  if (SYMBOLP (s2))
    s2 = SYMBOL_NAME (s2);
  CHECK_STRING (s1);
  CHECK_STRING (s2);

  if (SCHARS (s1) != SCHARS (s2) || SBYTES (s1) != SBYTES (s2)
      || memcmp (SDATA (s1), SDATA (s2), SBYTES (s1)))
    return Qnil;
  return Qt;
}

DEFUN ("compare-strings", Fcompare_strings, Scompare_strings, 6, 7, 0,
       doc: /* Compare the contents of two strings, converting to multibyte if needed.
The arguments START1, END1, START2, and END2, if non-nil, are
positions specifying which parts of STR1 or STR2 to compare.  In
string STR1, compare the part between START1 (inclusive) and END1
\(exclusive).  If START1 is nil, it defaults to 0, the beginning of
the string; if END1 is nil, it defaults to the length of the string.
Likewise, in string STR2, compare the part between START2 and END2.
Like in `substring', negative values are counted from the end.

The strings are compared by the numeric values of their characters.
For instance, STR1 is "less than" STR2 if its first differing
character has a smaller numeric value.  If IGNORE-CASE is non-nil,
characters are converted to upper-case before comparing them.  Unibyte
strings are converted to multibyte for comparison.

The value is t if the strings (or specified portions) match.
If string STR1 is less, the value is a negative number N;
  - 1 - N is the number of characters that match at the beginning.
If string STR1 is greater, the value is a positive number N;
  N - 1 is the number of characters that match at the beginning.  */)
(Lisp_Object str1, Lisp_Object start1, Lisp_Object end1, Lisp_Object str2,
 Lisp_Object start2, Lisp_Object end2, Lisp_Object ignore_case)
{
  ptrdiff_t from1, to1, from2, to2, i1, i1_byte, i2, i2_byte;

  CHECK_STRING (str1);
  CHECK_STRING (str2);

  if (FIXNUMP (end1) && SCHARS (str1) < XFIXNUM (end1))
    end1 = make_fixnum (SCHARS (str1));
  if (FIXNUMP (end2) && SCHARS (str2) < XFIXNUM (end2))
    end2 = make_fixnum (SCHARS (str2));

  validate_subarray (str1, start1, end1, SCHARS (str1), &from1, &to1);
  validate_subarray (str2, start2, end2, SCHARS (str2), &from2, &to2);

  i1 = from1;
  i2 = from2;

  i1_byte = string_char_to_byte (str1, i1);
  i2_byte = string_char_to_byte (str2, i2);

  while (i1 < to1 && i2 < to2)
    {
      int c1 = fetch_string_char_as_multibyte_advance (str1, &i1, &i1_byte);
      int c2 = fetch_string_char_as_multibyte_advance (str2, &i2, &i2_byte);

      if (c1 == c2)
        continue;

      if (!NILP (ignore_case))
        {
          c1 = XFIXNUM (Fupcase (make_fixnum (c1)));
          c2 = XFIXNUM (Fupcase (make_fixnum (c2)));
        }

      if (c1 == c2)
        continue;

      if (c1 < c2)
        return make_fixnum (-i1 + from1);
      else
        return make_fixnum (i1 - from1);
    }

  if (i1 < to1)
    return make_fixnum (i1 - from1 + 1);
  if (i2 < to2)
    return make_fixnum (-i1 + from1 - 1);

  return Qt;
}

#if TODO_NELISP_LATER_ELSE
# define HAVE_FAST_UNALIGNED_ACCESS 0
#endif

static int
string_cmp (Lisp_Object string1, Lisp_Object string2)
{
  ptrdiff_t n = min (SCHARS (string1), SCHARS (string2));

  if ((!STRING_MULTIBYTE (string1) || SCHARS (string1) == SBYTES (string1))
      && (!STRING_MULTIBYTE (string2) || SCHARS (string2) == SBYTES (string2)))
    {
      int d = memcmp (SSDATA (string1), SSDATA (string2), n);
      if (d)
        return d;
      return n < SCHARS (string2) ? -1 : n < SCHARS (string1);
    }
  else if (STRING_MULTIBYTE (string1) && STRING_MULTIBYTE (string2))
    {
      ptrdiff_t nb1 = SBYTES (string1);
      ptrdiff_t nb2 = SBYTES (string2);
      ptrdiff_t nb = min (nb1, nb2);
      ptrdiff_t b = 0;

      if (HAVE_FAST_UNALIGNED_ACCESS)
        {
          TODO; // int ws = sizeof (size_t);
          // const char *w1 = SSDATA (string1);
          // const char *w2 = SSDATA (string2);
          // while (b < nb - ws + 1
          //        && load_unaligned_size_t (w1 + b)
          //             == load_unaligned_size_t (w2 + b))
          //   b += ws;
        }

      while (b < nb && SREF (string1, b) == SREF (string2, b))
        b++;

      if (b >= nb)
        return b < nb2 ? -1 : b < nb1;

      while ((SREF (string1, b) & 0xc0) == 0x80)
        b--;

      ptrdiff_t i1 = 0, i2 = 0;
      ptrdiff_t i1_byte = b, i2_byte = b;
      int c1 = fetch_string_char_advance_no_check (string1, &i1, &i1_byte);
      int c2 = fetch_string_char_advance_no_check (string2, &i2, &i2_byte);
      return c1 < c2 ? -1 : c1 > c2;
    }
  else if (STRING_MULTIBYTE (string1))
    {
      ptrdiff_t i1 = 0, i1_byte = 0, i2 = 0;
      while (i1 < n)
        {
          int c1 = fetch_string_char_advance_no_check (string1, &i1, &i1_byte);
          int c2 = SREF (string2, i2++);
          if (c1 != c2)
            return c1 < c2 ? -1 : 1;
        }
      return i1 < SCHARS (string2) ? -1 : i1 < SCHARS (string1);
    }
  else
    {
      ptrdiff_t i1 = 0, i2 = 0, i2_byte = 0;
      while (i1 < n)
        {
          int c1 = SREF (string1, i1++);
          int c2 = fetch_string_char_advance_no_check (string2, &i2, &i2_byte);
          if (c1 != c2)
            return c1 < c2 ? -1 : 1;
        }
      return i1 < SCHARS (string2) ? -1 : i1 < SCHARS (string1);
    }
}

DEFUN ("string-lessp", Fstring_lessp, Sstring_lessp, 2, 2, 0,
       doc: /* Return non-nil if STRING1 is less than STRING2 in lexicographic order.
Case is significant.
Symbols are also allowed; their print names are used instead.  */)
(Lisp_Object string1, Lisp_Object string2)
{
  if (SYMBOLP (string1))
    string1 = SYMBOL_NAME (string1);
  else
    CHECK_STRING (string1);
  if (SYMBOLP (string2))
    string2 = SYMBOL_NAME (string2);
  else
    CHECK_STRING (string2);

  return string_cmp (string1, string2) < 0 ? Qt : Qnil;
}

ptrdiff_t
string_byte_to_char (Lisp_Object string, ptrdiff_t byte_index)
{
  ptrdiff_t i, i_byte;
  ptrdiff_t best_below, best_below_byte;
  ptrdiff_t best_above, best_above_byte;

  best_below = best_below_byte = 0;
  best_above = SCHARS (string);
  best_above_byte = SBYTES (string);
  if (best_above == best_above_byte)
    return byte_index;

  if (BASE_EQ (string, string_char_byte_cache_string))
    {
      if (string_char_byte_cache_bytepos < byte_index)
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

  if (byte_index - best_below_byte < best_above_byte - byte_index)
    {
      unsigned char *p = SDATA (string) + best_below_byte;
      unsigned char *pend = SDATA (string) + byte_index;

      while (p < pend)
        {
          p += BYTES_BY_CHAR_HEAD (*p);
          best_below++;
        }
      i = best_below;
      i_byte = p - SDATA (string);
    }
  else
    {
      unsigned char *p = SDATA (string) + best_above_byte;
      unsigned char *pbeg = SDATA (string) + byte_index;

      while (p > pbeg)
        {
          p--;
          while (!CHAR_HEAD_P (*p))
            p--;
          best_above--;
        }
      i = best_above;
      i_byte = p - SDATA (string);
    }

  string_char_byte_cache_bytepos = i_byte;
  string_char_byte_cache_charpos = i;
  string_char_byte_cache_string = string;

  return i;
}

void
validate_subarray (Lisp_Object array, Lisp_Object from, Lisp_Object to,
                   ptrdiff_t size, ptrdiff_t *ifrom, ptrdiff_t *ito)
{
  EMACS_INT f, t;

  if (FIXNUMP (from))
    {
      f = XFIXNUM (from);
      if (f < 0)
        f += size;
    }
  else if (NILP (from))
    f = 0;
  else
    wrong_type_argument (Qintegerp, from);

  if (FIXNUMP (to))
    {
      t = XFIXNUM (to);
      if (t < 0)
        t += size;
    }
  else if (NILP (to))
    t = size;
  else
    wrong_type_argument (Qintegerp, to);

  if (!(0 <= f && f <= t && t <= size))
    args_out_of_range_3 (array, from, to);

  *ifrom = f;
  *ito = t;
}

Lisp_Object
string_to_multibyte (Lisp_Object string)
{
  if (STRING_MULTIBYTE (string))
    return string;

  ptrdiff_t nchars = SCHARS (string);
  ptrdiff_t nbytes = count_size_as_multibyte (SDATA (string), nchars);
  if (nbytes == nchars)
    return make_multibyte_string (SSDATA (string), nbytes, nbytes);

  Lisp_Object ret = make_uninit_multibyte_string (nchars, nbytes);
  str_to_multibyte (SDATA (ret), SDATA (string), nchars);
  return ret;
}

DEFUN ("string-to-multibyte", Fstring_to_multibyte, Sstring_to_multibyte,
       1, 1, 0,
       doc: /* Return a multibyte string with the same individual chars as STRING.
If STRING is multibyte, the result is STRING itself.
Otherwise it is a newly created string, with no text properties.

If STRING is unibyte and contains an 8-bit byte, it is converted to
the corresponding multibyte character of charset `eight-bit'.

This differs from `string-as-multibyte' by converting each byte of a correct
utf-8 sequence to an eight-bit character, not just bytes that don't form a
correct sequence.  */)
(Lisp_Object string)
{
  CHECK_STRING (string);

  return string_to_multibyte (string);
}

DEFUN ("substring", Fsubstring, Ssubstring, 1, 3, 0,
       doc: /* Return a new string whose contents are a substring of STRING.
The returned string consists of the characters between index FROM
\(inclusive) and index TO (exclusive) of STRING.  FROM and TO are
zero-indexed: 0 means the first character of STRING.  Negative values
are counted from the end of STRING.  If TO is nil, the substring runs
to the end of STRING.

The STRING argument may also be a vector.  In that case, the return
value is a new vector that contains the elements between index FROM
\(inclusive) and index TO (exclusive) of that vector argument.

With one argument, just copy STRING (with properties, if any).  */)
(Lisp_Object string, Lisp_Object from, Lisp_Object to)
{
  Lisp_Object res;
  ptrdiff_t size, ifrom, ito;

  size = CHECK_VECTOR_OR_STRING (string);
  validate_subarray (string, from, to, size, &ifrom, &ito);

  if (STRINGP (string))
    {
      ptrdiff_t from_byte = !ifrom ? 0 : string_char_to_byte (string, ifrom);
      ptrdiff_t to_byte
        = ito == size ? SBYTES (string) : string_char_to_byte (string, ito);
      res = make_specified_string (SSDATA (string) + from_byte, ito - ifrom,
                                   to_byte - from_byte,
                                   STRING_MULTIBYTE (string));
#if TODO_NELISP_LATER_AND
      copy_text_properties (make_fixnum (ifrom), make_fixnum (ito), string,
                            make_fixnum (0), res, Qnil);
#endif
    }
  else
    res = Fvector (ito - ifrom, aref_addr (string, ifrom));

  return res;
}

Lisp_Object
substring_both (Lisp_Object string, ptrdiff_t from, ptrdiff_t from_byte,
                ptrdiff_t to, ptrdiff_t to_byte)
{
  Lisp_Object res;
  ptrdiff_t size = CHECK_VECTOR_OR_STRING (string);

  if (!(0 <= from && from <= to && to <= size))
    args_out_of_range_3 (string, make_fixnum (from), make_fixnum (to));

  if (STRINGP (string))
    {
      res = make_specified_string (SSDATA (string) + from_byte, to - from,
                                   to_byte - from_byte,
                                   STRING_MULTIBYTE (string));
      TODO; // copy_text_properties (make_fixnum (from), make_fixnum (to),
            //  string, make_fixnum (0), res, Qnil);
    }
  else
    res = Fvector (to - from, aref_addr (string, from));

  return res;
}

struct textprop_rec
{
  ptrdiff_t argnum;
  ptrdiff_t to;
};

static Lisp_Object
concat_to_string (ptrdiff_t nargs, Lisp_Object *args)
{
  USE_SAFE_ALLOCA;

  EMACS_INT result_len = 0;
  EMACS_INT result_len_byte = 0;
  bool dest_multibyte = false;
  bool some_unibyte = false;
  for (ptrdiff_t i = 0; i < nargs; i++)
    {
      Lisp_Object arg = args[i];
      EMACS_INT len;

      if (STRINGP (arg))
        {
          ptrdiff_t arg_len_byte = SBYTES (arg);
          len = SCHARS (arg);
          if (STRING_MULTIBYTE (arg))
            dest_multibyte = true;
          else
            some_unibyte = true;
          if (STRING_BYTES_BOUND - result_len_byte < arg_len_byte)
            string_overflow ();
          result_len_byte += arg_len_byte;
        }
      else if (VECTORP (arg))
        {
          len = ASIZE (arg);
          ptrdiff_t arg_len_byte = 0;
          for (ptrdiff_t j = 0; j < len; j++)
            {
              Lisp_Object ch = AREF (arg, j);
              CHECK_CHARACTER (ch);
              int c = XFIXNAT (ch);
              arg_len_byte += CHAR_BYTES (c);
              if (!ASCII_CHAR_P (c) && !CHAR_BYTE8_P (c))
                dest_multibyte = true;
            }
          if (STRING_BYTES_BOUND - result_len_byte < arg_len_byte)
            string_overflow ();
          result_len_byte += arg_len_byte;
        }
      else if (NILP (arg))
        continue;
      else if (CONSP (arg))
        {
          len = XFIXNAT (Flength (arg));
          ptrdiff_t arg_len_byte = 0;
          for (; CONSP (arg); arg = XCDR (arg))
            {
              Lisp_Object ch = XCAR (arg);
              CHECK_CHARACTER (ch);
              int c = XFIXNAT (ch);
              arg_len_byte += CHAR_BYTES (c);
              if (!ASCII_CHAR_P (c) && !CHAR_BYTE8_P (c))
                dest_multibyte = true;
            }
          if (STRING_BYTES_BOUND - result_len_byte < arg_len_byte)
            string_overflow ();
          result_len_byte += arg_len_byte;
        }
      else
        wrong_type_argument (Qsequencep, arg);

      result_len += len;
      if (MOST_POSITIVE_FIXNUM < result_len)
        memory_full (SIZE_MAX);
    }

  if (dest_multibyte && some_unibyte)
    {
      for (ptrdiff_t i = 0; i < nargs; i++)
        {
          Lisp_Object arg = args[i];
          if (STRINGP (arg) && !STRING_MULTIBYTE (arg))
            {
              ptrdiff_t bytes = SCHARS (arg);
              const unsigned char *s = SDATA (arg);
              ptrdiff_t nonascii = 0;
              for (ptrdiff_t j = 0; j < bytes; j++)
                nonascii += s[j] >> 7;
              if (STRING_BYTES_BOUND - result_len_byte < nonascii)
                string_overflow ();
              result_len_byte += nonascii;
            }
        }
    }

  if (!dest_multibyte)
    result_len_byte = result_len;

  Lisp_Object result
    = dest_multibyte
        ? make_uninit_multibyte_string (result_len, result_len_byte)
        : make_uninit_string (result_len);

  ptrdiff_t toindex = 0;
  ptrdiff_t toindex_byte = 0;

  struct textprop_rec *textprops;
  ptrdiff_t num_textprops = 0;
  SAFE_NALLOCA (textprops, 1, nargs);

  for (ptrdiff_t i = 0; i < nargs; i++)
    {
      Lisp_Object arg = args[i];
      if (STRINGP (arg))
        {
          if (string_intervals (arg))
            {
              textprops[num_textprops].argnum = i;
              textprops[num_textprops].to = toindex;
              num_textprops++;
            }
          ptrdiff_t nchars = SCHARS (arg);
          if (STRING_MULTIBYTE (arg) == dest_multibyte)
            {
              ptrdiff_t arg_len_byte = SBYTES (arg);
              memcpy (SDATA (result) + toindex_byte, SDATA (arg), arg_len_byte);
              toindex_byte += arg_len_byte;
            }
          else
            {
              toindex_byte += str_to_multibyte (SDATA (result) + toindex_byte,
                                                SDATA (arg), nchars);
            }
          toindex += nchars;
        }
      else if (VECTORP (arg))
        {
          ptrdiff_t len = ASIZE (arg);
          for (ptrdiff_t j = 0; j < len; j++)
            {
              int c = XFIXNAT (AREF (arg, j));
              if (dest_multibyte)
                toindex_byte += CHAR_STRING (c, SDATA (result) + toindex_byte);
              else
                SSET (result, toindex_byte++, c);
              toindex++;
            }
        }
      else
        for (Lisp_Object tail = arg; !NILP (tail); tail = XCDR (tail))
          {
            int c = XFIXNAT (XCAR (tail));
            if (dest_multibyte)
              toindex_byte += CHAR_STRING (c, SDATA (result) + toindex_byte);
            else
              SSET (result, toindex_byte++, c);
            toindex++;
          }
    }

  if (num_textprops > 0)
    {
      ptrdiff_t last_to_end = -1;
      for (ptrdiff_t i = 0; i < num_textprops; i++)
        {
          Lisp_Object arg = args[textprops[i].argnum];
          Lisp_Object props
            = text_property_list (arg, make_fixnum (0),
                                  make_fixnum (SCHARS (arg)), Qnil);
          if (last_to_end == textprops[i].to)
            make_composition_value_copy (props);
          add_text_properties_from_list (result, props,
                                         make_fixnum (textprops[i].to));
          last_to_end = textprops[i].to + SCHARS (arg);
        }
    }

  SAFE_FREE ();
  return result;
}

Lisp_Object
concat2 (Lisp_Object s1, Lisp_Object s2)
{
  return concat_to_string (2, ((Lisp_Object[]) { s1, s2 }));
}

Lisp_Object
concat3 (Lisp_Object s1, Lisp_Object s2, Lisp_Object s3)
{
  return concat_to_string (3, ((Lisp_Object[]) { s1, s2, s3 }));
}

Lisp_Object
concat_to_list (ptrdiff_t nargs, Lisp_Object *args, Lisp_Object last_tail)
{
  Lisp_Object result = Qnil;
  Lisp_Object last = Qnil; /* Last cons in result if nonempty.  */

  for (ptrdiff_t i = 0; i < nargs; i++)
    {
      Lisp_Object arg = args[i];
      if (CONSP (arg))
        {
          Lisp_Object head = Fcons (XCAR (arg), Qnil);
          Lisp_Object prev = head;
          arg = XCDR (arg);
          FOR_EACH_TAIL (arg)
            {
              Lisp_Object next = Fcons (XCAR (arg), Qnil);
              XSETCDR (prev, next);
              prev = next;
            }
          CHECK_LIST_END (arg, arg);
          if (NILP (result))
            result = head;
          else
            XSETCDR (last, head);
          last = prev;
        }
      else if (NILP (arg))
        ;
      else if (VECTORP (arg) || STRINGP (arg) || BOOL_VECTOR_P (arg)
               || CLOSUREP (arg))
        {
          ptrdiff_t arglen = XFIXNUM (Flength (arg));
          ptrdiff_t argindex_byte = 0;

          for (ptrdiff_t argindex = 0; argindex < arglen; argindex++)
            {
              Lisp_Object elt;
              if (STRINGP (arg))
                {
                  int c;
                  if (STRING_MULTIBYTE (arg))
                    {
                      ptrdiff_t char_idx = argindex;
                      c = fetch_string_char_advance_no_check (arg, &char_idx,
                                                              &argindex_byte);
                    }
                  else
                    c = SREF (arg, argindex);
                  elt = make_fixed_natnum (c);
                }
              else if (BOOL_VECTOR_P (arg))
                elt = bool_vector_ref (arg, argindex);
              else
                elt = AREF (arg, argindex);

              Lisp_Object node = Fcons (elt, Qnil);
              if (NILP (result))
                result = node;
              else
                XSETCDR (last, node);
              last = node;
            }
        }
      else
        wrong_type_argument (Qsequencep, arg);
    }

  if (NILP (result))
    result = last_tail;
  else
    XSETCDR (last, last_tail);

  return result;
}

Lisp_Object
concat_to_vector (ptrdiff_t nargs, Lisp_Object *args)
{
  EMACS_INT result_len = 0;
  for (ptrdiff_t i = 0; i < nargs; i++)
    {
      Lisp_Object arg = args[i];
      if (!(VECTORP (arg) || CONSP (arg) || NILP (arg) || STRINGP (arg)
            || BOOL_VECTOR_P (arg) || CLOSUREP (arg)))
        wrong_type_argument (Qsequencep, arg);
      EMACS_INT len = XFIXNAT (Flength (arg));
      result_len += len;
      if (MOST_POSITIVE_FIXNUM < result_len)
        memory_full (SIZE_MAX);
    }

  Lisp_Object result = make_uninit_vector (result_len);
  Lisp_Object *dst = XVECTOR (result)->contents;

  for (ptrdiff_t i = 0; i < nargs; i++)
    {
      Lisp_Object arg = args[i];
      if (VECTORP (arg))
        {
          ptrdiff_t size = ASIZE (arg);
          memcpy (dst, XVECTOR (arg)->contents, size * sizeof *dst);
          dst += size;
        }
      else if (CONSP (arg))
        do
          {
            *dst++ = XCAR (arg);
            arg = XCDR (arg);
          }
        while (!NILP (arg));
      else if (NILP (arg))
        ;
      else if (STRINGP (arg))
        {
          ptrdiff_t size = SCHARS (arg);
          if (STRING_MULTIBYTE (arg))
            {
              ptrdiff_t byte = 0;
              for (ptrdiff_t i = 0; i < size;)
                {
                  int c = fetch_string_char_advance_no_check (arg, &i, &byte);
                  *dst++ = make_fixnum (c);
                }
            }
          else
            for (ptrdiff_t i = 0; i < size; i++)
              *dst++ = make_fixnum (SREF (arg, i));
        }
      else if (BOOL_VECTOR_P (arg))
        {
          ptrdiff_t size = bool_vector_size (arg);
          for (ptrdiff_t i = 0; i < size; i++)
            *dst++ = bool_vector_ref (arg, i);
        }
      else
        {
          eassert (CLOSUREP (arg));
          ptrdiff_t size = PVSIZE (arg);
          memcpy (dst, XVECTOR (arg)->contents, size * sizeof *dst);
          dst += size;
        }
    }
  eassert (dst == XVECTOR (result)->contents + result_len);

  return result;
}

DEFUN ("append", Fappend, Sappend, 0, MANY, 0,
       doc: /* Concatenate all the arguments and make the result a list.
The result is a list whose elements are the elements of all the arguments.
Each argument may be a list, vector or string.

All arguments except the last argument are copied.  The last argument
is just used as the tail of the new list.  If the last argument is not
a list, this results in a dotted list.

As an exception, if all the arguments except the last are nil, and the
last argument is not a list, the return value is that last argument
unaltered, not a list.

usage: (append &rest SEQUENCES)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  if (nargs == 0)
    return Qnil;
  return concat_to_list (nargs - 1, args, args[nargs - 1]);
}

DEFUN ("concat", Fconcat, Sconcat, 0, MANY, 0,
       doc: /* Concatenate all the arguments and make the result a string.
The result is a string whose elements are the elements of all the arguments.
Each argument may be a string or a list or vector of characters (integers).

Values of the `composition' property of the result are not guaranteed
to be `eq'.
usage: (concat &rest SEQUENCES)  */)
(ptrdiff_t nargs, Lisp_Object *args) { return concat_to_string (nargs, args); }

DEFUN ("vconcat", Fvconcat, Svconcat, 0, MANY, 0,
       doc: /* Concatenate all the arguments and make the result a vector.
The result is a vector whose elements are the elements of all the arguments.
Each argument may be a list, vector or string.
usage: (vconcat &rest SEQUENCES)   */)
(ptrdiff_t nargs, Lisp_Object *args) { return concat_to_vector (nargs, args); }

DEFUN ("copy-sequence", Fcopy_sequence, Scopy_sequence, 1, 1, 0,
       doc: /* Return a copy of a list, vector, string, char-table or record.
The elements of a list, vector or record are not copied; they are
shared with the original.  See Info node `(elisp) Sequence Functions'
for more details about this sharing and its effects.
If the original sequence is empty, this function may return
the same empty object instead of its copy.  */)
(Lisp_Object arg)
{
  if (NILP (arg))
    return arg;

  if (CONSP (arg))
    {
      Lisp_Object val = Fcons (XCAR (arg), Qnil);
      Lisp_Object prev = val;
      Lisp_Object tail = XCDR (arg);
      FOR_EACH_TAIL (tail)
        {
          Lisp_Object c = Fcons (XCAR (tail), Qnil);
          XSETCDR (prev, c);
          prev = c;
        }
      CHECK_LIST_END (tail, tail);
      return val;
    }

  if (STRINGP (arg))
    {
      ptrdiff_t bytes = SBYTES (arg);
      ptrdiff_t chars = SCHARS (arg);
      Lisp_Object val = STRING_MULTIBYTE (arg)
                          ? make_uninit_multibyte_string (chars, bytes)
                          : make_uninit_string (bytes);
      memcpy (SDATA (val), SDATA (arg), bytes);
      INTERVAL ivs = string_intervals (arg);
      if (ivs)
        {
          TODO;
        }
      return val;
    }

  if (VECTORP (arg))
    return Fvector (ASIZE (arg), XVECTOR (arg)->contents);

  if (RECORDP (arg))
    TODO; // return Frecord (PVSIZE (arg), XVECTOR (arg)->contents);

  if (CHAR_TABLE_P (arg))
    return copy_char_table (arg);

  if (BOOL_VECTOR_P (arg))
    {
      EMACS_INT nbits = bool_vector_size (arg);
      ptrdiff_t nbytes = bool_vector_bytes (nbits);
      Lisp_Object val = make_uninit_bool_vector (nbits);
      memcpy (bool_vector_data (val), bool_vector_data (arg), nbytes);
      return val;
    }

  wrong_type_argument (Qsequencep, arg);
}

DEFUN ("copy-alist", Fcopy_alist, Scopy_alist, 1, 1, 0,
       doc: /* Return a copy of ALIST.
This is an alist which represents the same mapping from objects to objects,
but does not share the alist structure with ALIST.
The objects mapped (cars and cdrs of elements of the alist)
are shared, however.
Elements of ALIST that are not conses are also shared.  */)
(Lisp_Object alist)
{
  CHECK_LIST (alist);
  if (NILP (alist))
    return alist;
  alist = Fcopy_sequence (alist);
  for (Lisp_Object tem = alist; !NILP (tem); tem = XCDR (tem))
    {
      Lisp_Object car = XCAR (tem);
      if (CONSP (car))
        XSETCAR (tem, Fcons (XCAR (car), XCDR (car)));
    }
  return alist;
}

#define SXHASH_MAX_DEPTH 3
#define SXHASH_MAX_LEN 7
static EMACS_UINT sxhash_obj (Lisp_Object, int);
static EMACS_UINT
sxhash_float (double val)
{
  EMACS_UINT hash = 0;
  union double_and_words u = { .val = val };
  for (int i = 0; i < WORDS_PER_DOUBLE; i++)
    hash = sxhash_combine (hash, u.word[i]);
  return hash;
}
static EMACS_UINT
sxhash_list (Lisp_Object list, int depth)
{
  EMACS_UINT hash = 0;
  int i;

  if (depth < SXHASH_MAX_DEPTH)
    for (i = 0; CONSP (list) && i < SXHASH_MAX_LEN; list = XCDR (list), ++i)
      {
        EMACS_UINT hash2 = sxhash_obj (XCAR (list), depth + 1);
        hash = sxhash_combine (hash, hash2);
      }

  if (!NILP (list))
    {
      EMACS_UINT hash2 = sxhash_obj (list, depth + 1);
      hash = sxhash_combine (hash, hash2);
    }

  return hash;
}
static EMACS_UINT
sxhash_vector (Lisp_Object vec, int depth)
{
  EMACS_UINT hash = ASIZE (vec);
  int i, n;

  n = min (SXHASH_MAX_LEN, hash & PSEUDOVECTOR_FLAG ? PVSIZE (vec) : hash);
  for (i = 0; i < n; ++i)
    {
      EMACS_UINT hash2 = sxhash_obj (AREF (vec, i), depth + 1);
      hash = sxhash_combine (hash, hash2);
    }

  return hash;
}
static EMACS_UINT
sxhash_bool_vector (Lisp_Object vec)
{
  EMACS_INT size = bool_vector_size (vec);
  EMACS_UINT hash = size;
  int i, n;

  n = min (SXHASH_MAX_LEN, bool_vector_words (size));
  for (i = 0; i < n; ++i)
    hash = sxhash_combine (hash, bool_vector_data (vec)[i]);

  return hash;
}
static EMACS_UINT
sxhash_bignum (Lisp_Object bignum)
{
  mpz_t const *n = xbignum_val (bignum);
  size_t i, nlimbs = mpz_size (*n);
  EMACS_UINT hash = mpz_sgn (*n) < 0;

  for (i = 0; i < nlimbs; ++i)
    hash = sxhash_combine (hash, mpz_getlimbn (*n, i));

  return hash;
}
static EMACS_UINT
sxhash_obj (Lisp_Object obj, int depth)
{
  if (depth > SXHASH_MAX_DEPTH)
    return 0;

  switch (XTYPE (obj))
    {
    case_Lisp_Int:
      return XUFIXNUM (obj);

    case Lisp_Symbol:
      return XHASH (obj);

    case Lisp_String:
      return hash_string (SSDATA (obj), SBYTES (obj));

    case Lisp_Vectorlike:
      {
        enum pvec_type pvec_type = PSEUDOVECTOR_TYPE (XVECTOR (obj));
        if (!(PVEC_NORMAL_VECTOR < pvec_type && pvec_type < PVEC_CLOSURE))
          return (SUB_CHAR_TABLE_P (obj) ? 42 : sxhash_vector (obj, depth));
        else if (pvec_type == PVEC_BIGNUM)
          return sxhash_bignum (obj);
        else if (pvec_type == PVEC_MARKER)
          TODO;
        else if (pvec_type == PVEC_BOOL_VECTOR)
          return sxhash_bool_vector (obj);
        else if (pvec_type == PVEC_OVERLAY)
          TODO;
        else
          {
            if (symbols_with_pos_enabled && pvec_type == PVEC_SYMBOL_WITH_POS)
              obj = XSYMBOL_WITH_POS_SYM (obj);

            return XHASH (obj);
          }
      }

    case Lisp_Cons:
      return sxhash_list (obj, depth);

    case Lisp_Float:
      return sxhash_float (XFLOAT_DATA (obj));

    default:
      emacs_abort ();
    }
}
EMACS_UINT
sxhash (Lisp_Object obj) { return sxhash_obj (obj, 0); }

DEFUN ("secure-hash-algorithms", Fsecure_hash_algorithms,
       Ssecure_hash_algorithms, 0, 0, 0,
       doc: /* Return a list of all the supported `secure-hash' algorithms. */)
(void) { return list (Qmd5, Qsha1, Qsha224, Qsha256, Qsha384, Qsha512); }

/* --- hash table -- */
static void
CHECK_HASH_TABLE (Lisp_Object x)
{
  CHECK_TYPE (HASH_TABLE_P (x), Qhash_table_p, x);
}
static void
set_hash_next_slot (struct Lisp_Hash_Table *h, ptrdiff_t idx, ptrdiff_t val)
{
  eassert (idx >= 0 && idx < h->table_size);
  h->next[idx] = val;
}
static void
set_hash_hash_slot (struct Lisp_Hash_Table *h, ptrdiff_t idx, hash_hash_t val)
{
  eassert (idx >= 0 && idx < h->table_size);
  h->hash[idx] = val;
}
static void
set_hash_index_slot (struct Lisp_Hash_Table *h, ptrdiff_t idx, ptrdiff_t val)
{
  eassert (idx >= 0 && idx < hash_table_index_size (h));
  h->index[idx] = val;
}
static struct Lisp_Hash_Table *
check_hash_table (Lisp_Object obj)
{
  CHECK_HASH_TABLE (obj);
  return XHASH_TABLE (obj);
}

static ptrdiff_t
get_key_arg (Lisp_Object key, ptrdiff_t nargs, Lisp_Object *args, char *used)
{
  ptrdiff_t i;

  for (i = 1; i < nargs; i++)
    if (!used[i - 1] && EQ (args[i - 1], key))
      {
        used[i - 1] = 1;
        used[i] = 1;
        return i;
      }

  return 0;
}

Lisp_Object
larger_vector (Lisp_Object vec, ptrdiff_t incr_min, ptrdiff_t nitems_max)
{
  struct Lisp_Vector *v;
  ptrdiff_t incr, incr_max, old_size, new_size;
  ptrdiff_t C_language_max = min (PTRDIFF_MAX, SIZE_MAX) / sizeof *v->contents;
  ptrdiff_t n_max
    = (0 <= nitems_max && nitems_max < C_language_max ? nitems_max
                                                      : C_language_max);
  eassert (VECTORP (vec));
  eassert (0 < incr_min && -1 <= nitems_max);
  old_size = ASIZE (vec);
  incr_max = n_max - old_size;
  incr = max (incr_min, min (old_size >> 1, incr_max));
  if (incr_max < incr)
    memory_full (SIZE_MAX);
  new_size = old_size + incr;
  v = allocate_vector (new_size);
  memcpy (v->contents, XVECTOR (vec)->contents, old_size * sizeof *v->contents);
  memclear (v->contents + old_size, (new_size - old_size) * word_size);
  XSETVECTOR (vec, v);
  return vec;
}

static ptrdiff_t
HASH_NEXT (struct Lisp_Hash_Table *h, ptrdiff_t idx)
{
  eassert (idx >= 0 && idx < h->table_size);
  return h->next[idx];
}
static ptrdiff_t
HASH_INDEX (struct Lisp_Hash_Table *h, ptrdiff_t idx)
{
  eassert (idx >= 0 && idx < hash_table_index_size (h));
  return h->index[idx];
}

static Lisp_Object
cmpfn_eql (Lisp_Object key1, Lisp_Object key2, struct Lisp_Hash_Table *h)
{
  UNUSED (h);
  UNUSED (key1);
  UNUSED (key2);
  TODO; // return Feql (key1, key2);
}
static Lisp_Object
cmpfn_equal (Lisp_Object key1, Lisp_Object key2, struct Lisp_Hash_Table *h)
{
  UNUSED (h);
  return Fequal (key1, key2);
}

static EMACS_INT
sxhash_eq (Lisp_Object key)
{
  Lisp_Object k = maybe_remove_pos_from_symbol (key);
  return XHASH (k) ^ XTYPE (k);
}
static EMACS_INT
sxhash_eql (Lisp_Object key)
{
  return FLOATP (key) || BIGNUMP (key) ? (EMACS_INT) sxhash (key)
                                       : sxhash_eq (key);
}
static hash_hash_t
hashfn_eq (Lisp_Object key, struct Lisp_Hash_Table *h)
{
  UNUSED (h);
  return reduce_emacs_uint_to_hash_hash (sxhash_eq (key));
}
static hash_hash_t
hashfn_equal (Lisp_Object key, struct Lisp_Hash_Table *h)
{
  UNUSED (h);
  return reduce_emacs_uint_to_hash_hash (sxhash (key));
}
static hash_hash_t
hashfn_eql (Lisp_Object key, struct Lisp_Hash_Table *h)
{
  UNUSED (h);
  return reduce_emacs_uint_to_hash_hash (sxhash_eql (key));
}

struct hash_table_test const hashtest_eq
  = { .name = LISPSYM_INITIALLY (Qeq), .cmpfn = 0, .hashfn = hashfn_eq },
  hashtest_eql = { .name = LISPSYM_INITIALLY (Qeql),
                   .cmpfn = cmpfn_eql,
                   .hashfn = hashfn_eql },
  hashtest_equal = { .name = LISPSYM_INITIALLY (Qequal),
                     .cmpfn = cmpfn_equal,
                     .hashfn = hashfn_equal };
static struct Lisp_Hash_Table *
allocate_hash_table (void)
{
  return ALLOCATE_PLAIN_PSEUDOVECTOR (struct Lisp_Hash_Table, PVEC_HASH_TABLE);
}
static int
compute_hash_index_bits (hash_idx_t size)
{
  unsigned long upper_bound
    = (hash_idx_t) min (MOST_POSITIVE_FIXNUM,
                        min (TYPE_MAXIMUM (hash_idx_t),
                             PTRDIFF_MAX / (sizeof (hash_idx_t))));
  int bits = elogb (size) + 1;
  if (bits >= (int) TYPE_WIDTH (uintmax_t)
      || ((uintmax_t) 1 << bits) > upper_bound)
    error ("Hash table too large");
  return bits;
}
static const hash_idx_t empty_hash_index_vector[] = { -1 };
Lisp_Object
make_hash_table (const struct hash_table_test *test, EMACS_INT size,
                 hash_table_weakness_t weak, bool purecopy)
{
  eassert (SYMBOLP (test->name));
  eassert (0 <= size && size <= min (MOST_POSITIVE_FIXNUM, PTRDIFF_MAX));

  struct Lisp_Hash_Table *h = allocate_hash_table ();

  h->test = test;
  h->weakness = weak;
  h->count = 0;
  h->table_size = size;

  if (size == 0)
    {
      h->key_and_value = NULL;
      h->hash = NULL;
      h->next = NULL;
      h->index_bits = 0;
      h->index = (hash_idx_t *) empty_hash_index_vector;
      h->next_free = -1;
    }
  else
    {
      h->key_and_value
        = hash_table_alloc_bytes (2 * size * sizeof *h->key_and_value);
      for (ptrdiff_t i = 0; i < 2 * size; i++)
        h->key_and_value[i] = HASH_UNUSED_ENTRY_KEY;

      h->hash = hash_table_alloc_bytes (size * sizeof *h->hash);

      h->next = hash_table_alloc_bytes (size * sizeof *h->next);
      for (ptrdiff_t i = 0; i < size - 1; i++)
        h->next[i] = i + 1;
      h->next[size - 1] = -1;

      int index_bits = compute_hash_index_bits (size);
      h->index_bits = index_bits;
      ptrdiff_t index_size = hash_table_index_size (h);
      h->index = hash_table_alloc_bytes (index_size * sizeof *h->index);
      for (ptrdiff_t i = 0; i < index_size; i++)
        h->index[i] = -1;

      h->next_free = 0;
    }

  h->next_weak = NULL;
  h->purecopy = purecopy;
  h->mutable = true;
  return make_lisp_hash_table (h);
}

static inline ptrdiff_t
hash_index_index (struct Lisp_Hash_Table *h, hash_hash_t hash)
{
  return knuth_hash (hash, h->index_bits);
}
static void
maybe_resize_hash_table (struct Lisp_Hash_Table *h)
{
  if (h->next_free < 0)
    {
      ptrdiff_t old_size = HASH_TABLE_SIZE (h);
      ptrdiff_t min_size = 6;
      ptrdiff_t base_size = min (max (old_size, min_size), PTRDIFF_MAX / 2);
      ptrdiff_t new_size
        = old_size == 0 ? min_size
                        : (base_size <= 64 ? base_size * 4 : base_size * 2);

      hash_idx_t *next = hash_table_alloc_bytes (new_size * sizeof *next);
      for (ptrdiff_t i = old_size; i < new_size - 1; i++)
        next[i] = i + 1;
      next[new_size - 1] = -1;

      Lisp_Object *key_and_value
        = hash_table_alloc_bytes (2 * new_size * sizeof *key_and_value);
      memcpy (key_and_value, h->key_and_value,
              2 * old_size * sizeof *key_and_value);
      for (ptrdiff_t i = 2 * old_size; i < 2 * new_size; i++)
        key_and_value[i] = HASH_UNUSED_ENTRY_KEY;

      hash_hash_t *hash = hash_table_alloc_bytes (new_size * sizeof *hash);
      memcpy (hash, h->hash, old_size * sizeof *hash);

      ptrdiff_t old_index_size = hash_table_index_size (h);
      ptrdiff_t index_bits = compute_hash_index_bits (new_size);
      ptrdiff_t index_size = (ptrdiff_t) 1 << index_bits;
      hash_idx_t *index = hash_table_alloc_bytes (index_size * sizeof *index);
      for (ptrdiff_t i = 0; i < index_size; i++)
        index[i] = -1;

      h->index_bits = index_bits;
      h->table_size = new_size;
      h->next_free = old_size;

      if (old_index_size > 1)
        hash_table_free_bytes (h->index, old_index_size * sizeof *h->index);
      h->index = index;

      hash_table_free_bytes (h->key_and_value,
                             2 * old_size * sizeof *h->key_and_value);
      h->key_and_value = key_and_value;

      hash_table_free_bytes (h->hash, old_size * sizeof *h->hash);
      h->hash = hash;

      hash_table_free_bytes (h->next, old_size * sizeof *h->next);
      h->next = next;

      h->key_and_value = key_and_value;

      for (ptrdiff_t i = 0; i < old_size; i++)
        {
          hash_hash_t hash_code = HASH_HASH (h, i);
          ptrdiff_t start_of_bucket = hash_index_index (h, hash_code);
          set_hash_next_slot (h, i, HASH_INDEX (h, start_of_bucket));
          set_hash_index_slot (h, start_of_bucket, i);
        }

#ifdef ENABLE_CHECKING
      if (HASH_TABLE_P (Vpurify_flag) && XHASH_TABLE (Vpurify_flag) == h)
        message ("Growing hash table to: %" pD "d", new_size);
#endif
    }
}

static ptrdiff_t
hash_lookup_with_hash (struct Lisp_Hash_Table *h, Lisp_Object key,
                       hash_hash_t hash)
{
  ptrdiff_t start_of_bucket = hash_index_index (h, hash);
  for (ptrdiff_t i = HASH_INDEX (h, start_of_bucket); 0 <= i;
       i = HASH_NEXT (h, i))
    if (EQ (key, HASH_KEY (h, i))
        || (h->test->cmpfn && hash == HASH_HASH (h, i)
            && !NILP (h->test->cmpfn (key, HASH_KEY (h, i), h))))
      return i;

  return -1;
}
ptrdiff_t
hash_lookup (struct Lisp_Hash_Table *h, Lisp_Object key)
{
  return hash_lookup_with_hash (h, key, hash_from_key (h, key));
}
ptrdiff_t
hash_lookup_get_hash (struct Lisp_Hash_Table *h, Lisp_Object key,
                      hash_hash_t *phash)
{
  EMACS_UINT hash = hash_from_key (h, key);
  *phash = hash;
  return hash_lookup_with_hash (h, key, hash);
}

static void
check_mutable_hash_table (Lisp_Object obj, struct Lisp_Hash_Table *h)
{
  if (!h->mutable)
    signal_error ("hash table test modifies table", obj);
  eassert (!PURE_P (h));
}
ptrdiff_t
hash_put (struct Lisp_Hash_Table *h, Lisp_Object key, Lisp_Object value,
          hash_hash_t hash)
{
  eassert (!hash_unused_entry_key_p (key));
  maybe_resize_hash_table (h);
  h->count++;

  ptrdiff_t i = h->next_free;
  eassert (hash_unused_entry_key_p (HASH_KEY (h, i)));
  h->next_free = HASH_NEXT (h, i);
  set_hash_key_slot (h, i, key);
  set_hash_value_slot (h, i, value);

  set_hash_hash_slot (h, i, hash);

  ptrdiff_t start_of_bucket = hash_index_index (h, hash);
  set_hash_next_slot (h, i, HASH_INDEX (h, start_of_bucket));
  set_hash_index_slot (h, start_of_bucket, i);
  return i;
}

static inline bool
keep_entry_p (hash_table_weakness_t weakness, bool strong_key,
              bool strong_value)
{
  switch (weakness)
    {
    case Weak_None:
      return true;
    case Weak_Key:
      return strong_key;
    case Weak_Value:
      return strong_value;
    case Weak_Key_Or_Value:
      return strong_key || strong_value;
    case Weak_Key_And_Value:
      return strong_key && strong_value;
    }
  emacs_abort ();
}

bool
sweep_weak_table (struct Lisp_Hash_Table *h, bool remove_entries_p)
{
  ptrdiff_t n = hash_table_index_size (h);
  bool marked = false;

  for (ptrdiff_t bucket = 0; bucket < n; ++bucket)
    {
      ptrdiff_t prev = -1;
      ptrdiff_t next;
      for (ptrdiff_t i = HASH_INDEX (h, bucket); 0 <= i; i = next)
        {
          bool key_known_to_survive_p = survives_gc_p (HASH_KEY (h, i));
          bool value_known_to_survive_p = survives_gc_p (HASH_VALUE (h, i));
          bool remove_p = !keep_entry_p (h->weakness, key_known_to_survive_p,
                                         value_known_to_survive_p);

          next = HASH_NEXT (h, i);

          if (remove_entries_p)
            {
              eassert (!remove_p
                       == (key_known_to_survive_p && value_known_to_survive_p));
              if (remove_p)
                {
                  if (prev < 0)
                    set_hash_index_slot (h, bucket, next);
                  else
                    set_hash_next_slot (h, prev, next);

                  set_hash_next_slot (h, i, h->next_free);
                  h->next_free = i;

                  set_hash_key_slot (h, i, HASH_UNUSED_ENTRY_KEY);
                  set_hash_value_slot (h, i, Qnil);

                  eassert (h->count != 0);
                  h->count--;
                }
              else
                {
                  prev = i;
                }
            }
          else
            {
              if (!remove_p)
                {
                  if (!key_known_to_survive_p)
                    {
                      mark_object (HASH_KEY (h, i));
                      marked = true;
                    }

                  if (!value_known_to_survive_p)
                    {
                      mark_object (HASH_VALUE (h, i));
                      marked = true;
                    }
                }
            }
        }
    }

  return marked;
}

DEFUN ("make-hash-table", Fmake_hash_table, Smake_hash_table, 0, MANY, 0,
       doc: /* Create and return a new hash table.

Arguments are specified as keyword/argument pairs.  The following
arguments are defined:

:test TEST -- TEST must be a symbol that specifies how to compare
keys.  Default is `eql'.  Predefined are the tests `eq', `eql', and
`equal'.  User-supplied test and hash functions can be specified via
`define-hash-table-test'.

:size SIZE -- A hint as to how many elements will be put in the table.
The table will always grow as needed; this argument may help performance
slightly if the size is known in advance but is never required.

:weakness WEAK -- WEAK must be one of nil, t, `key', `value',
`key-or-value', or `key-and-value'.  If WEAK is not nil, the table
returned is a weak table.  Key/value pairs are removed from a weak
hash table when there are no non-weak references pointing to their
key, value, one of key or value, or both key and value, depending on
WEAK.  WEAK t is equivalent to `key-and-value'.  Default value of WEAK
is nil.

:purecopy PURECOPY -- If PURECOPY is non-nil, the table can be copied
to pure storage when Emacs is being dumped, making the contents of the
table read only. Any further changes to purified tables will result
in an error.

The keywords arguments :rehash-threshold and :rehash-size are obsolete
and ignored.

usage: (make-hash-table &rest KEYWORD-ARGS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  USE_SAFE_ALLOCA;

  char *used = SAFE_ALLOCA (nargs * sizeof *used);
  memset (used, 0, nargs * sizeof *used);

  ptrdiff_t i = get_key_arg (QCtest, nargs, args, used);
  Lisp_Object test = i ? maybe_remove_pos_from_symbol (args[i]) : Qeql;
  const struct hash_table_test *testdesc;
  if (BASE_EQ (test, Qeq))
    testdesc = &hashtest_eq;
  else if (BASE_EQ (test, Qeql))
    testdesc = &hashtest_eql;
  else if (BASE_EQ (test, Qequal))
    testdesc = &hashtest_equal;
  else
    TODO; // testdesc = get_hash_table_user_test (test);

  i = get_key_arg (QCpurecopy, nargs, args, used);
  bool purecopy = i && !NILP (args[i]);
  i = get_key_arg (QCsize, nargs, args, used);
  Lisp_Object size_arg = i ? args[i] : Qnil;
  EMACS_INT size;
  if (NILP (size_arg))
    size = DEFAULT_HASH_SIZE;
  else if (FIXNATP (size_arg))
    size = XFIXNAT (size_arg);
  else
    signal_error ("Invalid hash table size", size_arg);

  i = get_key_arg (QCweakness, nargs, args, used);
  Lisp_Object weakness = i ? args[i] : Qnil;
  hash_table_weakness_t weak;
  if (NILP (weakness))
    weak = Weak_None;
  else if (EQ (weakness, Qkey))
    weak = Weak_Key;
  else if (EQ (weakness, Qvalue))
    weak = Weak_Value;
  else if (EQ (weakness, Qkey_or_value))
    weak = Weak_Key_Or_Value;
  else if (EQ (weakness, Qt) || EQ (weakness, Qkey_and_value))
    weak = Weak_Key_And_Value;
  else
    signal_error ("Invalid hash table weakness", weakness);

  for (i = 0; i < nargs; ++i)
    if (!used[i])
      {
        if (EQ (args[i], QCrehash_threshold) || EQ (args[i], QCrehash_size))
          i++;
        else
          signal_error ("Invalid argument list", args[i]);
      }

  SAFE_FREE ();
  return make_hash_table (testdesc, size, weak, purecopy);
}

DEFUN ("gethash", Fgethash, Sgethash, 2, 3, 0,
       doc: /* Look up KEY in TABLE and return its associated value.
If KEY is not found, return DFLT which defaults to nil.  */)
(Lisp_Object key, Lisp_Object table, Lisp_Object dflt)
{
  struct Lisp_Hash_Table *h = check_hash_table (table);
  ptrdiff_t i = hash_lookup (h, key);
  return i >= 0 ? HASH_VALUE (h, i) : dflt;
}
DEFUN ("puthash", Fputhash, Sputhash, 3, 3, 0,
       doc: /* Associate KEY with VALUE in hash table TABLE.
If KEY is already present in table, replace its current value with
VALUE.  In any case, return VALUE.  */)
(Lisp_Object key, Lisp_Object value, Lisp_Object table)
{
  struct Lisp_Hash_Table *h = check_hash_table (table);
  check_mutable_hash_table (table, h);

  EMACS_UINT hash = hash_from_key (h, key);
  ptrdiff_t i = hash_lookup_with_hash (h, key, hash);
  if (i >= 0)
    set_hash_value_slot (h, i, value);
  else
    hash_put (h, key, value, hash);

  return value;
}
DEFUN ("hash-table-p", Fhash_table_p, Shash_table_p, 1, 1, 0,
       doc: /* Return t if OBJ is a Lisp hash table object.  */)
(Lisp_Object obj) { return HASH_TABLE_P (obj) ? Qt : Qnil; }
DEFUN ("maphash", Fmaphash, Smaphash, 2, 2, 0,
       doc: /* Call FUNCTION for all entries in hash table TABLE.
FUNCTION is called with two arguments, KEY and VALUE.
It should not alter TABLE in any way other than using `puthash' to
set a new value for KEY, or `remhash' to remove KEY.
`maphash' always returns nil.  */)
(Lisp_Object function, Lisp_Object table)
{
  struct Lisp_Hash_Table *h = check_hash_table (table);
  DOHASH_SAFE (h, i)
  call2 (function, HASH_KEY (h, i), HASH_VALUE (h, i));
  return Qnil;
}
DEFUN ("featurep", Ffeaturep, Sfeaturep, 1, 2, 0,
       doc: /* Return t if FEATURE is present in this Emacs.

Use this to conditionalize execution of lisp code based on the
presence or absence of Emacs or environment extensions.
Use `provide' to declare that a feature is available.  This function
looks at the value of the variable `features'.  The optional argument
SUBFEATURE can be used to check a specific subfeature of FEATURE.  */)
(Lisp_Object feature, Lisp_Object subfeature)
{
  register Lisp_Object tem;
  CHECK_SYMBOL (feature);
  tem = Fmemq (feature, Vfeatures);
  if (!NILP (tem) && !NILP (subfeature))
    tem = Fmember (subfeature, Fget (feature, Qsubfeatures));
  return (NILP (tem)) ? Qnil : Qt;
}
DEFUN ("provide", Fprovide, Sprovide, 1, 2, 0,
       doc: /* Announce that FEATURE is a feature of the current Emacs.
The optional argument SUBFEATURES should be a list of symbols listing
particular subfeatures supported in this version of FEATURE.  */)
(Lisp_Object feature, Lisp_Object subfeatures)
{
  register Lisp_Object tem;
  CHECK_SYMBOL (feature);
  CHECK_LIST (subfeatures);
#if TODO_NELISP_LATER_AND
  if (!NILP (Vautoload_queue))
    Vautoload_queue
      = Fcons (Fcons (make_fixnum (0), Vfeatures), Vautoload_queue);
#endif
  tem = Fmemq (feature, Vfeatures);
  if (NILP (tem))
    Vfeatures = Fcons (feature, Vfeatures);
  if (!NILP (subfeatures))
    Fput (feature, Qsubfeatures, subfeatures);
  LOADHIST_ATTACH (Fcons (Qprovide, feature));

  tem = Fassq (feature, Vafter_load_alist);
  if (CONSP (tem))
    Fmapc (Qfuncall, XCDR (tem));

  return feature;
}

static Lisp_Object require_nesting_list;

static void
require_unwind (Lisp_Object old_value)
{
  require_nesting_list = old_value;
}

DEFUN ("require", Frequire, Srequire, 1, 3, 0,
       doc: /* If FEATURE is not already loaded, load it from FILENAME.
If FEATURE is not a member of the list `features', then the feature was
not yet loaded; so load it from file FILENAME.

If FILENAME is omitted, the printname of FEATURE is used as the file
name, and `load' is called to try to load the file by that name, after
appending the suffix `.elc', `.el', or the system-dependent suffix for
dynamic module files, in that order; but the function will not try to
load the file without any suffix.  See `get-load-suffixes' for the
complete list of suffixes.

To find the file, this function searches the directories in `load-path'.

If the optional third argument NOERROR is non-nil, then, if
the file is not found, the function returns nil instead of signaling
an error.  Normally the return value is FEATURE.

The normal messages issued by `load' at start and end of loading
FILENAME are suppressed.  */)
(Lisp_Object feature, Lisp_Object filename, Lisp_Object noerror)
{
  Lisp_Object tem;
  bool from_file = load_in_progress;

  CHECK_SYMBOL (feature);

  if (!from_file)
    {
      Lisp_Object tail = Vcurrent_load_list;
      FOR_EACH_TAIL_SAFE (tail)
        if (NILP (XCDR (tail)) && STRINGP (XCAR (tail)))
          from_file = true;
    }

  if (from_file)
    {
      tem = Fcons (Qrequire, feature);
      if (NILP (Fmember (tem, Vcurrent_load_list)))
        LOADHIST_ATTACH (tem);
    }
  tem = Fmemq (feature, Vfeatures);

  if (NILP (tem))
    {
      specpdl_ref count = SPECPDL_INDEX ();
      int nesting = 0;

      if (will_dump_p () && !will_bootstrap_p ())
        {
          TODO;
        }

      tem = require_nesting_list;
      while (!NILP (tem))
        {
          if (!NILP (Fequal (feature, XCAR (tem))))
            nesting++;
          tem = XCDR (tem);
        }
      if (nesting > 3)
        error ("Recursive `require' for feature `%s'",
               SDATA (SYMBOL_NAME (feature)));

      record_unwind_protect (require_unwind, require_nesting_list);
      require_nesting_list = Fcons (feature, require_nesting_list);

      tem = load_with_autoload_queue (NILP (filename) ? Fsymbol_name (feature)
                                                      : filename,
                                      noerror, Qt, Qnil,
                                      (NILP (filename) ? Qt : Qnil));

      if (NILP (tem))
        return unbind_to (count, Qnil);

      tem = Fmemq (feature, Vfeatures);
      if (NILP (tem))
        {
          TODO;
        }

      feature = unbind_to (count, feature);
    }

  return feature;
}

void
syms_of_fns (void)
{
  DEFSYM (Qmd5,    "md5");
  DEFSYM (Qsha1,   "sha1");
  DEFSYM (Qsha224, "sha224");
  DEFSYM (Qsha256, "sha256");
  DEFSYM (Qsha384, "sha384");
  DEFSYM (Qsha512, "sha512");

  DEFSYM (Qhash_table_p, "hash-table-p");
  DEFSYM (Qeq, "eq");
  DEFSYM (Qeql, "eql");
  DEFSYM (Qequal, "equal");
  DEFSYM (QCtest, ":test");
  DEFSYM (QCsize, ":size");
  DEFSYM (QCpurecopy, ":purecopy");
  DEFSYM (QCrehash_size, ":rehash-size");
  DEFSYM (QCrehash_threshold, ":rehash-threshold");
  DEFSYM (QCweakness, ":weakness");
  DEFSYM (Qkey, "key");
  DEFSYM (Qvalue, "value");
  DEFSYM (Qhash_table_test, "hash-table-test");
  DEFSYM (Qkey_or_value, "key-or-value");
  DEFSYM (Qkey_and_value, "key-and-value");

  DEFSYM (Qrequire, "require");
  DEFSYM (Qprovide, "provide");

  DEFVAR_LISP ("features", Vfeatures,
    doc: /* A list of symbols which are the features of the executing Emacs.
Used by `featurep' and `require', and altered by `provide'.  */);
  Vfeatures = list1 (Qemacs);
  DEFSYM (Qfeatures, "features");
#if TODO_NELISP_LATER_AND
  Fmake_var_non_special (Qfeatures);
#endif
  DEFSYM (Qsubfeatures, "subfeatures");
  DEFSYM (Qfuncall, "funcall");
  DEFSYM (Qplistp, "plistp");
  DEFSYM (Qlist_or_vector_p, "list-or-vector-p");

  require_nesting_list = Qnil;
  staticpro (&require_nesting_list);

  defsubr (&Ssort);
  defsubr (&Splist_get);
  defsubr (&Sget);
  defsubr (&Smemq);
  defsubr (&Sassq);
  defsubr (&Sassoc);
  defsubr (&Srassq);
  defsubr (&Splist_put);
  defsubr (&Sput);
  defsubr (&Smember);
  defsubr (&Sequal);
  defsubr (&Seql);
  defsubr (&Snthcdr);
  defsubr (&Snth);
  defsubr (&Sproper_list_p);
  defsubr (&Sdelq);
  defsubr (&Smapconcat);
  defsubr (&Smapcar);
  defsubr (&Smapc);
  defsubr (&Snconc);
  defsubr (&Sdelete);
  defsubr (&Snreverse);
  defsubr (&Sreverse);
  defsubr (&Slength);
  defsubr (&Sstring_search);
  defsubr (&Sstring_equal);
  defsubr (&Scompare_strings);
  defsubr (&Sstring_lessp);
  defsubr (&Sstring_to_multibyte);
  defsubr (&Ssubstring);
  defsubr (&Sappend);
  defsubr (&Sconcat);
  defsubr (&Svconcat);
  defsubr (&Scopy_sequence);
  defsubr (&Scopy_alist);
  defsubr (&Ssecure_hash_algorithms);
  defsubr (&Smake_hash_table);
  defsubr (&Sgethash);
  defsubr (&Sputhash);
  defsubr (&Shash_table_p);
  defsubr (&Smaphash);
  defsubr (&Sfeaturep);
  defsubr (&Sprovide);
  defsubr (&Srequire);

  DEFSYM (QCkey, ":key");
  DEFSYM (QClessp, ":lessp");
  DEFSYM (QCin_place, ":in-place");
  DEFSYM (QCreverse, ":reverse");
  DEFSYM (Qvaluelt, "value<");
}
