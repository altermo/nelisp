#ifndef EMACS_LISP_H
#define EMACS_LISP_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <stdalign.h>
#include <setjmp.h>

#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>

static lua_State *global_lua_state;
static bool unrecoverable_error;
#define unrecoverable_lua_error(L,...) unrecoverable_error=true,luaL_error(L,__VA_ARGS__)
#define TODO_NELISP_LATER (void)0;
#define TODO_NELISP_LATER_ELSE true
#define TODO_NELISP_LATER_AND false
#define TODO if (1){eassert(global_lua_state);unrecoverable_lua_error(global_lua_state,"TODO at %s:%d",__FILE__,__LINE__);}
#define UNUSED(x) (void)x

// LSP being anyoing about them existing and not existing, so just define them here
#ifndef LONG_WIDTH
#define LONG_WIDTH   __LONG_WIDTH__
#define ULONG_WIDTH  __LONG_WIDTH__
#endif
#ifndef SIZE_WIDTH
#define SIZE_WIDTH   __WORDSIZE
#endif

#define symbols_with_pos_enabled 0

# define assume(R) ((R) ? (void) 0 : __builtin_unreachable ())
// Taken from conf_post.h
#define INLINE EXTERN_INLINE
#define EXTERN_INLINE static inline __attribute__((unused))
#define NO_INLINE __attribute__ ((__noinline__))
#define FLEXIBLE_ARRAY_MEMBER /**/
#define FLEXALIGNOF(type) (sizeof (type) & ~ (sizeof (type) - 1))
#define FLEXSIZEOF(type, member, n) \
((offsetof (type, member) + FLEXALIGNOF (type) - 1 + (n)) \
    & ~ (FLEXALIGNOF (type) - 1))
typedef bool bool_bf;

//!IMPORTANT: just to get things started, a lot of things will be presumed (like 64-bit ptrs) or not optimized

INLINE bool
pdumper_object_p (const void *obj) {
    UNUSED(obj);
    TODO_NELISP_LATER
    return false;
}

#undef min
#undef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define POWER_OF_2(n) (((n) & ((n) - 1)) == 0)
#define ROUNDUP(x, y) (POWER_OF_2 (y)					\
    ? ((y) - 1 + (x)) & ~ ((y) - 1)			\
    : ((y) - 1 + (x)) - ((y) - 1 + (x)) % (y))
#define ARRAYELTS(arr) (sizeof (arr) / sizeof (arr)[0])
#define GCTYPEBITS 3
typedef long int EMACS_INT;
typedef unsigned long EMACS_UINT;
enum { EMACS_INT_WIDTH = LONG_WIDTH, EMACS_UINT_WIDTH = ULONG_WIDTH };
#define EMACS_INT_MAX LONG_MAX
typedef size_t bits_word;
# define BITS_WORD_MAX SIZE_MAX
enum { BITS_PER_BITS_WORD = SIZE_WIDTH };
#define pI "l"
#define AVOID _Noreturn void
#define eassert(cond) ((void) (false && (cond)))
#define eassume(cond) assume (cond)
enum Lisp_Bits
{
    VALBITS = EMACS_INT_WIDTH - GCTYPEBITS,
    FIXNUM_BITS = VALBITS + 1
};
#define INTTYPEBITS (GCTYPEBITS - 1)
#define VAL_MAX (EMACS_INT_MAX >> (GCTYPEBITS - 1))
#define USE_LSB_TAG 1
#define VALMASK (USE_LSB_TAG ? - (1 << GCTYPEBITS) : VAL_MAX)
#define GCALIGNMENT 8
#define GCALIGNED_UNION_MEMBER char alignas (GCALIGNMENT) gcaligned;
typedef struct Lisp_X *Lisp_Word;
#define lisp_h_XLI(o) ((EMACS_INT) (o))
#define lisp_h_XIL(i) ((Lisp_Object) (i))
#define lisp_h_XLP(o) ((void *) (o))
#define lisp_h_Qnil 0
#define lisp_h_PSEUDOVECTORP(a,code)                            \
(lisp_h_VECTORLIKEP((a)) &&                                   \
    ((XUNTAG ((a), Lisp_Vectorlike, union vectorlike_header)->size     \
    & (PSEUDOVECTOR_FLAG | PVEC_TYPE_MASK))                    \
    == (PSEUDOVECTOR_FLAG | ((code) << PSEUDOVECTOR_AREA_BITS))))

#define lisp_h_CHECK_FIXNUM(x) CHECK_TYPE (FIXNUMP (x), Qfixnump, x)
#define lisp_h_CHECK_SYMBOL(x) CHECK_TYPE (SYMBOLP (x), Qsymbolp, x)
#define lisp_h_CHECK_TYPE(ok, predicate, x) \
((ok) ? (void) 0 : wrong_type_argument (predicate, x))
#define lisp_h_CONSP(x) TAGGEDP (x, Lisp_Cons)
#define lisp_h_BASE_EQ(x, y) (XLI (x) == XLI (y))
#define lisp_h_BASE2_EQ(x, y)				    \
(BASE_EQ (x, y)					    \
    || (symbols_with_pos_enabled				    \
    && SYMBOL_WITH_POS_P (x)				    \
    && BASE_EQ (XSYMBOL_WITH_POS (x)->sym, y)))

#define lisp_h_EQ(x, y)                                     \
((XLI ((x)) == XLI ((y)))                                 \
    || (symbols_with_pos_enabled                             \
    && (SYMBOL_WITH_POS_P ((x))                          \
    ? (BARE_SYMBOL_P ((y))                           \
    ? XLI (XSYMBOL_WITH_POS((x))->sym) == XLI (y) \
    : SYMBOL_WITH_POS_P((y))                      \
    && (XLI (XSYMBOL_WITH_POS((x))->sym)        \
    == XLI (XSYMBOL_WITH_POS((y))->sym)))   \
    : (SYMBOL_WITH_POS_P ((y))                       \
    && BARE_SYMBOL_P ((x))                        \
    && (XLI (x) == XLI ((XSYMBOL_WITH_POS ((y)))->sym))))))

#define lisp_h_FIXNUMP(x) \
(! (((unsigned) (XLI (x) >> (USE_LSB_TAG ? 0 : FIXNUM_BITS)) \
    - (unsigned) (Lisp_Int0 >> !USE_LSB_TAG)) \
    & ((1 << INTTYPEBITS) - 1)))
#define lisp_h_FLOATP(x) TAGGEDP (x, Lisp_Float)
#define lisp_h_NILP(x)  BASE_EQ (x, Qnil)
#define lisp_h_SET_SYMBOL_VAL(sym, v) \
(eassert ((sym)->u.s.redirect == SYMBOL_PLAINVAL), \
    (sym)->u.s.val.value = (v))
#define lisp_h_SYMBOL_CONSTANT_P(sym) \
(XSYMBOL (sym)->u.s.trapped_write == SYMBOL_NOWRITE)
#define lisp_h_SYMBOL_TRAPPED_WRITE_P(sym) (XSYMBOL (sym)->u.s.trapped_write)
#define lisp_h_SYMBOL_VAL(sym) \
(eassert ((sym)->u.s.redirect == SYMBOL_PLAINVAL), (sym)->u.s.val.value)
#define lisp_h_SYMBOL_WITH_POS_P(x) PSEUDOVECTORP ((x), PVEC_SYMBOL_WITH_POS)
#define lisp_h_BARE_SYMBOL_P(x) TAGGEDP ((x), Lisp_Symbol)
#define lisp_h_SYMBOLP(x) ((BARE_SYMBOL_P ((x)) ||               \
    (symbols_with_pos_enabled && (SYMBOL_WITH_POS_P ((x))))))
#define lisp_h_TAGGEDP(a, tag) \
(! (((unsigned) (XLI (a) >> (USE_LSB_TAG ? 0 : VALBITS)) \
    - (unsigned) (tag)) \
    & ((1 << GCTYPEBITS) - 1)))
#define lisp_h_VECTORLIKEP(x) TAGGEDP (x, Lisp_Vectorlike)
#define lisp_h_XCAR(c) XCONS (c)->u.s.car
#define lisp_h_XCDR(c) XCONS (c)->u.s.u.cdr
#define lisp_h_XCONS(a) \
(eassert (CONSP (a)), XUNTAG (a, Lisp_Cons, struct Lisp_Cons))
#define lisp_h_XHASH(a) XUFIXNUM_RAW (a)
#define lisp_h_make_fixnum_wrap(n) \
XIL ((EMACS_INT) (((EMACS_UINT) (n) << INTTYPEBITS) + Lisp_Int0))
#define lisp_h_make_fixnum(n) lisp_h_make_fixnum_wrap (n)
#define lisp_h_XFIXNUM_RAW(a) (XLI (a) >> INTTYPEBITS)
#define lisp_h_XTYPE(a) ((enum Lisp_Type) (XLI (a) & ~VALMASK))
#define XLI(o) lisp_h_XLI (o)
#define XIL(i) lisp_h_XIL (i)
#define XLP(o) lisp_h_XLP (o)
#define BARE_SYMBOL_P(x) lisp_h_BARE_SYMBOL_P (x)
#define CHECK_FIXNUM(x) lisp_h_CHECK_FIXNUM (x)
#define CHECK_SYMBOL(x) lisp_h_CHECK_SYMBOL (x)
#define CHECK_TYPE(ok, predicate, x) lisp_h_CHECK_TYPE (ok, predicate, x)
#define CONSP(x) lisp_h_CONSP (x)
#define BASE_EQ(x, y) lisp_h_BASE_EQ (x, y)
#define BASE2_EQ(x, y) lisp_h_BASE2_EQ (x, y)
#define FLOATP(x) lisp_h_FLOATP (x)
#define FIXNUMP(x) lisp_h_FIXNUMP (x)
#define NILP(x) lisp_h_NILP (x)
#define SET_SYMBOL_VAL(sym, v) lisp_h_SET_SYMBOL_VAL (sym, v)
#define SYMBOL_CONSTANT_P(sym) lisp_h_SYMBOL_CONSTANT_P (sym)
#define SYMBOL_TRAPPED_WRITE_P(sym) lisp_h_SYMBOL_TRAPPED_WRITE_P (sym)
#define SYMBOL_VAL(sym) lisp_h_SYMBOL_VAL (sym)
#define TAGGEDP(a, tag) lisp_h_TAGGEDP (a, tag)
#define VECTORLIKEP(x) lisp_h_VECTORLIKEP (x)
#define XCAR(c) lisp_h_XCAR (c)
#define XCDR(c) lisp_h_XCDR (c)
#define XCONS(a) lisp_h_XCONS (a)
#define XHASH(a) lisp_h_XHASH (a)
#define make_fixnum(n) lisp_h_make_fixnum (n)
#define XFIXNUM_RAW(a) lisp_h_XFIXNUM_RAW (a)
#define XTYPE(a) lisp_h_XTYPE (a)
#define INTMASK (EMACS_INT_MAX >> (INTTYPEBITS - 1))
#define case_Lisp_Int case Lisp_Int0: case Lisp_Int1
enum Lisp_Type
{
    Lisp_Symbol = 0,
    Lisp_Type_Unused0 = 1,
    Lisp_Int0 = 2,
    Lisp_Int1 = USE_LSB_TAG ? 6 : 3,
    Lisp_String = 4,
    Lisp_Vectorlike = 5,
    Lisp_Cons = USE_LSB_TAG ? 3 : 6,
    Lisp_Float = 7
};
typedef Lisp_Word Lisp_Object;
#define LISP_INITIALLY(w) (w)

#define DEFUN_ARGS_MANY		(ptrdiff_t, Lisp_Object *)
#define DEFUN_ARGS_UNEVALLED	(Lisp_Object)
#define DEFUN_ARGS_0	(void)
#define DEFUN_ARGS_1	(Lisp_Object)
#define DEFUN_ARGS_2	(Lisp_Object, Lisp_Object)
#define DEFUN_ARGS_3	(Lisp_Object, Lisp_Object, Lisp_Object)
#define DEFUN_ARGS_4	(Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object)
#define DEFUN_ARGS_5	(Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, \
    Lisp_Object)
#define DEFUN_ARGS_6	(Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, \
    Lisp_Object, Lisp_Object)
#define DEFUN_ARGS_7	(Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, \
    Lisp_Object, Lisp_Object, Lisp_Object)
#define DEFUN_ARGS_8	(Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, \
    Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object)
#define EXFUN(fnname, maxargs) \
extern Lisp_Object fnname DEFUN_ARGS_ ## maxargs

#define XUNTAG(a, type, ctype) ((ctype *) \
    ((char *) XLP (a) - LISP_WORD_TAG (type)))

typedef char *untagged_ptr;
typedef uintptr_t Lisp_Word_tag;
#define LISP_WORD_TAG(tag) \
((Lisp_Word_tag) (tag) << (USE_LSB_TAG ? 0 : VALBITS))
#define TAG_PTR(tag, ptr) \
LISP_INITIALLY ((Lisp_Word) ((untagged_ptr) (ptr) + LISP_WORD_TAG (tag)))

#define ENUM_BF(TYPE) enum TYPE
enum symbol_interned
{
    SYMBOL_UNINTERNED = 0,
    SYMBOL_INTERNED = 1,
    SYMBOL_INTERNED_IN_INITIAL_OBARRAY = 2
};

enum symbol_redirect
{
    SYMBOL_PLAINVAL  = 4,
    SYMBOL_VARALIAS  = 1,
    SYMBOL_LOCALIZED = 2,
    SYMBOL_FORWARDED = 3
};

enum symbol_trapped_write
{
    SYMBOL_UNTRAPPED_WRITE = 0,
    SYMBOL_NOWRITE = 1,
    SYMBOL_TRAPPED_WRITE = 2
};
typedef struct { void const *fwdptr; } lispfwd;
struct Lisp_Buffer_Local_Value
{
    bool_bf local_if_set : 1;
    bool_bf found : 1;
    lispfwd fwd;
    Lisp_Object where;
    Lisp_Object defcell;
    Lisp_Object valcell;
};

struct Lisp_Symbol
{
    union
    {
        struct
        {
            bool_bf gcmarkbit : 1;
            ENUM_BF (symbol_redirect) redirect : 3;
            ENUM_BF (symbol_trapped_write) trapped_write : 2;
            unsigned interned : 2;
            bool_bf declared_special : 1;
            bool_bf pinned : 1;
            Lisp_Object name;
            union {
                Lisp_Object value;
                struct Lisp_Symbol *alias;
                struct Lisp_Buffer_Local_Value *blv;
                lispfwd fwd;
            } val;
            Lisp_Object function;
            Lisp_Object plist;
            struct Lisp_Symbol *next;
        } s;
    } u;
};
#include "globals.h"

INLINE EMACS_INT
XFIXNUM (Lisp_Object a)
{
    eassert (FIXNUMP (a));
    return XFIXNUM_RAW (a);
}
INLINE EMACS_INT
XFIXNAT (Lisp_Object a)
{
    eassert (FIXNUMP (a));
    EMACS_INT int0 = Lisp_Int0;
    EMACS_INT result = USE_LSB_TAG ? XFIXNUM (a) : XLI (a) - (int0 << VALBITS);
    eassume (0 <= result);
    return result;
}

INLINE Lisp_Object
make_lisp_ptr (void *ptr, enum Lisp_Type type)
{
    Lisp_Object a = TAG_PTR (type, ptr);
    eassert (TAGGEDP (a, type) && XUNTAG (a, type, char) == ptr);
    return a;
}
#define XSETINT(a, b) ((a) = make_fixnum (b))
#define XSETFASTINT(a, b) ((a) = make_fixed_natnum (b))
#define XSETCONS(a, b) ((a) = make_lisp_ptr (b, Lisp_Cons))
#define XSETVECTOR(a, b) ((a) = make_lisp_ptr (b, Lisp_Vectorlike))
#define XSETSTRING(a, b) ((a) = make_lisp_ptr (b, Lisp_String))
#define XSETSYMBOL(a, b) ((a) = make_lisp_symbol (b))
#define XSETFLOAT(a, b) ((a) = make_lisp_ptr (b, Lisp_Float))

struct Lisp_Float
{
    union
    {
        double data;
        struct Lisp_Float *chain;
    } u;
};
INLINE struct Lisp_Float *
XFLOAT (Lisp_Object a)
{
    eassert (FLOATP (a));
    return XUNTAG (a, Lisp_Float, struct Lisp_Float);
}
INLINE double
XFLOAT_DATA (Lisp_Object f)
{
    return XFLOAT (f)->u.data;
}

struct Lisp_Cons {
    union {
        struct {
            Lisp_Object car;
            union {
                Lisp_Object cdr;
                struct Lisp_Cons *chain;
            } u;
        } s;
    } u;
};
INLINE Lisp_Object *
xcar_addr (Lisp_Object c)
{
    return &XCONS (c)->u.s.car;
}
INLINE Lisp_Object *
xcdr_addr (Lisp_Object c)
{
    return &XCONS (c)->u.s.u.cdr;
}
INLINE void
XSETCAR (Lisp_Object c, Lisp_Object n)
{
    *xcar_addr (c) = n;
}
INLINE void
XSETCDR (Lisp_Object c, Lisp_Object n)
{
    *xcdr_addr (c) = n;
}

union vectorlike_header {
    ptrdiff_t size;
};
enum { NIL_IS_ZERO = 1 };
INLINE void
memclear (void *p, ptrdiff_t nbytes)
{
    eassert (0 <= nbytes);
    memset (p, 0, nbytes);
}
# define ARRAY_MARK_FLAG PTRDIFF_MIN
enum pvec_type {
    PVEC_NORMAL_VECTOR,
    PVEC_FREE,
    PVEC_BIGNUM,
    PVEC_MARKER,
    PVEC_OVERLAY,
    PVEC_FINALIZER,
    PVEC_SYMBOL_WITH_POS,
    PVEC_MISC_PTR,
    PVEC_USER_PTR,
    PVEC_PROCESS,
    PVEC_FRAME,
    PVEC_WINDOW,
    PVEC_BOOL_VECTOR,
    PVEC_BUFFER,
    PVEC_HASH_TABLE,
    PVEC_TERMINAL,
    PVEC_WINDOW_CONFIGURATION,
    PVEC_SUBR,
    PVEC_OTHER,
    PVEC_XWIDGET,
    PVEC_XWIDGET_VIEW,
    PVEC_THREAD,
    PVEC_MUTEX,
    PVEC_CONDVAR,
    PVEC_MODULE_FUNCTION,
    PVEC_NATIVE_COMP_UNIT,
    PVEC_TS_PARSER,
    PVEC_TS_NODE,
    PVEC_TS_COMPILED_QUERY,
    PVEC_SQLITE,
    PVEC_COMPILED,
    PVEC_CHAR_TABLE,
    PVEC_SUB_CHAR_TABLE,
    PVEC_RECORD,
    PVEC_FONT
};
enum More_Lisp_Bits
{
    PSEUDOVECTOR_SIZE_BITS = 12,
    PSEUDOVECTOR_SIZE_MASK = (1 << PSEUDOVECTOR_SIZE_BITS) - 1,
    PSEUDOVECTOR_REST_BITS = 12,
    PSEUDOVECTOR_REST_MASK = (((1 << PSEUDOVECTOR_REST_BITS) - 1)
    << PSEUDOVECTOR_SIZE_BITS),
    PSEUDOVECTOR_AREA_BITS = PSEUDOVECTOR_SIZE_BITS + PSEUDOVECTOR_REST_BITS,
    PVEC_TYPE_MASK = 0x3f << PSEUDOVECTOR_AREA_BITS
};

#define ARRAYELTS(arr) (sizeof (arr) / sizeof (arr)[0])

typedef struct interval *INTERVAL;
struct Lisp_String
{
    union
    {
        struct
        {
            ptrdiff_t size;
            ptrdiff_t size_byte;
            INTERVAL intervals;
            unsigned char *data;
        } s;
        struct Lisp_String *next;
    } u;
};
INLINE bool
STRINGP (Lisp_Object x)
{
    return TAGGEDP (x, Lisp_String);
}
INLINE struct Lisp_String *
XSTRING (Lisp_Object a)
{
    eassert (STRINGP (a));
    return XUNTAG (a, Lisp_String, struct Lisp_String);
}
INLINE bool
STRING_MULTIBYTE (Lisp_Object str)
{
    return 0 <= XSTRING (str)->u.s.size_byte;
}
INLINE unsigned char *
SDATA (Lisp_Object string)
{
    return XSTRING (string)->u.s.data;
}
INLINE unsigned char
SREF (Lisp_Object string, ptrdiff_t index)
{
  return SDATA (string)[index];
}
INLINE ptrdiff_t
STRING_BYTES (struct Lisp_String *s)
{
    ptrdiff_t nbytes = s->u.s.size_byte < 0 ? s->u.s.size : s->u.s.size_byte;
    eassume (0 <= nbytes);
    return nbytes;
}
INLINE ptrdiff_t
SBYTES (Lisp_Object string)
{
    return STRING_BYTES (XSTRING (string));
}
INLINE ptrdiff_t
SCHARS (Lisp_Object string)
{
  ptrdiff_t nchars = XSTRING (string)->u.s.size;
  eassume (0 <= nchars);
  return nchars;
}
#if TODO_NELISP_LATER_AND
#define STRING_SET_UNIBYTE(STR)				\
do {							\
    if (XSTRING (STR)->u.s.size == 0)			\
        (STR) = empty_unibyte_string;			\
    else						\
        XSTRING (STR)->u.s.size_byte = -1;		\
} while (false)
#else
#define STRING_SET_UNIBYTE(STR)				\
XSTRING (STR)->u.s.size_byte = -1;
#endif
INLINE Lisp_Object
dead_object (void)
{
#if TODO_NELISP_LATER_AND
    return make_lisp_ptr (NULL, Lisp_String);
#endif
    // Workaround for compiling with `zig cc` causing a crash.
    return (Lisp_Word)((Lisp_Word_tag)NULL+(Lisp_Word_tag)Lisp_String);
}

struct Lisp_Vector
{
    union vectorlike_header header;
    Lisp_Object contents[FLEXIBLE_ARRAY_MEMBER];
};
struct Lisp_Bool_Vector
{
    union vectorlike_header header;
    EMACS_INT size;
    bits_word data[FLEXIBLE_ARRAY_MEMBER];
};
enum
{
    header_size = offsetof (struct Lisp_Vector, contents),
    bool_header_size = offsetof (struct Lisp_Bool_Vector, data),
    word_size = sizeof (Lisp_Object)
};
# define PSEUDOVECTOR_FLAG (PTRDIFF_MAX - PTRDIFF_MAX / 2)
#define MOST_POSITIVE_FIXNUM (EMACS_INT_MAX >> INTTYPEBITS)
#define MOST_NEGATIVE_FIXNUM (-1 - MOST_POSITIVE_FIXNUM)
#define XSETPVECTYPE(v, code)						\
((v)->header.size |= PSEUDOVECTOR_FLAG | ((code) << PSEUDOVECTOR_AREA_BITS))
#define PVECHEADERSIZE(code, lispsize, restsize) \
(PSEUDOVECTOR_FLAG | ((code) << PSEUDOVECTOR_AREA_BITS) \
    | ((restsize) << PSEUDOVECTOR_SIZE_BITS) | (lispsize))
#define XSETPVECTYPESIZE(v, code, lispsize, restsize)		\
((v)->header.size = PVECHEADERSIZE (code, lispsize, restsize))
INLINE struct Lisp_Vector *
XVECTOR (Lisp_Object a)
{
    eassert (VECTORLIKEP (a));
    return XUNTAG (a, Lisp_Vectorlike, struct Lisp_Vector);
}
INLINE enum pvec_type
PSEUDOVECTOR_TYPE (const struct Lisp_Vector *v)
{
    ptrdiff_t size = v->header.size;
    return (enum pvec_type)(size & PSEUDOVECTOR_FLAG
    ? (size & PVEC_TYPE_MASK) >> PSEUDOVECTOR_AREA_BITS
    : PVEC_NORMAL_VECTOR);
}
extern ptrdiff_t vectorlike_nbytes (const union vectorlike_header *hdr);
INLINE ptrdiff_t
vector_nbytes (const struct Lisp_Vector *v)
{
    return vectorlike_nbytes (&v->header);
}
INLINE bool
PSEUDOVECTOR_TYPEP (const union vectorlike_header *a, enum pvec_type code)
{
    return ((a->size & (PSEUDOVECTOR_FLAG | PVEC_TYPE_MASK))
    == (PSEUDOVECTOR_FLAG | (code << PSEUDOVECTOR_AREA_BITS)));
}
INLINE ptrdiff_t
ASIZE (Lisp_Object array)
{
    ptrdiff_t size = XVECTOR (array)->header.size;
    eassume (0 <= size);
    return size;
}
INLINE ptrdiff_t
gc_asize (Lisp_Object array)
{
  return XVECTOR (array)->header.size & ~ARRAY_MARK_FLAG;
}
INLINE Lisp_Object
AREF (Lisp_Object array, ptrdiff_t idx)
{
  eassert (0 <= idx && idx < gc_asize (array));
  return XVECTOR (array)->contents[idx];
}
INLINE Lisp_Object *
aref_addr (Lisp_Object array, ptrdiff_t idx)
{
  eassert (0 <= idx && idx <= gc_asize (array));
  return & XVECTOR (array)->contents[idx];
}
INLINE bool
PSEUDOVECTORP (Lisp_Object a, int code)
{
  return lisp_h_PSEUDOVECTORP (a, code);
}
INLINE bool
VECTORP (Lisp_Object x)
{
    return VECTORLIKEP (x) && ! (ASIZE (x) & PSEUDOVECTOR_FLAG);
}

INLINE bool
(SYMBOL_WITH_POS_P) (Lisp_Object x)
{
  return lisp_h_SYMBOL_WITH_POS_P (x);
}
struct Lisp_Symbol_With_Pos
{
  union vectorlike_header header;
  Lisp_Object sym;
  Lisp_Object pos;
};
INLINE struct Lisp_Symbol_With_Pos *
XSYMBOL_WITH_POS (Lisp_Object a)
{
    TODO
    eassert (SYMBOL_WITH_POS_P (a));
    return XUNTAG (a, Lisp_Vectorlike, struct Lisp_Symbol_With_Pos);
}
INLINE bool
(EQ) (Lisp_Object x, Lisp_Object y)
{
  return lisp_h_EQ (x, y);
}
INLINE bool
(SYMBOLP) (Lisp_Object x)
{
  return lisp_h_SYMBOLP (x);
}
INLINE struct Lisp_Symbol *
(XBARE_SYMBOL) (Lisp_Object a)
{
    eassert (BARE_SYMBOL_P (a));
    intptr_t i = (intptr_t) XUNTAG (a, Lisp_Symbol, struct Lisp_Symbol);
    void *p = (char *) lispsym + i;
    return (struct Lisp_Symbol *) p;
}
INLINE struct Lisp_Symbol *
(XSYMBOL) (Lisp_Object a)
{
    eassert (SYMBOLP ((a)));
#if TODO_NELISP_LATER_AND
    if (!symbols_with_pos_enabled || BARE_SYMBOL_P (a))
        return XBARE_SYMBOL (a);
    return XBARE_SYMBOL (XSYMBOL_WITH_POS (a)->sym);
#else
    return XBARE_SYMBOL (a);
#endif
}
INLINE Lisp_Object
make_lisp_symbol (struct Lisp_Symbol *sym)
{
    /* GCC 7 x86-64 generates faster code if lispsym is
     cast to char * rather than to intptr_t.  */
    char *symoffset = (char *) ((char *) sym - (char *) lispsym);
    Lisp_Object a = TAG_PTR (Lisp_Symbol, symoffset);
    eassert (XSYMBOL (a) == sym);
    return a;
}
INLINE Lisp_Object
builtin_lisp_symbol (int index)
{
  return make_lisp_symbol (&lispsym[index]);
}
INLINE void
set_symbol_function (Lisp_Object sym, Lisp_Object function)
{
  XSYMBOL (sym)->u.s.function = function;
}

INLINE void
set_symbol_plist (Lisp_Object sym, Lisp_Object plist)
{
  XSYMBOL (sym)->u.s.plist = plist;
}

INLINE void
set_symbol_next (Lisp_Object sym, struct Lisp_Symbol *next)
{
  XSYMBOL (sym)->u.s.next = next;
}
INLINE void
SET_SYMBOL_FWD (struct Lisp_Symbol *sym, void const *v)
{
  eassume (sym->u.s.redirect == SYMBOL_FORWARDED && v);
  sym->u.s.val.fwd.fwdptr = v;
}
INLINE Lisp_Object
SYMBOL_NAME (Lisp_Object sym)
{
  return XSYMBOL (sym)->u.s.name;
}
INLINE void
make_symbol_constant (Lisp_Object sym)
{
  XSYMBOL (sym)->u.s.trapped_write = SYMBOL_NOWRITE;
}

struct Lisp_Subr {
    union vectorlike_header header;
    union {
        Lisp_Object (*a0) (void);
        Lisp_Object (*a1) (Lisp_Object);
        Lisp_Object (*a2) (Lisp_Object, Lisp_Object);
        Lisp_Object (*a3) (Lisp_Object, Lisp_Object, Lisp_Object);
        Lisp_Object (*a4) (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);
        Lisp_Object (*a5) (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);
        Lisp_Object (*a6) (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);
        Lisp_Object (*a7) (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);
        Lisp_Object (*a8) (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);
        Lisp_Object (*aUNEVALLED) (Lisp_Object args);
        Lisp_Object (*aMANY) (ptrdiff_t, Lisp_Object *);
    } function;
    short min_args, max_args;
    const char *symbol_name;
    union {
        const char *string;
        Lisp_Object native;
    } intspec;
    Lisp_Object command_modes;
    EMACS_INT doc;
#ifdef HAVE_NATIVE_COMP
    Lisp_Object native_comp_u;
    char *native_c_name;
    Lisp_Object lambda_list;
    Lisp_Object type;
#endif
};
union Aligned_Lisp_Subr {
    struct Lisp_Subr s;
};

#ifdef ENABLE_CHECKING
#error "TODO"
#else
#define set_obj_check(L,idx)
#define check_obj(L,idx)
#endif

INLINE Lisp_Object userdata_to_obj(lua_State *L,int idx){
    if (!lua_checkstack(L,lua_gettop(L)+5))
        unrecoverable_lua_error(L,"Lua stack overflow");
    check_obj(L,idx);

    if (lua_islightuserdata(L,idx)){
        Lisp_Object obj=(Lisp_Object)lua_touserdata(L,idx);
        eassert(FIXNUMP(obj));
        return obj;
    } else {
        Lisp_Object obj=*(Lisp_Object*)lua_touserdata(L,idx);
        eassert(!FIXNUMP(obj));
        return obj;
    }
}

INLINE void push_obj(lua_State *L, Lisp_Object obj){
    if (!lua_checkstack(L,lua_gettop(L)+10))
        unrecoverable_lua_error(L,"Lua stack overflow");
    if (FIXNUMP(obj)) {
        lua_pushlightuserdata(L,obj);
        set_obj_check(L,-1);
        return;
    }
    union {
        Lisp_Object l;
        char c[sizeof(Lisp_Object)];
    } u;
    u.l=obj;

    lua_getfield(L,LUA_ENVIRONINDEX,"memtbl");
    eassert(lua_istable(L,-1));
    // (-1)memtbl
    lua_pushlstring(L,u.c,sizeof(Lisp_Object));
    // (-2)memtbl, (-1)idx
    lua_gettable(L,-2);
    // (-2)memtbl, (-1)nil/obj
    if (lua_isuserdata(L,-1)){
        // (-2)memtbl, (-1)obj
        Lisp_Object* ptr=(Lisp_Object*)lua_touserdata(L,-1);
        eassert(*ptr==obj);
        lua_remove(L,-2);
        // (-1)obj
        return;
    }
    // (-2)memtbl, (-1)nil
    lua_pop(L,2);
    //
    Lisp_Object* ptr=(Lisp_Object*)lua_newuserdata(L,sizeof(Lisp_Object));
    *ptr=obj;
    // (-1)obj
    lua_getfield(L,LUA_ENVIRONINDEX,"memtbl");
    eassert(lua_istable(L,-1));
    // (-2)obj, (-1)memtbl
    lua_pushlstring(L,u.c,sizeof(Lisp_Object));
    // (-3)obj, (-2)memtbl, (-1)idx
    lua_pushvalue(L,-3);
    // (-4)obj, (-3)memtbl, (-2)idx, (-1)obj
    lua_settable(L,-3);
    // (-2)obj, (-1)memtbl
    lua_pop(L,1);
    // (-1)obj
    set_obj_check(L,-1);
    return;
}

#define pub __attribute__((visibility("default")))
#define ret(...)

INLINE void check_nargs(lua_State *L,int nargs){
    if (global_lua_state==NULL)
        luaL_error(L,"Nelisp is not inited (please run `require('nelisp.c').init()`)");
    if (unrecoverable_error)
        luaL_error(L,"Previous error was unrecoverable, please restart Neovim");
    if (nargs != lua_gettop(L))
        luaL_error(L,"Wrong number of arguments: expected %d, got %d",nargs,lua_gettop(L));
}
INLINE void check_isnumber(lua_State *L,int n){
    if (!lua_isnumber(L,n))
        luaL_error(L,"Wrong argument #%d: expected number, got %s",n,lua_typename(L,lua_type(L,n)));
}
INLINE void check_isstring(lua_State *L,int n){
    if (!lua_isstring(L,n))
        luaL_error(L,"Wrong argument #%d: expected string, got %s",n,lua_typename(L,lua_type(L,n)));
}
INLINE void check_isobject(lua_State *L,int n){
    if (!lua_isuserdata(L,n))
        luaL_error(L,"Wrong argument #%d: expected userdata(lisp object), got %s",n,lua_typename(L,lua_type(L,n)));
    check_obj(L,n);
}

INLINE void check_istable(lua_State *L,int n){
    if (!lua_istable(L,n))
        luaL_error(L,"Wrong argument #%d: expected table, got %s",n,lua_typename(L,lua_type(L,n)));
}

static jmp_buf mainloop_jmp;
static jmp_buf mainloop_return_jmp;
static void (*mainloop_func)(void);
INLINE void enter_mainloop(void (*f)(void)){
    mainloop_func=f;
    if (!setjmp(mainloop_return_jmp)){
        longjmp(mainloop_jmp,1);
    };
}
static void (*tcall_func_var)(lua_State *L);
INLINE void tcall_func(void){
    tcall_func_var(global_lua_state);
}
// If this is not a macro, then it will crash
#define tcall(L,f)\
    if (global_lua_state!=L)\
        TODO /*use lua_xmove to move between the states*/ \
    tcall_func_var=f;\
    enter_mainloop(tcall_func);
void mainloop(void){
    TODO_NELISP_LATER // move function to somewhere else
    while (1){
        if (!setjmp(mainloop_jmp))
            longjmp(mainloop_return_jmp,1);
        mainloop_func();
    }
}

#define DEFUN_LUA_1(fname)\
void t_l##fname(lua_State *L) {\
    Lisp_Object obj=fname(userdata_to_obj(L,1));\
    push_obj(L,obj);\
}\
int __attribute__((visibility("default"))) l##fname(lua_State *L) {\
    check_nargs(L,1);\
    check_isobject(L,1);\
    tcall(L,t_l##fname);\
    return 1;\
}
#define DEFUN_LUA_2(fname)\
void t_l##fname(lua_State *L) {\
    Lisp_Object obj=fname(userdata_to_obj(L,1),userdata_to_obj(L,2));\
    push_obj(L,obj);\
}\
int __attribute__((visibility("default"))) l##fname(lua_State *L) {\
    check_nargs(L,2);\
    check_isobject(L,1);\
    check_isobject(L,2);\
    tcall(L,t_l##fname);\
    return 1;\
}

#define DEFUN(lname, fnname, sname, minargs, maxargs, intspec, doc) \
static union Aligned_Lisp_Subr sname =                            \
{{{ PVEC_SUBR << PSEUDOVECTOR_AREA_BITS },			    \
{ .a ## maxargs = fnname },				    \
minargs, maxargs, lname, {intspec}, lisp_h_Qnil, 0}};	    \
DEFUN_LUA_##maxargs(fnname)\
Lisp_Object fnname


enum Lisp_Fwd_Type
  {
    Lisp_Fwd_Int,
    Lisp_Fwd_Bool,
    Lisp_Fwd_Obj,
    Lisp_Fwd_Buffer_Obj,
    Lisp_Fwd_Kboard_Obj
  };
struct Lisp_Objfwd
  {
    enum Lisp_Fwd_Type type;
    Lisp_Object *objvar;
  };
extern void defvar_lisp (struct Lisp_Objfwd const *, char const *);
#define DEFVAR_LISP(lname, vname, doc)		\
  do {						\
    static struct Lisp_Objfwd const o_fwd	\
      = {Lisp_Fwd_Obj, &globals.f_##vname};	\
    defvar_lisp (&o_fwd, lname);		\
  } while (false)
#define DEFSYM(sym, name)

#endif /* EMACS_LISP_H */
