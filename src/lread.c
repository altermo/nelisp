#ifndef EMACS_LREAD_C
#define EMACS_LREAD_C
#include "lisp.h"
#include "alloc.c"
#include "fns.c"

static Lisp_Object initial_obarray;
static size_t oblookup_last_bucket_number;

Lisp_Object
check_obarray_slow (Lisp_Object obarray)
{
    UNUSED(obarray);
    TODO;
    return NULL;
}

INLINE Lisp_Object
check_obarray (Lisp_Object obarray)
{
    return OBARRAYP (obarray) ? obarray : check_obarray_slow (obarray);
}

static ptrdiff_t
obarray_index (struct Lisp_Obarray *oa, const char *str, ptrdiff_t size_byte)
{
    EMACS_UINT hash = hash_string (str, size_byte);
    return knuth_hash (reduce_emacs_uint_to_hash_hash (hash), oa->size_bits);
}

Lisp_Object
oblookup (Lisp_Object obarray, register const char *ptr, ptrdiff_t size, ptrdiff_t size_byte)
{
    struct Lisp_Obarray *o = XOBARRAY (obarray);
    ptrdiff_t idx = obarray_index (o, ptr, size_byte);
    Lisp_Object bucket = o->buckets[idx];

    oblookup_last_bucket_number = idx;
    if (!BASE_EQ (bucket, make_fixnum (0)))
    {
        Lisp_Object sym = bucket;
        while (1)
        {
            struct Lisp_Symbol *s = XBARE_SYMBOL (sym);
            Lisp_Object name = s->u.s.name;
            if (SBYTES (name) == size_byte && SCHARS (name) == size
                && memcmp (SDATA (name), ptr, size_byte) == 0)
                return sym;
            if (s->u.s.next == NULL)
                break;
            sym = make_lisp_symbol(s->u.s.next);
        }
    }
    return make_fixnum (idx);
}

enum {
    obarray_default_bits = 3,
    word_size_log2 = word_size < 8 ? 5 : 6,
    obarray_max_bits = min (8 * sizeof (int),
                            8 * sizeof (ptrdiff_t) - word_size_log2) - 1,
};

static void
grow_obarray (struct Lisp_Obarray *o)
{
    ptrdiff_t old_size = obarray_size (o);
    eassert (o->count > old_size);
    Lisp_Object *old_buckets = o->buckets;

    int new_bits = o->size_bits + 1;
    if (new_bits > obarray_max_bits)
        TODO //error ("Obarray too big");
    ptrdiff_t new_size = (ptrdiff_t)1 << new_bits;
    o->buckets = hash_table_alloc_bytes (new_size * sizeof *o->buckets);
    for (ptrdiff_t i = 0; i < new_size; i++)
        o->buckets[i] = make_fixnum (0);
    o->size_bits = new_bits;

    for (ptrdiff_t i = 0; i < old_size; i++)
    {
        Lisp_Object obj = old_buckets[i];
        if (BARE_SYMBOL_P (obj))
        {
            struct Lisp_Symbol *s = XBARE_SYMBOL (obj);
            while (1)
            {
                Lisp_Object name = s->u.s.name;
                ptrdiff_t idx = obarray_index (o, SSDATA (name), SBYTES (name));
                Lisp_Object *loc = o->buckets + idx;
                struct Lisp_Symbol *next = s->u.s.next;
                s->u.s.next = BARE_SYMBOL_P (*loc) ? XBARE_SYMBOL (*loc) : NULL;
                *loc = make_lisp_symbol (s);
                if (next == NULL)
                    break;
                s = next;
            }
        }
    }

    hash_table_free_bytes (old_buckets, old_size * sizeof *old_buckets);
}

static Lisp_Object
intern_sym (Lisp_Object sym, Lisp_Object obarray, Lisp_Object index)
{
    eassert (BARE_SYMBOL_P (sym) && OBARRAYP (obarray) && FIXNUMP (index));
    struct Lisp_Symbol *s = XBARE_SYMBOL (sym);
    s->u.s.interned = (BASE_EQ (obarray, initial_obarray)
        ? SYMBOL_INTERNED_IN_INITIAL_OBARRAY
        : SYMBOL_INTERNED);

    if (SREF (s->u.s.name, 0) == ':' && BASE_EQ (obarray, initial_obarray))
    {
        s->u.s.trapped_write = SYMBOL_NOWRITE;
        s->u.s.redirect = SYMBOL_PLAINVAL;
        s->u.s.declared_special = true;
        SET_SYMBOL_VAL (s, sym);
    }

    struct Lisp_Obarray *o = XOBARRAY (obarray);
    Lisp_Object *ptr = o->buckets + XFIXNUM (index);
    s->u.s.next = BARE_SYMBOL_P (*ptr) ? XBARE_SYMBOL (*ptr) : NULL;
    *ptr = sym;
    o->count++;
    if (o->count > obarray_size (o))
        grow_obarray (o);
    return sym;
}

static struct Lisp_Obarray *
allocate_obarray (void)
{
  return ALLOCATE_PLAIN_PSEUDOVECTOR (struct Lisp_Obarray, PVEC_OBARRAY);
}

static Lisp_Object
make_obarray (unsigned bits)
{
  struct Lisp_Obarray *o = allocate_obarray ();
  o->count = 0;
  o->size_bits = bits;
  ptrdiff_t size = (ptrdiff_t)1 << bits;
  o->buckets = hash_table_alloc_bytes (size * sizeof *o->buckets);
  for (ptrdiff_t i = 0; i < size; i++)
    o->buckets[i] = make_fixnum (0);
  return make_lisp_obarray (o);
}

static void
define_symbol (Lisp_Object sym, char const *str)
{
  ptrdiff_t len = strlen (str);
#if TODO_NELISP_LATER_AND
    Lisp_Object string = make_pure_c_string (str, len);
#else
    Lisp_Object string = make_unibyte_string(str, len);
#endif
  init_symbol (sym, string);

  if (! BASE_EQ (sym, Qunbound))
    {
      Lisp_Object bucket = oblookup (initial_obarray, str, len, len);
      eassert (FIXNUMP (bucket));
      intern_sym (sym, initial_obarray, bucket);
    }
}

void
init_obarray_once (void)
{
    Vobarray = make_obarray (15);
    initial_obarray = Vobarray;
    staticpro (&initial_obarray);

    for (unsigned long i = 0; i < ARRAYELTS (lispsym); i++)
        define_symbol (builtin_lisp_symbol (i), defsym_name[i]);
    TODO_NELISP_LATER
}

Lisp_Object
intern_driver (Lisp_Object string, Lisp_Object obarray, Lisp_Object index)
{
    SET_SYMBOL_VAL (XBARE_SYMBOL (Qobarray_cache), Qnil);
    return intern_sym (Fmake_symbol (string), obarray, index);
}

Lisp_Object
intern_c_string_1 (const char *str, ptrdiff_t len)
{
    Lisp_Object obarray = check_obarray (Vobarray);
    Lisp_Object tem = oblookup (obarray, str, len, len);

    if (!BARE_SYMBOL_P (tem))
    {
        Lisp_Object string;

#if TODO_NELISP_LATER_AND
        if (NILP (Vpurify_flag))
            string = make_string (str, len);
        else
            string = make_pure_c_string (str, len);
#else
        string = make_unibyte_string(str, len);
#endif

        tem = intern_driver (string, obarray, tem);
    }
    return tem;
}
INLINE Lisp_Object
intern_c_string (const char *str)
{
    return intern_c_string_1 (str, strlen (str));
}

void
defvar_lisp_nopro (struct Lisp_Objfwd const *o_fwd, char const *namestring)
{
    Lisp_Object sym = intern_c_string (namestring);
    XBARE_SYMBOL (sym)->u.s.declared_special = true;
    XBARE_SYMBOL (sym)->u.s.redirect = SYMBOL_FORWARDED;
    SET_SYMBOL_FWD (XBARE_SYMBOL (sym), o_fwd);
}
void
defvar_lisp (struct Lisp_Objfwd const *o_fwd, char const *namestring)
{
    defvar_lisp_nopro (o_fwd, namestring);
    staticpro (o_fwd->objvar);
}

void
syms_of_lread (void) {
    DEFVAR_LISP ("obarray", Vobarray,
                 doc: /* Symbol table for use by `intern' and `read'.
It is a vector whose length ought to be prime for best results.
The vector's contents don't make sense if examined from Lisp programs;
to find all the symbols in an obarray, use `mapatoms'.  */);

    DEFSYM (Qobarray_cache, "obarray-cache");
}

#endif
