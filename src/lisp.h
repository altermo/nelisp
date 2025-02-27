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
#include <stdckdint.h>

#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>

static lua_State *global_lua_state;
static bool unrecoverable_error;
#define TODO_NELISP_LATER (void)0
#define TODO_NELISP_LATER_ELSE true
#define TODO_NELISP_LATER_AND false
static inline _Noreturn void TODO_(const char *file, int line){
    luaL_error(global_lua_state,"TODO at %s:%d",file,line);
    __builtin_unreachable();
}
#define TODO TODO_(__FILE__,__LINE__)
#define UNUSED(x) (void)x

// LSP being anyoing about them existing and not existing, so just define them here
#ifndef LONG_WIDTH
#define LONG_WIDTH   __LONG_WIDTH__
#define ULONG_WIDTH  __LONG_WIDTH__
#endif
#ifndef SIZE_WIDTH
#define SIZE_WIDTH   __WORDSIZE
#endif
#ifndef USHRT_WIDTH
#define USHRT_WIDTH 16
#endif

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

#define symbols_with_pos_enabled 1
_Noreturn INLINE void emacs_abort(void) {
    luaL_error(global_lua_state,"emacs_abort");
    __builtin_unreachable();
}

INLINE bool
pdumper_object_p (const void *obj)
{
    UNUSED(obj);
    TODO_NELISP_LATER;
    return false;
}
INLINE bool
pdumper_marked_p (const void *obj)
{
    UNUSED(obj);
    TODO_NELISP_LATER;
    return false;
}
INLINE void
pdumper_set_marked (const void *obj)
{
    UNUSED(obj);
    TODO_NELISP_LATER;
}
INLINE bool
pdumper_cold_object_p (const void *obj)
{
    UNUSED(obj);
    TODO_NELISP_LATER;
    return false;
}

//!IMPORTANT: just to get things started, a lot of things will be presumed (like 64-bit ptrs) or not optimized

#undef min
#undef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define ARRAYELTS(arr) (sizeof (arr) / sizeof (arr)[0])
#define GCTYPEBITS 3
typedef long int EMACS_INT;
typedef unsigned long EMACS_UINT;
enum { EMACS_INT_WIDTH = LONG_WIDTH, EMACS_UINT_WIDTH = ULONG_WIDTH };
#define EMACS_INT_MAX LONG_MAX
#define pI "l"
typedef size_t bits_word;
# define BITS_WORD_MAX SIZE_MAX
enum { BITS_PER_BITS_WORD = SIZE_WIDTH };
#define pD "t"
#define AVOID _Noreturn void
#define eassert(cond) ((void) (false && (cond))) /* Check COND compiles.  */
#define eassume(cond) assume (cond)
enum Lisp_Bits
{
    VALBITS = EMACS_INT_WIDTH - GCTYPEBITS,
    FIXNUM_BITS = VALBITS + 1
};
#define INTTYPEBITS (GCTYPEBITS - 1)
#define VAL_MAX (EMACS_INT_MAX >> (GCTYPEBITS - 1))
#define USE_LSB_TAG (VAL_MAX / 2 < INTPTR_MAX)
#if !USE_LSB_TAG
#error "TODO"
#endif
#define VALMASK (USE_LSB_TAG ? - (1 << GCTYPEBITS) : VAL_MAX)
#define GCALIGNMENT 8
#define GCALIGNED_UNION_MEMBER char alignas (GCALIGNMENT) gcaligned;
#define GCALIGNED_STRUCT
#if !(EMACS_INT_MAX == INTPTR_MAX)
#error "TODO"
#endif
typedef struct Lisp_X *Lisp_Word;
#define lisp_h_XLI(o) ((EMACS_INT) (o))
#define lisp_h_XIL(i) ((Lisp_Object) (i))
#define lisp_h_XLP(o) ((void *) (o))
#define lisp_h_Qnil 0
#define lisp_h_CHECK_FIXNUM(x) CHECK_TYPE (FIXNUMP (x), Qfixnump, x)
#define lisp_h_CHECK_SYMBOL(x) CHECK_TYPE (SYMBOLP (x), Qsymbolp, x)
#define lisp_h_CHECK_TYPE(ok, predicate, x) \
((ok) ? (void) 0 : wrong_type_argument (predicate, x))
#define lisp_h_CONSP(x) TAGGEDP (x, Lisp_Cons)
#define lisp_h_BASE_EQ(x, y) (XLI (x) == XLI (y))
#define lisp_h_FIXNUMP(x) \
(! (((unsigned) (XLI (x) >> (USE_LSB_TAG ? 0 : FIXNUM_BITS)) \
    - (unsigned) (Lisp_Int0 >> !USE_LSB_TAG)) \
    & ((1 << INTTYPEBITS) - 1)))
#define lisp_h_FLOATP(x) TAGGEDP (x, Lisp_Float)
#define lisp_h_NILP(x)  BASE_EQ (x, Qnil)
#define lisp_h_SYMBOL_CONSTANT_P(sym) \
(XSYMBOL (sym)->u.s.trapped_write == SYMBOL_NOWRITE)
#define lisp_h_SYMBOL_TRAPPED_WRITE_P(sym) (XSYMBOL (sym)->u.s.trapped_write)
#define lisp_h_SYMBOL_WITH_POS_P(x) PSEUDOVECTORP (x, PVEC_SYMBOL_WITH_POS)
#define lisp_h_BARE_SYMBOL_P(x) TAGGEDP (x, Lisp_Symbol)
#define lisp_h_TAGGEDP(a, tag) \
(! (((unsigned) (XLI (a) >> (USE_LSB_TAG ? 0 : VALBITS)) \
    - (unsigned) (tag)) \
    & ((1 << GCTYPEBITS) - 1)))
#define lisp_h_VECTORLIKEP(x) TAGGEDP (x, Lisp_Vectorlike)
#define lisp_h_XCAR(c) XCONS (c)->u.s.car
#define lisp_h_XCDR(c) XCONS (c)->u.s.u.cdr
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
#define FLOATP(x) lisp_h_FLOATP (x)
#define FIXNUMP(x) lisp_h_FIXNUMP (x)
#define NILP(x) lisp_h_NILP (x)
#define SYMBOL_CONSTANT_P(sym) lisp_h_SYMBOL_CONSTANT_P (sym)
#define SYMBOL_TRAPPED_WRITE_P(sym) lisp_h_SYMBOL_TRAPPED_WRITE_P (sym)
#define TAGGEDP(a, tag) lisp_h_TAGGEDP (a, tag)
#define VECTORLIKEP(x) lisp_h_VECTORLIKEP (x)
#define XCAR(c) lisp_h_XCAR (c)
#define XCDR(c) lisp_h_XCDR (c)
#define XHASH(a) lisp_h_XHASH (a)
#define make_fixnum(n) lisp_h_make_fixnum (n)
#define XFIXNUM_RAW(a) lisp_h_XFIXNUM_RAW (a)
#define XTYPE(a) lisp_h_XTYPE (a)
#define INTMASK (EMACS_INT_MAX >> (INTTYPEBITS - 1))
#define case_Lisp_Int case Lisp_Int0: case Lisp_Int1
#define ENUM_BF(TYPE) enum TYPE
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
enum Lisp_Fwd_Type
{
    Lisp_Fwd_Int,
    Lisp_Fwd_Bool,
    Lisp_Fwd_Obj,
    Lisp_Fwd_Buffer_Obj,
    Lisp_Fwd_Kboard_Obj
};
typedef Lisp_Word Lisp_Object;
#define LISP_INITIALLY(w) (w)

#define XUNTAG(a, type, ctype) \
((ctype *) ((uintptr_t) XLP (a) - (uintptr_t) LISP_WORD_TAG (type)))

typedef struct { void const *fwdptr; } lispfwd;
enum symbol_interned
{
    SYMBOL_UNINTERNED,
    SYMBOL_INTERNED,
    SYMBOL_INTERNED_IN_INITIAL_OBARRAY
};
enum symbol_redirect
{
    SYMBOL_PLAINVAL,
    SYMBOL_VARALIAS,
    SYMBOL_LOCALIZED,
    SYMBOL_FORWARDED
};
enum symbol_trapped_write
{
    SYMBOL_UNTRAPPED_WRITE,
    SYMBOL_NOWRITE,
    SYMBOL_TRAPPED_WRITE
};
struct Lisp_Symbol
{
union
{
    struct
    {
        bool_bf gcmarkbit : 1;
        ENUM_BF (symbol_redirect) redirect : 2;
        ENUM_BF (symbol_trapped_write) trapped_write : 2;
        ENUM_BF (symbol_interned) interned : 2;
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
    GCALIGNED_UNION_MEMBER
} u;
};
#define EXFUN(fnname, maxargs) \
extern Lisp_Object fnname DEFUN_ARGS_ ## maxargs
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
typedef uintptr_t Lisp_Word_tag;
#define LISP_WORD_TAG(tag) \
((Lisp_Word_tag) (tag) << (USE_LSB_TAG ? 0 : VALBITS))
#define TAG_PTR_INITIALLY(tag, p) \
LISP_INITIALLY ((Lisp_Word) ((uintptr_t) (p) + LISP_WORD_TAG (tag)))

#define POWER_OF_2(n) (((n) & ((n) - 1)) == 0)
#define ROUNDUP(x, y) (POWER_OF_2 (y)					\
    ? ((y) - 1 + (x)) & ~ ((y) - 1)			\
    : ((y) - 1 + (x)) - ((y) - 1 + (x)) % (y))

#include "globals.h"

union vectorlike_header
{
    ptrdiff_t size;
};
struct Lisp_Symbol_With_Pos
{
    union vectorlike_header header;
    Lisp_Object sym;              /* A symbol */
    Lisp_Object pos;              /* A fixnum */
} GCALIGNED_STRUCT;
#define ARRAY_MARK_FLAG PTRDIFF_MIN
#define PSEUDOVECTOR_FLAG (PTRDIFF_MAX - PTRDIFF_MAX / 2)
enum pvec_type
{
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
    PVEC_OBARRAY,
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

    PVEC_CLOSURE,
    PVEC_CHAR_TABLE,
    PVEC_SUB_CHAR_TABLE,
    PVEC_RECORD,
    PVEC_FONT,
    PVEC_TAG_MAX = PVEC_FONT
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
#define MOST_POSITIVE_FIXNUM (EMACS_INT_MAX >> INTTYPEBITS)
#define MOST_NEGATIVE_FIXNUM (-1 - MOST_POSITIVE_FIXNUM)
INLINE bool
PSEUDOVECTORP (Lisp_Object a, int code)
{
    return (lisp_h_VECTORLIKEP (a)
    && ((XUNTAG (a, Lisp_Vectorlike, union vectorlike_header)->size
         & (PSEUDOVECTOR_FLAG | PVEC_TYPE_MASK))
        == (PSEUDOVECTOR_FLAG | (code << PSEUDOVECTOR_AREA_BITS))));
}
INLINE bool
(BARE_SYMBOL_P) (Lisp_Object x)
{
    return lisp_h_BARE_SYMBOL_P (x);
}
INLINE bool
(SYMBOL_WITH_POS_P) (Lisp_Object x)
{
    return lisp_h_SYMBOL_WITH_POS_P (x);
}
INLINE bool
SYMBOLP (Lisp_Object x)
{
    return (BARE_SYMBOL_P (x)
    || (symbols_with_pos_enabled && SYMBOL_WITH_POS_P (x)));
}
INLINE struct Lisp_Symbol_With_Pos *
XSYMBOL_WITH_POS (Lisp_Object a)
{
    eassert (SYMBOL_WITH_POS_P (a));
    return XUNTAG (a, Lisp_Vectorlike, struct Lisp_Symbol_With_Pos);
}
INLINE Lisp_Object
XSYMBOL_WITH_POS_SYM (Lisp_Object a)
{
    Lisp_Object sym = XSYMBOL_WITH_POS (a)->sym;
    eassume (BARE_SYMBOL_P (sym));
    return sym;
}
INLINE struct Lisp_Symbol *
XBARE_SYMBOL (Lisp_Object a)
{
    eassert (BARE_SYMBOL_P (a));
    intptr_t i = (intptr_t) XUNTAG (a, Lisp_Symbol, struct Lisp_Symbol);
    void *p = (char *) lispsym + i;
    return p;
}
INLINE struct Lisp_Symbol *
XSYMBOL (Lisp_Object a)
{
    if (!BARE_SYMBOL_P (a))
    {
        eassume (symbols_with_pos_enabled);
        a = XSYMBOL_WITH_POS_SYM (a);
    }
    return XBARE_SYMBOL (a);
}
INLINE Lisp_Object
make_lisp_symbol_internal (struct Lisp_Symbol *sym)
{
    char *symoffset = (char *) ((char *) sym - (char *) lispsym);
    Lisp_Object a = TAG_PTR_INITIALLY (Lisp_Symbol, symoffset);
    return a;
}
INLINE Lisp_Object
make_lisp_symbol (struct Lisp_Symbol *sym)
{
    Lisp_Object a = make_lisp_symbol_internal (sym);
    eassert (XBARE_SYMBOL (a) == sym);
    return a;
}
INLINE Lisp_Object
builtin_lisp_symbol (int index)
{
    return make_lisp_symbol_internal (&lispsym[index]);
}
#define FIXNUM_OVERFLOW_P(i) \
(! ((0 <= (i) || MOST_NEGATIVE_FIXNUM <= (i)) && (i) <= MOST_POSITIVE_FIXNUM))
INLINE Lisp_Object
(make_fixnum) (EMACS_INT n)
{
    eassert (!FIXNUM_OVERFLOW_P (n));
    return lisp_h_make_fixnum_wrap (n);
}
INLINE bool
(FIXNUMP) (Lisp_Object x)
{
    return lisp_h_FIXNUMP (x);
}
INLINE EMACS_INT
XFIXNUM (Lisp_Object a)
{
    eassert (FIXNUMP (a));
    return XFIXNUM_RAW (a);
}
INLINE bool
EQ (Lisp_Object x, Lisp_Object y)
{
    return BASE_EQ ((symbols_with_pos_enabled && SYMBOL_WITH_POS_P (x)
                    ? XSYMBOL_WITH_POS_SYM (x) : x),
                    (symbols_with_pos_enabled && SYMBOL_WITH_POS_P (y)
                    ? XSYMBOL_WITH_POS_SYM (y) : y));
}
INLINE Lisp_Object
make_lisp_ptr (void *ptr, enum Lisp_Type type)
{
    Lisp_Object a = TAG_PTR_INITIALLY (type, ptr);
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

INLINE Lisp_Object
dead_object (void)
{
    return make_lisp_ptr (NULL, Lisp_String);
}

#define XSETPVECTYPE(v, code)						\
((v)->header.size |= PSEUDOVECTOR_FLAG | ((code) << PSEUDOVECTOR_AREA_BITS))
#define PVECHEADERSIZE(code, lispsize, restsize) \
(PSEUDOVECTOR_FLAG | ((code) << PSEUDOVECTOR_AREA_BITS) \
    | ((restsize) << PSEUDOVECTOR_SIZE_BITS) | (lispsize))
#define XSETPVECTYPESIZE(v, code, lispsize, restsize)		\
((v)->header.size = PVECHEADERSIZE (code, lispsize, restsize))
#define XSETPSEUDOVECTOR(a, b, code) \
XSETTYPED_PSEUDOVECTOR (a, b,					\
                        (XUNTAG (a, Lisp_Vectorlike,		\
                                 union vectorlike_header)	\
                         ->size),				\
                         code)
#define XSETTYPED_PSEUDOVECTOR(a, b, size, code)			\
(XSETVECTOR (a, b),							\
    eassert ((size & (PSEUDOVECTOR_FLAG | PVEC_TYPE_MASK))		\
             == (PSEUDOVECTOR_FLAG | (code << PSEUDOVECTOR_AREA_BITS))))

#define XSETSUBR(a, b) XSETPSEUDOVECTOR (a, b, PVEC_SUBR)

typedef struct interval *INTERVAL;
struct Lisp_Cons
{
union
{
    struct
    {
        Lisp_Object car;
        union
        {
            Lisp_Object cdr;
            struct Lisp_Cons *chain;
        } u;
    } s;
    GCALIGNED_UNION_MEMBER
} u;
};
INLINE bool
(NILP) (Lisp_Object x)
{
    return lisp_h_NILP (x);
}
INLINE struct Lisp_Cons *
XCONS (Lisp_Object a)
{
    eassert (CONSP (a));
    return XUNTAG (a, Lisp_Cons, struct Lisp_Cons);
}
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
INLINE Lisp_Object
CAR (Lisp_Object c)
{
  if (CONSP (c))
    return XCAR (c);
  if (!NILP (c))
    TODO; //wrong_type_argument (Qlistp, c);
  return Qnil;
}
INLINE Lisp_Object
CDR (Lisp_Object c)
{
  if (CONSP (c))
    return XCDR (c);
  if (!NILP (c))
    TODO; //wrong_type_argument (Qlistp, c);
  return Qnil;
}
INLINE Lisp_Object
CAR_SAFE (Lisp_Object c)
{
  return CONSP (c) ? XCAR (c) : Qnil;
}
INLINE Lisp_Object
CDR_SAFE (Lisp_Object c)
{
  return CONSP (c) ? XCDR (c) : Qnil;
}

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
    GCALIGNED_UNION_MEMBER
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
#define STRING_BYTES_BOUND  \
((ptrdiff_t) min (MOST_POSITIVE_FIXNUM, min (SIZE_MAX, PTRDIFF_MAX) - 1))
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
do {							\
    XSTRING (STR)->u.s.size_byte = -1;		\
} while (false)
#endif
INLINE unsigned char *
SDATA (Lisp_Object string)
{
    return XSTRING (string)->u.s.data;
}
INLINE char *
SSDATA (Lisp_Object string)
{
    return (char *) SDATA (string);
}
INLINE unsigned char
SREF (Lisp_Object string, ptrdiff_t index)
{
    return SDATA (string)[index];
}
INLINE ptrdiff_t
SCHARS (Lisp_Object string)
{
    ptrdiff_t nchars = XSTRING (string)->u.s.size;
    eassume (0 <= nchars);
    return nchars;
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

struct Lisp_Vector
{
    union vectorlike_header header;
    Lisp_Object contents[FLEXIBLE_ARRAY_MEMBER];
} GCALIGNED_STRUCT;
INLINE struct Lisp_Vector *
XVECTOR (Lisp_Object a)
{
    eassert (VECTORLIKEP (a));
    return XUNTAG (a, Lisp_Vectorlike, struct Lisp_Vector);
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
INLINE bool
VECTORP (Lisp_Object x)
{
    return VECTORLIKEP (x) && ! (ASIZE (x) & PSEUDOVECTOR_FLAG);
}
INLINE enum pvec_type
PSEUDOVECTOR_TYPE (const struct Lisp_Vector *v)
{
    ptrdiff_t size = v->header.size;
    return (size & PSEUDOVECTOR_FLAG
    ? (size & PVEC_TYPE_MASK) >> PSEUDOVECTOR_AREA_BITS
    : PVEC_NORMAL_VECTOR);
}
INLINE bool
PSEUDOVECTOR_TYPEP (const union vectorlike_header *a, enum pvec_type code)
{
    return ((a->size & (PSEUDOVECTOR_FLAG | PVEC_TYPE_MASK))
    == (PSEUDOVECTOR_FLAG | (code << PSEUDOVECTOR_AREA_BITS)));
}

struct Lisp_Bool_Vector
{
    union vectorlike_header header;
    EMACS_INT size;
    bits_word data[FLEXIBLE_ARRAY_MEMBER];
} GCALIGNED_STRUCT;
enum
{
    header_size = offsetof (struct Lisp_Vector, contents),
    bool_header_size = offsetof (struct Lisp_Bool_Vector, data),
    word_size = sizeof (Lisp_Object)
};

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

enum { NIL_IS_ZERO = iQnil == 0 && Lisp_Symbol == 0 };
INLINE void
memclear (void *p, ptrdiff_t nbytes)
{
    eassert (0 <= nbytes);
    memset (p, 0, nbytes);
}
#define VECSIZE(type)						\
((sizeof (type) - header_size + word_size - 1) / word_size)

struct Lisp_Subr
{
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
} GCALIGNED_STRUCT;
union Aligned_Lisp_Subr
{
struct Lisp_Subr s;
GCALIGNED_UNION_MEMBER
  };
INLINE bool
SUBRP (Lisp_Object a)
{
  return PSEUDOVECTORP (a, PVEC_SUBR);
}
INLINE struct Lisp_Subr *
XSUBR (Lisp_Object a)
{
  eassert (SUBRP (a));
  return &XUNTAG (a, Lisp_Vectorlike, union Aligned_Lisp_Subr)->s;
}

INLINE Lisp_Object
SYMBOL_VAL (struct Lisp_Symbol *sym)
{
    eassert (sym->u.s.redirect == SYMBOL_PLAINVAL);
    return sym->u.s.val.value;
}

INLINE void
SET_SYMBOL_VAL (struct Lisp_Symbol *sym, Lisp_Object v)
{
    eassert (sym->u.s.redirect == SYMBOL_PLAINVAL);
    sym->u.s.val.value = v;
}
INLINE void
SET_SYMBOL_FWD (struct Lisp_Symbol *sym, void const *v)
{
    eassume (sym->u.s.redirect == SYMBOL_FORWARDED && v);
    sym->u.s.val.fwd.fwdptr = v;
}

struct Lisp_Obarray
{
    union vectorlike_header header;
    Lisp_Object *buckets;
    unsigned size_bits;
    unsigned count;
};
INLINE bool
OBARRAYP (Lisp_Object a)
{
    return PSEUDOVECTORP (a, PVEC_OBARRAY);
}
INLINE struct Lisp_Obarray *
XOBARRAY (Lisp_Object a)
{
    eassert (OBARRAYP (a));
    return XUNTAG (a, Lisp_Vectorlike, struct Lisp_Obarray);
}
INLINE Lisp_Object
make_lisp_obarray (struct Lisp_Obarray *o)
{
    eassert (PSEUDOVECTOR_TYPEP (&o->header, PVEC_OBARRAY));
    return make_lisp_ptr (o, Lisp_Vectorlike);
}
INLINE ptrdiff_t
obarray_size (const struct Lisp_Obarray *o)
{
    return (ptrdiff_t)1 << o->size_bits;
}

typedef unsigned int hash_hash_t;

INLINE hash_hash_t
reduce_emacs_uint_to_hash_hash (EMACS_UINT x)
{
    return (sizeof x == sizeof (hash_hash_t)
    ? x
    : x ^ (x >> (8 * (sizeof x - sizeof (hash_hash_t)))));
}

INLINE ptrdiff_t
knuth_hash (hash_hash_t hash, unsigned bits)
{
    unsigned int h = hash;
    unsigned int alpha = 2654435769;
    unsigned int product = (h * alpha) & 0xffffffffu;
    unsigned long long int wide_product = product;
    return wide_product >> (32 - bits);
}

struct Lisp_Float
{
union
{
    double data;
    struct Lisp_Float *chain;
    GCALIGNED_UNION_MEMBER
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

INLINE EMACS_INT
XFIXNAT (Lisp_Object a)
{
    eassert (FIXNUMP (a));
    EMACS_INT int0 = Lisp_Int0;
    EMACS_INT result = USE_LSB_TAG ? XFIXNUM (a) : XLI (a) - (int0 << VALBITS);
    eassume (0 <= result);
    return result;
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
make_symbol_constant (Lisp_Object sym)
{
    XSYMBOL (sym)->u.s.trapped_write = SYMBOL_NOWRITE;
}

INLINE bool
NATIVE_COMP_FUNCTION_DYNP (Lisp_Object a)
{
    UNUSED(a);
    return false;
}

enum maxargs
  {
    MANY = -2,
    UNEVALLED = -1
  };

enum specbind_tag {
  SPECPDL_UNWIND,
  SPECPDL_UNWIND_ARRAY,
  SPECPDL_UNWIND_PTR,
  SPECPDL_UNWIND_INT,
  SPECPDL_UNWIND_INTMAX,
  SPECPDL_UNWIND_EXCURSION,
  SPECPDL_UNWIND_VOID,
  SPECPDL_BACKTRACE,
  SPECPDL_NOP,
  SPECPDL_LET,
  SPECPDL_LET_LOCAL,
  SPECPDL_LET_DEFAULT
};
typedef struct kboard KBOARD;
union specbinding
  {
    ENUM_BF (specbind_tag) kind : CHAR_BIT;
    struct {
      ENUM_BF (specbind_tag) kind : CHAR_BIT;
      void (*func) (Lisp_Object);
      Lisp_Object arg;
      EMACS_INT eval_depth;
    } unwind;
    struct {
      ENUM_BF (specbind_tag) kind : CHAR_BIT;
      ptrdiff_t nelts;
      Lisp_Object *array;
    } unwind_array;
    struct {
      ENUM_BF (specbind_tag) kind : CHAR_BIT;
      void (*func) (void *);
      void *arg;
      void (*mark) (void *);
    } unwind_ptr;
    struct {
      ENUM_BF (specbind_tag) kind : CHAR_BIT;
      void (*func) (int);
      int arg;
    } unwind_int;
    struct {
      ENUM_BF (specbind_tag) kind : CHAR_BIT;
      void (*func) (intmax_t);
      intmax_t arg;
    } unwind_intmax;
    struct {
      ENUM_BF (specbind_tag) kind : CHAR_BIT;
      Lisp_Object marker, window;
    } unwind_excursion;
    struct {
      ENUM_BF (specbind_tag) kind : CHAR_BIT;
      void (*func) (void);
    } unwind_void;
    struct {
      ENUM_BF (specbind_tag) kind : CHAR_BIT;
      Lisp_Object symbol, old_value;
      union {
	KBOARD *kbd;
	Lisp_Object buf;
      } where;
    } let;
    struct {
      ENUM_BF (specbind_tag) kind : CHAR_BIT;
      bool_bf debug_on_exit : 1;
      Lisp_Object function;
      Lisp_Object *args;
      ptrdiff_t nargs;
    } bt;
  };

union specbinding *specpdl_ptr;
union specbinding *specpdl_end;
union specbinding *specpdl;

typedef struct { ptrdiff_t bytes; } specpdl_ref;
INLINE specpdl_ref
wrap_specpdl_ref (ptrdiff_t bytes)
{
  return (specpdl_ref){.bytes = bytes};
}
INLINE ptrdiff_t
unwrap_specpdl_ref (specpdl_ref ref)
{
  return ref.bytes;
}
INLINE union specbinding *
specpdl_ref_to_ptr (specpdl_ref ref)
{
  return (union specbinding *)((char *)specpdl + unwrap_specpdl_ref (ref));
}
INLINE specpdl_ref
SPECPDL_INDEX (void)
{
  return wrap_specpdl_ref ((char *)specpdl_ptr - (char *)specpdl);
}


#if TODO_NELISP_LATER_ELSE
void circular_list (Lisp_Object list){
    UNUSED(list);
    TODO;
}
void maybe_quit (void){
}
#endif
struct for_each_tail_internal
{
  Lisp_Object tortoise;
  intptr_t max, n;
  unsigned short int q;
};
#define FOR_EACH_TAIL_INTERNAL(tail, cycle, check_quit)			\
  for (struct for_each_tail_internal li = { tail, 2, 0, 2 };		\
       CONSP (tail);							\
       ((tail) = XCDR (tail),						\
	((--li.q != 0							\
	  || ((check_quit) ? maybe_quit () : (void) 0, 0 < --li.n)	\
	  || (li.q = li.n = li.max <<= 1, li.n >>= USHRT_WIDTH,		\
	      li.tortoise = (tail), false))				\
	 && BASE_EQ (tail, li.tortoise))				\
	? (cycle) : (void) 0))
#define FOR_EACH_TAIL(tail) \
  FOR_EACH_TAIL_INTERNAL (tail, circular_list (tail), true)
enum Set_Internal_Bind
  {
    SET_INTERNAL_SET,
    SET_INTERNAL_BIND,
    SET_INTERNAL_UNBIND,
    SET_INTERNAL_THREAD_SWITCH,
  };


#ifdef ENABLE_CHECKING
#error "TODO"
#else
#define set_obj_check(L,idx)
#define check_obj(L,idx)
#endif

INLINE Lisp_Object userdata_to_obj(lua_State *L,int idx){
    if (!lua_checkstack(L,lua_gettop(L)+5))
        luaL_error(L,"Lua stack overflow");
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
        luaL_error(L,"Lua stack overflow");
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
static int tcall_func_n(lua_State *L){
    tcall_func_var(L);
    return 1;
}
INLINE void tcall_func(void){
    lua_pushcfunction(global_lua_state,tcall_func_n);
    lua_insert(global_lua_state,1);
    if (lua_pcall(global_lua_state,lua_gettop(global_lua_state)-1,1,0)){
        unrecoverable_error=true;
        lua_error(global_lua_state);
    }
}
// If this is not a macro, then it will crash
#define tcall(L,f)\
if (global_lua_state!=L)\
    TODO; /*use lua_xmove to move between the states*/ \
tcall_func_var=f;\
enter_mainloop(tcall_func);
void mainloop(void){
    TODO_NELISP_LATER; // move function to somewhere else
    while (1){
        if (!setjmp(mainloop_jmp))
            longjmp(mainloop_return_jmp,1);
        mainloop_func();
    }
}

#define DEF_TCALL_ARGS_0(fname)
#define DEF_TCALL_ARGS_1(fname) userdata_to_obj(L,1)
#define DEF_TCALL_ARGS_2(fname) DEF_TCALL_ARGS_1(fname),userdata_to_obj(L,2)
#define DEF_TCALL_ARGS_3(fname) DEF_TCALL_ARGS_2(fname),userdata_to_obj(L,3)
#define DEF_TCALL_ARGS_4(fname) DEF_TCALL_ARGS_3(fname),userdata_to_obj(L,4)
#define DEF_TCALL_ARGS_5(fname) DEF_TCALL_ARGS_4(fname),userdata_to_obj(L,5)
#define DEF_TCALL_ARGS_6(fname) DEF_TCALL_ARGS_5(fname),userdata_to_obj(L,6)
#define DEF_TCALL_ARGS_7(fname) DEF_TCALL_ARGS_6(fname),userdata_to_obj(L,7)
#define DEF_TCALL_ARGS_8(fname) DEF_TCALL_ARGS_7(fname),userdata_to_obj(L,8)
#define DEF_TCALL_ARGS_UNEVALLED(fname) userdata_to_obj(L,1)

#define DEFUN_LUA_N(fname,maxargs)\
void t_l##fname(lua_State *L) {\
Lisp_Object obj=fname(DEF_TCALL_ARGS_##maxargs(maxargs));\
push_obj(L,obj);\
}\
int __attribute__((visibility("default"))) l##fname(lua_State *L) {\
    check_nargs(L,maxargs);\
    static int i;\
    for (i=1;i<=maxargs;i++){\
        check_isobject(L,i);\
    }\
    tcall(L,t_l##fname);\
    return 1;\
}

#define DEFUN(lname, fnname, sname, minargs, maxargs, intspec, doc) \
static union Aligned_Lisp_Subr sname =                            \
{{{ PVEC_SUBR << PSEUDOVECTOR_AREA_BITS },			    \
{ .a ## maxargs = fnname },				    \
minargs, maxargs, lname, {intspec}, lisp_h_Qnil, 0}};	    \
DEFUN_LUA_N(fnname,maxargs)\
Lisp_Object fnname


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
extern void defsubr (union Aligned_Lisp_Subr *);
#endif
