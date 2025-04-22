#include "lisp.h"
#include "bignum.h"
#include "character.h"
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

#define SXHASH_MAX_DEPTH 3
#define SXHASH_MAX_LEN 7
enum DEFAULT_HASH_SIZE
{
  DEFAULT_HASH_SIZE = 0
};
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
          TODO;
        else if (pvec_type == PVEC_BIGNUM)
          return sxhash_bignum (obj);
        else if (pvec_type == PVEC_MARKER)
          TODO;
        else if (pvec_type == PVEC_BOOL_VECTOR)
          TODO; // return sxhash_bool_vector (obj);
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

#if TODO_NELISP_LATER_AND
  tem = Fassq (feature, Vafter_load_alist);
  if (CONSP (tem))
    Fmapc (Qfuncall, XCDR (tem));
#endif

  return feature;
}

void
syms_of_fns (void)
{
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
  DEFSYM (Qplistp, "plistp");

  defsubr (&Sget);
  defsubr (&Smemq);
  defsubr (&Sassq);
  defsubr (&Sassoc);
  defsubr (&Sput);
  defsubr (&Smember);
  defsubr (&Sequal);
  defsubr (&Snthcdr);
  defsubr (&Snth);
  defsubr (&Sproper_list_p);
  defsubr (&Sdelq);
  defsubr (&Snconc);
  defsubr (&Snreverse);
  defsubr (&Slength);
  defsubr (&Ssubstring);
  defsubr (&Smake_hash_table);
  defsubr (&Sgethash);
  defsubr (&Sputhash);
  defsubr (&Sprovide);
}
