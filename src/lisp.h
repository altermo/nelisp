#ifndef EMACS_LISP_H
#define EMACS_LISP_H

#include <alloca.h>
#include <errno.h>
#include <ieee754.h>
#include <inttypes.h>
#include <limits.h>
#include <setjmp.h>
#include <stdalign.h>
#include <stdbit.h>
#include <stdbool.h>
#include <stdckdint.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include <threads.h>

#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lua.h>

static inline _Noreturn void TODO_ (const char *file, int line);

static lua_State *_global_lua_state;
static bool unrecoverable_error;
#define TODO_NELISP_LATER (void) 0
#define TODO_NELISP_LATER_ELSE true
#define TODO_NELISP_LATER_AND false
#define TODO TODO_ (__FILE__, __LINE__)
#define UNUSED(x) (void) x
#define STATIC_ASSERT(expr, msg) \
  (void) (sizeof (struct { int static_error___##msg##__ : ((expr) ? 1 : -1); }))

// LSP being anyoing about them existing and not existing, so just define them
// here
#ifndef LONG_WIDTH
# define LONG_WIDTH __LONG_WIDTH__
# define ULONG_WIDTH __LONG_WIDTH__
#endif
#ifndef SIZE_WIDTH
# define SIZE_WIDTH __WORDSIZE
#endif
#ifndef USHRT_WIDTH
# define USHRT_WIDTH 16
#endif

#define assume(R) ((R) ? (void) 0 : __builtin_unreachable ())
static void maybe_quit (void);
// Taken from lib/intprops.h
#define _GL_TYPE_SIGNED(t) (!((t) 0 < (t) - 1))
#define _GL_TYPE_WIDTH(t) (sizeof (t) * CHAR_BIT)
#define TYPE_SIGNED(t) _GL_TYPE_SIGNED (t)
#define TYPE_WIDTH(t) _GL_TYPE_WIDTH (t)
#define TYPE_MINIMUM(t) ((t) ~TYPE_MAXIMUM (t))
#define TYPE_MAXIMUM(t)            \
  ((t) (!TYPE_SIGNED (t) ? (t) - 1 \
                         : ((((t) 1 << (TYPE_WIDTH (t) - 2)) - 1) * 2 + 1)))
// Taken from sysdep.c
int
emacs_fstatat (int dirfd, char const *filename, void *st, int flags)
{
  int r;
  while ((r = fstatat (dirfd, filename, st, flags)) != 0 && errno == EINTR)
    maybe_quit ();
  return r;
}
// Taken from config.h
#define ATTRIBUTE_FORMAT_PRINTF(string_index, first_to_check) \
  __attribute__ ((__format__ (__printf__, string_index, first_to_check)))
#define DIRECTORY_SEP '/'
#define IS_DIRECTORY_SEP(_c_) ((_c_) == DIRECTORY_SEP)
#define IS_ANY_SEP(_c_) (IS_DIRECTORY_SEP (_c_))
#define IS_DEVICE_SEP(_c_) 0
// Taken from conf_post.h
#ifdef lint
# define UNINIT \
   = {          \
     0,         \
   }
#else
# define UNINIT
#endif
#define INLINE EXTERN_INLINE
#define EXTERN_INLINE static inline __attribute__ ((unused))
#define NO_INLINE __attribute__ ((__noinline__))
#define FLEXIBLE_ARRAY_MEMBER /**/
#define FLEXALIGNOF(type) (sizeof (type) & ~(sizeof (type) - 1))
#define FLEXSIZEOF(type, member, n)                         \
  ((offsetof (type, member) + FLEXALIGNOF (type) - 1 + (n)) \
   & ~(FLEXALIGNOF (type) - 1))
typedef bool bool_bf;
#define FALLTHROUGH __attribute__ ((fallthrough))

#define symbols_with_pos_enabled 1
_Noreturn INLINE void
emacs_abort (void)
{
  TODO;
  __builtin_unreachable ();
}

INLINE bool
pdumper_object_p (const void *obj)
{
  UNUSED (obj);
  TODO_NELISP_LATER;
  return false;
}
INLINE bool
pdumper_marked_p (const void *obj)
{
  UNUSED (obj);
  TODO_NELISP_LATER;
  return false;
}
INLINE void
pdumper_set_marked (const void *obj)
{
  UNUSED (obj);
  TODO_NELISP_LATER;
}
INLINE bool
pdumper_cold_object_p (const void *obj)
{
  UNUSED (obj);
  TODO_NELISP_LATER;
  return false;
}

//! IMPORTANT: just to get things started, a lot of things will be presumed
//! (like 64-bit ptrs) or not optimized

#undef min
#undef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define ARRAYELTS(arr) (sizeof (arr) / sizeof (arr)[0])
#define GCTYPEBITS 3
typedef long int EMACS_INT;
typedef unsigned long EMACS_UINT;
enum
{
  EMACS_INT_WIDTH = LONG_WIDTH,
  EMACS_UINT_WIDTH = ULONG_WIDTH
};
#define EMACS_INT_MAX LONG_MAX
#define pI "l"
typedef size_t bits_word;
#define BITS_WORD_MAX SIZE_MAX
enum
{
  BITS_PER_BITS_WORD = SIZE_WIDTH
};
#define pD "t"
#define AVOID _Noreturn void
#define eassert(cond) (cond ? (void) 0 : TODO)
#define eassume(cond) (cond ? (void) 0 : TODO)
enum Lisp_Bits
{
  VALBITS = EMACS_INT_WIDTH - GCTYPEBITS,
  FIXNUM_BITS = VALBITS + 1
};
#define INTTYPEBITS (GCTYPEBITS - 1)
#define VAL_MAX (EMACS_INT_MAX >> (GCTYPEBITS - 1))
#define USE_LSB_TAG (VAL_MAX / 2 < INTPTR_MAX)
#if !USE_LSB_TAG
# error "TODO"
#endif
#define VALMASK (USE_LSB_TAG ? -(1 << GCTYPEBITS) : VAL_MAX)
#define GCALIGNMENT 8
#define GCALIGNED_UNION_MEMBER char alignas (GCALIGNMENT) gcaligned;
#define GCALIGNED_STRUCT
#if !(EMACS_INT_MAX == INTPTR_MAX)
# error "TODO"
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
#define lisp_h_FIXNUMP(x)                                     \
  (!(((unsigned) (XLI (x) >> (USE_LSB_TAG ? 0 : FIXNUM_BITS)) \
      - (unsigned) (Lisp_Int0 >> !USE_LSB_TAG))               \
     & ((1 << INTTYPEBITS) - 1)))
#define lisp_h_FLOATP(x) TAGGEDP (x, Lisp_Float)
#define lisp_h_NILP(x) BASE_EQ (x, Qnil)
#define lisp_h_SYMBOL_CONSTANT_P(sym) \
  (XSYMBOL (sym)->u.s.trapped_write == SYMBOL_NOWRITE)
#define lisp_h_SYMBOL_TRAPPED_WRITE_P(sym) (XSYMBOL (sym)->u.s.trapped_write)
#define lisp_h_SYMBOL_WITH_POS_P(x) PSEUDOVECTORP (x, PVEC_SYMBOL_WITH_POS)
#define lisp_h_BARE_SYMBOL_P(x) TAGGEDP (x, Lisp_Symbol)
#define lisp_h_TAGGEDP(a, tag)                                                \
  (!(((unsigned) (XLI (a) >> (USE_LSB_TAG ? 0 : VALBITS)) - (unsigned) (tag)) \
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
#define case_Lisp_Int \
case Lisp_Int0:       \
case Lisp_Int1
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

INLINE void
CHECK_IMPURE (Lisp_Object obj, void *ptr)
{
  UNUSED (obj);
  UNUSED (ptr);
  TODO_NELISP_LATER_AND;
}
extern AVOID wrong_type_argument (Lisp_Object, Lisp_Object);

#define XUNTAG(a, type, ctype) \
  ((ctype *) ((uintptr_t) XLP (a) - (uintptr_t) LISP_WORD_TAG (type)))

typedef struct
{
  void const *fwdptr;
} lispfwd;
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
      union
      {
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
#define EXFUN(fnname, maxargs) extern Lisp_Object fnname DEFUN_ARGS_##maxargs
#define DEFUN_ARGS_MANY (ptrdiff_t, Lisp_Object *)
#define DEFUN_ARGS_UNEVALLED (Lisp_Object)
#define DEFUN_ARGS_0 (void)
#define DEFUN_ARGS_1 (Lisp_Object)
#define DEFUN_ARGS_2 (Lisp_Object, Lisp_Object)
#define DEFUN_ARGS_3 (Lisp_Object, Lisp_Object, Lisp_Object)
#define DEFUN_ARGS_4 (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object)
#define DEFUN_ARGS_5 \
  (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object)
#define DEFUN_ARGS_6 \
  (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object)
#define DEFUN_ARGS_7                                                \
  (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, \
   Lisp_Object, Lisp_Object)
#define DEFUN_ARGS_8                                                \
  (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, \
   Lisp_Object, Lisp_Object, Lisp_Object)
typedef uintptr_t Lisp_Word_tag;
#define LISP_WORD_TAG(tag) \
  ((Lisp_Word_tag) (tag) << (USE_LSB_TAG ? 0 : VALBITS))
#define TAG_PTR_INITIALLY(tag, p) \
  LISP_INITIALLY ((Lisp_Word) ((uintptr_t) (p) + LISP_WORD_TAG (tag)))
#define LISPSYM_INITIALLY(name) \
  TAG_PTR_INITIALLY (Lisp_Symbol, (intptr_t) ((i##name) * sizeof *lispsym))

#define POWER_OF_2(n) (((n) & ((n) - 1)) == 0)
#define ROUNDUP(x, y)                            \
  (POWER_OF_2 (y) ? ((y) - 1 + (x)) & ~((y) - 1) \
                  : ((y) - 1 + (x)) - ((y) - 1 + (x)) % (y))

#include "globals.h"

union vectorlike_header
{
  ptrdiff_t size;
};
struct Lisp_Symbol_With_Pos
{
  union vectorlike_header header;
  Lisp_Object sym; /* A symbol */
  Lisp_Object pos; /* A fixnum */
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
  PSEUDOVECTOR_REST_MASK
  = (((1 << PSEUDOVECTOR_REST_BITS) - 1) << PSEUDOVECTOR_SIZE_BITS),
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
INLINE bool (BARE_SYMBOL_P) (Lisp_Object x) { return lisp_h_BARE_SYMBOL_P (x); }
INLINE bool (SYMBOL_WITH_POS_P) (Lisp_Object x)
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
INLINE Lisp_Object
maybe_remove_pos_from_symbol (Lisp_Object x)
{
  return (symbols_with_pos_enabled && SYMBOL_WITH_POS_P (x)
            ? XSYMBOL_WITH_POS_SYM (x)
            : x);
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
  (!((0 <= (i) || MOST_NEGATIVE_FIXNUM <= (i)) && (i) <= MOST_POSITIVE_FIXNUM))
INLINE
Lisp_Object (make_fixnum) (EMACS_INT n)
{
  eassert (!FIXNUM_OVERFLOW_P (n));
  return lisp_h_make_fixnum_wrap (n);
}
INLINE bool (FIXNUMP) (Lisp_Object x) { return lisp_h_FIXNUMP (x); }
INLINE EMACS_INT
XFIXNUM (Lisp_Object a)
{
  eassert (FIXNUMP (a));
  return XFIXNUM_RAW (a);
}
INLINE EMACS_UINT
XUFIXNUM_RAW (Lisp_Object a)
{
  EMACS_UINT i = XLI (a);
  return USE_LSB_TAG ? i >> INTTYPEBITS : i & INTMASK;
}
INLINE EMACS_UINT
XUFIXNUM (Lisp_Object a)
{
  eassert (FIXNUMP (a));
  return XUFIXNUM_RAW (a);
}
INLINE bool
EQ (Lisp_Object x, Lisp_Object y)
{
  return BASE_EQ ((symbols_with_pos_enabled && SYMBOL_WITH_POS_P (x)
                     ? XSYMBOL_WITH_POS_SYM (x)
                     : x),
                  (symbols_with_pos_enabled && SYMBOL_WITH_POS_P (y)
                     ? XSYMBOL_WITH_POS_SYM (y)
                     : y));
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

#define XSETPVECTYPE(v, code) \
  ((v)->header.size |= PSEUDOVECTOR_FLAG | ((code) << PSEUDOVECTOR_AREA_BITS))
#define PVECHEADERSIZE(code, lispsize, restsize)          \
  (PSEUDOVECTOR_FLAG | ((code) << PSEUDOVECTOR_AREA_BITS) \
   | ((restsize) << PSEUDOVECTOR_SIZE_BITS) | (lispsize))
#define XSETPVECTYPESIZE(v, code, lispsize, restsize) \
  ((v)->header.size = PVECHEADERSIZE (code, lispsize, restsize))
#define XSETPSEUDOVECTOR(a, b, code)                        \
  XSETTYPED_PSEUDOVECTOR (a, b,                             \
                          (XUNTAG (a, Lisp_Vectorlike,      \
                                   union vectorlike_header) \
                             ->size),                       \
                          code)
#define XSETTYPED_PSEUDOVECTOR(a, b, size, code)          \
  (XSETVECTOR (a, b),                                     \
   eassert ((size & (PSEUDOVECTOR_FLAG | PVEC_TYPE_MASK)) \
            == (PSEUDOVECTOR_FLAG | (code << PSEUDOVECTOR_AREA_BITS))))

#define XSETSUBR(a, b) XSETPSEUDOVECTOR (a, b, PVEC_SUBR)
#define XSETBUFFER(a, b) XSETPSEUDOVECTOR (a, b, PVEC_BUFFER)

INLINE Lisp_Object
make_int (intmax_t n)
{
  return FIXNUM_OVERFLOW_P (n) ? (TODO, NULL) : make_fixnum (n);
}

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
INLINE bool (NILP) (Lisp_Object x) { return lisp_h_NILP (x); }
INLINE void
CHECK_CONS (Lisp_Object x)
{
  CHECK_TYPE (CONSP (x), Qconsp, x);
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
    wrong_type_argument (Qlistp, c);
  return Qnil;
}
INLINE Lisp_Object
CDR (Lisp_Object c)
{
  if (CONSP (c))
    return XCDR (c);
  if (!NILP (c))
    wrong_type_argument (Qlistp, c);
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
INLINE void
CHECK_STRING (Lisp_Object x)
{
  CHECK_TYPE (STRINGP (x), Qstringp, x);
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
#define STRING_BYTES_BOUND \
  ((ptrdiff_t) min (MOST_POSITIVE_FIXNUM, min (SIZE_MAX, PTRDIFF_MAX) - 1))
#define STRING_SET_UNIBYTE(STR)            \
  do                                       \
    {                                      \
      if (XSTRING (STR)->u.s.size == 0)    \
        (STR) = empty_unibyte_string;      \
      else                                 \
        XSTRING (STR)->u.s.size_byte = -1; \
    }                                      \
  while (false)
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
INLINE void
CHECK_STRING_NULL_BYTES (Lisp_Object string)
{
  CHECK_TYPE (memchr (SSDATA (string), '\0', SBYTES (string)) == NULL,
              Qfilenamep, string);
}
INLINE bool
string_immovable_p (Lisp_Object str)
{
  return XSTRING (str)->u.s.size_byte == -3;
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
  return VECTORLIKEP (x) && !(ASIZE (x) & PSEUDOVECTOR_FLAG);
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

INLINE bool
BOOL_VECTOR_P (Lisp_Object a)
{
  return PSEUDOVECTORP (a, PVEC_BOOL_VECTOR);
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
  return &XVECTOR (array)->contents[idx];
}

INLINE void
ASET (Lisp_Object array, ptrdiff_t idx, Lisp_Object val)
{
  eassert (0 <= idx && idx < ASIZE (array));
  XVECTOR (array)->contents[idx] = val;
}

enum
{
  NIL_IS_ZERO = iQnil == 0 && Lisp_Symbol == 0
};
INLINE void
memclear (void *p, ptrdiff_t nbytes)
{
  eassert (0 <= nbytes);
  memset (p, 0, nbytes);
}
#define VECSIZE(type) \
  ((sizeof (type) - header_size + word_size - 1) / word_size)

#define PSEUDOVECSIZE(type, lastlispfield)                  \
  (offsetof (type, lastlispfield) + word_size < header_size \
     ? 0                                                    \
     : (offsetof (type, lastlispfield) + word_size - header_size) / word_size)

INLINE bool
ASCII_CHAR_P (intmax_t c)
{
  return 0 <= c && c < 0x80;
}

struct Lisp_Subr
{
  union vectorlike_header header;
  union
  {
    Lisp_Object (*a0) (void);
    Lisp_Object (*a1) (Lisp_Object);
    Lisp_Object (*a2) (Lisp_Object, Lisp_Object);
    Lisp_Object (*a3) (Lisp_Object, Lisp_Object, Lisp_Object);
    Lisp_Object (*a4) (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);
    Lisp_Object (*a5) (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object,
                       Lisp_Object);
    Lisp_Object (*a6) (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object,
                       Lisp_Object, Lisp_Object);
    Lisp_Object (*a7) (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object,
                       Lisp_Object, Lisp_Object, Lisp_Object);
    Lisp_Object (*a8) (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object,
                       Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);
    Lisp_Object (*aUNEVALLED) (Lisp_Object args);
    Lisp_Object (*aMANY) (ptrdiff_t, Lisp_Object *);
  } function;
  short min_args, max_args;
  const char *symbol_name;
  union
  {
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

typedef jmp_buf sys_jmp_buf;
#define sys_setjmp(j) setjmp (j)
#define sys_longjmp(j, v) longjmp (j, v)
#include "thread.h"

INLINE Lisp_Object
SYMBOL_VAL (struct Lisp_Symbol *sym)
{
  eassert (sym->u.s.redirect == SYMBOL_PLAINVAL);
  return sym->u.s.val.value;
}
INLINE lispfwd
SYMBOL_FWD (struct Lisp_Symbol *sym)
{
  eassume (sym->u.s.redirect == SYMBOL_FORWARDED && sym->u.s.val.fwd.fwdptr);
  return sym->u.s.val.fwd;
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
INLINE Lisp_Object
SYMBOL_NAME (Lisp_Object sym)
{
  return XSYMBOL (sym)->u.s.name;
}

INLINE bool
SYMBOL_INTERNED_IN_INITIAL_OBARRAY_P (Lisp_Object sym)
{
  return XSYMBOL (sym)->u.s.interned == SYMBOL_INTERNED_IN_INITIAL_OBARRAY;
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
  return (ptrdiff_t) 1 << o->size_bits;
}

struct Lisp_Hash_Table;
typedef unsigned int hash_hash_t;
typedef enum
{
  Test_eql,
  Test_eq,
  Test_equal,
} hash_table_std_test_t;
struct hash_table_test
{
  hash_hash_t (*hashfn) (Lisp_Object, struct Lisp_Hash_Table *);

  Lisp_Object (*cmpfn) (Lisp_Object, Lisp_Object, struct Lisp_Hash_Table *);

  Lisp_Object user_hash_function;

  Lisp_Object user_cmp_function;

  Lisp_Object name;
};
typedef enum
{
  Weak_None,
  Weak_Key,
  Weak_Value,
  Weak_Key_Or_Value,
  Weak_Key_And_Value,
} hash_table_weakness_t;
typedef int32_t hash_idx_t;
struct Lisp_Hash_Table
{
  union vectorlike_header header;

  hash_idx_t *index;

  hash_hash_t *hash;

  Lisp_Object *key_and_value;

  const struct hash_table_test *test;

  hash_idx_t *next;

  hash_idx_t count;

  hash_idx_t next_free;

  hash_idx_t table_size;

  unsigned char index_bits;

  hash_table_weakness_t weakness : 3;

  hash_table_std_test_t frozen_test : 2;

  bool_bf purecopy : 1;

  bool_bf mutable : 1;

  struct Lisp_Hash_Table *next_weak;
} GCALIGNED_STRUCT;
#define INVALID_LISP_VALUE make_lisp_ptr (NULL, Lisp_Float)
#define HASH_UNUSED_ENTRY_KEY INVALID_LISP_VALUE
INLINE bool
hash_unused_entry_key_p (Lisp_Object key)
{
  return BASE_EQ (key, HASH_UNUSED_ENTRY_KEY);
}
INLINE bool
HASH_TABLE_P (Lisp_Object a)
{
  return PSEUDOVECTORP (a, PVEC_HASH_TABLE);
}
INLINE struct Lisp_Hash_Table *
XHASH_TABLE (Lisp_Object a)
{
  eassert (HASH_TABLE_P (a));
  return XUNTAG (a, Lisp_Vectorlike, struct Lisp_Hash_Table);
}
INLINE hash_hash_t
hash_from_key (struct Lisp_Hash_Table *h, Lisp_Object key)
{
  return h->test->hashfn (key, h);
}
INLINE Lisp_Object
make_lisp_hash_table (struct Lisp_Hash_Table *h)
{
  eassert (PSEUDOVECTOR_TYPEP (&h->header, PVEC_HASH_TABLE));
  return make_lisp_ptr (h, Lisp_Vectorlike);
}
INLINE Lisp_Object
HASH_KEY (const struct Lisp_Hash_Table *h, ptrdiff_t idx)
{
  eassert (idx >= 0 && idx < h->table_size);
  return h->key_and_value[2 * idx];
}
INLINE Lisp_Object
HASH_VALUE (const struct Lisp_Hash_Table *h, ptrdiff_t idx)
{
  eassert (idx >= 0 && idx < h->table_size);
  return h->key_and_value[2 * idx + 1];
}
INLINE hash_hash_t
HASH_HASH (const struct Lisp_Hash_Table *h, ptrdiff_t idx)
{
  eassert (idx >= 0 && idx < h->table_size);
  return h->hash[idx];
}
INLINE ptrdiff_t
HASH_TABLE_SIZE (const struct Lisp_Hash_Table *h)
{
  return h->table_size;
}
INLINE ptrdiff_t
hash_table_index_size (const struct Lisp_Hash_Table *h)
{
  return (ptrdiff_t) 1 << h->index_bits;
}
INLINE void
set_hash_key_slot (struct Lisp_Hash_Table *h, ptrdiff_t idx, Lisp_Object val)
{
  eassert (idx >= 0 && idx < h->table_size);
  h->key_and_value[2 * idx] = val;
}
INLINE void
set_hash_value_slot (struct Lisp_Hash_Table *h, ptrdiff_t idx, Lisp_Object val)
{
  eassert (idx >= 0 && idx < h->table_size);
  h->key_and_value[2 * idx + 1] = val;
  ;
}

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

INLINE enum Lisp_Fwd_Type
XFWDTYPE (lispfwd a)
{
  enum Lisp_Fwd_Type const *p = a.fwdptr;
  return *p;
}
INLINE bool
BUFFER_OBJFWDP (lispfwd a)
{
  return XFWDTYPE (a) == Lisp_Fwd_Buffer_Obj;
}
INLINE bool
KBOARD_OBJFWDP (lispfwd a)
{
  return XFWDTYPE (a) == Lisp_Fwd_Kboard_Obj;
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

INLINE Lisp_Object
make_uint (uintmax_t n)
{
  TODO_NELISP_LATER;
  return make_fixnum (n);
}

enum Lisp_Closure
{
  CLOSURE_ARGLIST = 0,
  CLOSURE_CODE = 1,
  CLOSURE_CONSTANTS = 2,
  CLOSURE_STACK_DEPTH = 3,
  CLOSURE_DOC_STRING = 4,
  CLOSURE_INTERACTIVE = 5
};
enum char_bits
{
  CHAR_ALT = 0x0400000,
  CHAR_SUPER = 0x0800000,
  CHAR_HYPER = 0x1000000,
  CHAR_SHIFT = 0x2000000,
  CHAR_CTL = 0x4000000,
  CHAR_META = 0x8000000,
  CHAR_MODIFIER_MASK
  = CHAR_ALT | CHAR_SUPER | CHAR_HYPER | CHAR_SHIFT | CHAR_CTL | CHAR_META,
  CHARACTERBITS = 22
};

INLINE bool
CLOSUREP (Lisp_Object a)
{
  return PSEUDOVECTORP (a, PVEC_CLOSURE);
}
INLINE bool
MARKERP (Lisp_Object x)
{
  return PSEUDOVECTORP (x, PVEC_MARKER);
}
INLINE bool
BIGNUMP (Lisp_Object x)
{
  return PSEUDOVECTORP (x, PVEC_BIGNUM);
}
INLINE bool
INTEGERP (Lisp_Object x)
{
  return FIXNUMP (x) || BIGNUMP (x);
}
INLINE bool
RECORDP (Lisp_Object a)
{
  return PSEUDOVECTORP (a, PVEC_RECORD);
}
INLINE bool
MODULE_FUNCTIONP (Lisp_Object o)
{
  return PSEUDOVECTORP (o, PVEC_MODULE_FUNCTION);
}

INLINE void
CHECK_LIST_END (Lisp_Object x, Lisp_Object y)
{
  CHECK_TYPE (NILP (x), Qlistp, y);
}
INLINE void
CHECK_STRING_CAR (Lisp_Object x)
{
  CHECK_TYPE (STRINGP (XCAR (x)), Qstringp, XCAR (x));
}

INLINE bool
FIXNATP (Lisp_Object x)
{
  return FIXNUMP (x) && 0 <= XFIXNUM (x);
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
INLINE bool
NUMBERP (Lisp_Object x)
{
  return INTEGERP (x) || FLOATP (x);
}

INLINE void
CHECK_INTEGER (Lisp_Object x)
{
  CHECK_TYPE (INTEGERP (x), Qintegerp, x);
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
  UNUSED (a);
  return false;
}

INLINE bool
integer_to_intmax (Lisp_Object num, intmax_t *n)
{
  if (FIXNUMP (num))
    {
      *n = XFIXNUM (num);
      return true;
    }
  else
    {
      TODO;
    }
}

INLINE int
elogb (unsigned long long int n)
{
  int width = stdc_bit_width (n);
  return width - 1;
}

enum maxargs
{
  MANY = -2,
  UNEVALLED = -1
};
#define CALLMANY(f, array) (f) (ARRAYELTS (array), array)
#define CALLN(f, ...) CALLMANY (f, ((Lisp_Object[]) { __VA_ARGS__ }))
#define calln(...) CALLN (Ffuncall, __VA_ARGS__)
#define call1 calln
#define call2 calln
#define call3 calln
#define call4 calln
#define call5 calln
#define call6 calln
#define call7 calln
#define call8 calln

enum specbind_tag
{
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
  struct
  {
    ENUM_BF (specbind_tag) kind : CHAR_BIT;
    void (*func) (Lisp_Object);
    Lisp_Object arg;
    EMACS_INT eval_depth;
  } unwind;
  struct
  {
    ENUM_BF (specbind_tag) kind : CHAR_BIT;
    ptrdiff_t nelts;
    Lisp_Object *array;
  } unwind_array;
  struct
  {
    ENUM_BF (specbind_tag) kind : CHAR_BIT;
    void (*func) (void *);
    void *arg;
    void (*mark) (void *);
  } unwind_ptr;
  struct
  {
    ENUM_BF (specbind_tag) kind : CHAR_BIT;
    void (*func) (int);
    int arg;
  } unwind_int;
  struct
  {
    ENUM_BF (specbind_tag) kind : CHAR_BIT;
    void (*func) (intmax_t);
    intmax_t arg;
  } unwind_intmax;
  struct
  {
    ENUM_BF (specbind_tag) kind : CHAR_BIT;
    Lisp_Object marker, window;
  } unwind_excursion;
  struct
  {
    ENUM_BF (specbind_tag) kind : CHAR_BIT;
    void (*func) (void);
  } unwind_void;
  struct
  {
    ENUM_BF (specbind_tag) kind : CHAR_BIT;
    Lisp_Object symbol, old_value;
    union
    {
      KBOARD *kbd;
      Lisp_Object buf;
    } where;
  } let;
  struct
  {
    ENUM_BF (specbind_tag) kind : CHAR_BIT;
    bool_bf debug_on_exit : 1;
    Lisp_Object function;
    Lisp_Object *args;
    ptrdiff_t nargs;
  } bt;
};

typedef struct
{
  ptrdiff_t bytes;
} specpdl_ref;

INLINE specpdl_ref
wrap_specpdl_ref (ptrdiff_t bytes)
{
  return (specpdl_ref) { .bytes = bytes };
}
INLINE ptrdiff_t
unwrap_specpdl_ref (specpdl_ref ref)
{
  return ref.bytes;
}
INLINE bool
specpdl_ref_lt (specpdl_ref a, specpdl_ref b)
{
  return unwrap_specpdl_ref (a) < unwrap_specpdl_ref (b);
}
INLINE union specbinding *
specpdl_ref_to_ptr (specpdl_ref ref)
{
  return (union specbinding *) ((char *) specpdl + unwrap_specpdl_ref (ref));
}
INLINE specpdl_ref
SPECPDL_INDEX (void)
{
  return wrap_specpdl_ref ((char *) specpdl_ptr - (char *) specpdl);
}

enum handlertype
{
  CATCHER,
  CONDITION_CASE,
  CATCHER_ALL,
  HANDLER_BIND,
  SKIP_CONDITIONS
};
enum nonlocal_exit
{
  NONLOCAL_EXIT_SIGNAL,
  NONLOCAL_EXIT_THROW,
};
struct handler
{
  enum handlertype type;
  Lisp_Object tag_or_ch;
  enum nonlocal_exit nonlocal_exit;
  Lisp_Object val;
  struct handler *next;
  struct handler *nextfree;
  Lisp_Object *bytecode_top;
  int bytecode_dest;
  sys_jmp_buf jmp;
  EMACS_INT f_lisp_eval_depth;
  specpdl_ref pdlcount;
  struct bc_frame *act_rec;
  int poll_suppress_count;
  int interrupt_input_blocked;
};

extern Lisp_Object unbind_to (specpdl_ref, Lisp_Object);

extern void record_unwind_protect_array (Lisp_Object *, ptrdiff_t);
extern void record_unwind_protect_intmax (void (*) (intmax_t), intmax_t);
extern void record_unwind_protect_int (void (*function) (int), int arg);
specpdl_ref record_in_backtrace (Lisp_Object function, Lisp_Object *args,
                                 ptrdiff_t nargs);
extern void set_unwind_protect_ptr (specpdl_ref count, void (*func) (void *),
                                    void *arg);
#define SAFE_ALLOCA_LISP(buf, nelt) SAFE_ALLOCA_LISP_EXTRA (buf, nelt, 0)
enum MAX_ALLOCA
{
  MAX_ALLOCA = 16 * 1024
};
#define AVAIL_ALLOCA(size) (sa_avail -= (size), alloca (size))
#define USE_SAFE_ALLOCA                \
  unsigned long sa_avail = MAX_ALLOCA; \
  specpdl_ref sa_count = SPECPDL_INDEX ()
#define SAFE_ALLOCA_LISP_EXTRA(buf, nelt, extra)            \
  do                                                        \
    {                                                       \
      unsigned long alloca_nbytes;                          \
      if (ckd_mul (&alloca_nbytes, nelt, word_size)         \
          || ckd_add (&alloca_nbytes, alloca_nbytes, extra) \
          || SIZE_MAX < alloca_nbytes)                      \
        memory_full (SIZE_MAX);                             \
      else if (alloca_nbytes <= sa_avail)                   \
        (buf) = AVAIL_ALLOCA (alloca_nbytes);               \
      else                                                  \
        {                                                   \
          (buf) = xzalloc (alloca_nbytes);                  \
          record_unwind_protect_array (buf, nelt);          \
        }                                                   \
    }                                                       \
  while (false)
#define SAFE_ALLOCA(size) \
  ((long) (size) <= (long) sa_avail ? AVAIL_ALLOCA (size) : (TODO, NULL))
#define SAFE_ALLOCA_STRING(ptr, string)                  \
  do                                                     \
    {                                                    \
      (ptr) = SAFE_ALLOCA (SBYTES (string) + 1);         \
      memcpy (ptr, SDATA (string), SBYTES (string) + 1); \
    }                                                    \
  while (false)
#define SAFE_FREE() safe_free (sa_count)
extern void xfree (void *);
INLINE void
safe_free (specpdl_ref sa_count)
{
  while (specpdl_ptr != specpdl_ref_to_ptr (sa_count))
    {
      specpdl_ptr--;
      if (specpdl_ptr->kind == SPECPDL_UNWIND_PTR)
        {
          eassert (specpdl_ptr->unwind_ptr.func == xfree);
          xfree (specpdl_ptr->unwind_ptr.arg);
        }
      else
        {
          eassert (specpdl_ptr->kind == SPECPDL_UNWIND_ARRAY);
          xfree (specpdl_ptr->unwind_array.array);
        }
    }
}
#define SAFE_FREE_UNBIND_TO(count, val) \
  safe_free_unbind_to (count, sa_count, val)
INLINE Lisp_Object
safe_free_unbind_to (specpdl_ref count, specpdl_ref sa_count, Lisp_Object val)
{
  eassert (!specpdl_ref_lt (sa_count, count));
  return unbind_to (count, val);
}
extern Lisp_Object list1 (Lisp_Object);
extern Lisp_Object list2 (Lisp_Object, Lisp_Object);
extern Lisp_Object list3 (Lisp_Object, Lisp_Object, Lisp_Object);
extern Lisp_Object list4 (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);
extern Lisp_Object list5 (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object,
                          Lisp_Object);

#define USE_STACK_LISP_OBJECTS true
enum
{
  USE_STACK_CONS = USE_STACK_LISP_OBJECTS,
  USE_STACK_STRING = USE_STACK_CONS
};
#define STACK_CONS(a, b) \
  make_lisp_ptr (&((struct Lisp_Cons) { { { a, { b } } } }), Lisp_Cons)
#define AUTO_LIST1(name, a) \
  Lisp_Object name = (USE_STACK_CONS ? STACK_CONS (a, Qnil) : list1 (a))
#define AUTO_STRING_WITH_LEN(name, str, len)                                 \
  Lisp_Object name                                                           \
    = (USE_STACK_STRING                                                      \
         ? (make_lisp_ptr ((&(struct Lisp_String) {                          \
                             { { len, -1, 0, (unsigned char *) (str) } } }), \
                           Lisp_String))                                     \
         : make_unibyte_string (str, len))
#define AUTO_STRING(name, str) AUTO_STRING_WITH_LEN (name, str, strlen (str))

#if TODO_NELISP_LATER_ELSE
INLINE void
maybe_quit (void)
{
}
INLINE void
rarely_quit (unsigned short int count)
{
  UNUSED (count);
}
#endif
enum
{
  SMALL_LIST_LEN_MAX = 127
};
struct for_each_tail_internal
{
  Lisp_Object tortoise;
  intptr_t max, n;
  unsigned short int q;
};
#define FOR_EACH_TAIL_INTERNAL(tail, cycle, check_quit)                       \
  for (struct for_each_tail_internal li = { tail, 2, 0, 2 }; CONSP (tail);    \
       ((tail) = XCDR (tail),                                                 \
       ((--li.q != 0 || ((check_quit) ? maybe_quit () : (void) 0, 0 < --li.n) \
         || (li.q = li.n = li.max <<= 1, li.n >>= USHRT_WIDTH,                \
            li.tortoise = (tail), false))                                     \
        && BASE_EQ (tail, li.tortoise))                                       \
          ? (cycle)                                                           \
          : (void) 0))
#define FOR_EACH_TAIL(tail) \
  FOR_EACH_TAIL_INTERNAL (tail, circular_list (tail), true)
#define FOR_EACH_TAIL_SAFE(tail) \
  FOR_EACH_TAIL_INTERNAL (tail, (void) ((tail) = Qnil), false)
enum Set_Internal_Bind
{
  SET_INTERNAL_SET,
  SET_INTERNAL_BIND,
  SET_INTERNAL_UNBIND,
  SET_INTERNAL_THREAD_SWITCH,
};

extern Lisp_Object make_float (double);
extern void syms_of_alloc (void);
extern Lisp_Object make_unibyte_string (const char *, ptrdiff_t);
extern void garbage_collect (void);
extern void init_alloc_once (void);
extern void *xmalloc (size_t);
extern void *xzalloc (size_t);
extern void *xrealloc (void *, size_t);
extern void xfree (void *);
extern void *xnmalloc (ptrdiff_t, ptrdiff_t);
extern void *xnrealloc (void *, ptrdiff_t, ptrdiff_t);
extern void *xpalloc (void *, ptrdiff_t *, ptrdiff_t, ptrdiff_t, ptrdiff_t);
extern AVOID memory_full (size_t);
void *hash_table_alloc_bytes (ptrdiff_t nbytes);
void hash_table_free_bytes (void *p, ptrdiff_t nbytes);
extern struct Lisp_Vector *allocate_pseudovector (int, int, int,
                                                  enum pvec_type);
extern struct buffer *allocate_buffer (void);
#define ALLOCATE_PLAIN_PSEUDOVECTOR(type, tag) \
  ((type *) allocate_pseudovector (VECSIZE (type), 0, 0, tag))
extern Lisp_Object make_pure_c_string (const char *, ptrdiff_t);
INLINE Lisp_Object
build_pure_c_string (const char *str)
{
  return make_pure_c_string (str, strlen (str));
}
extern void init_symbol (Lisp_Object, Lisp_Object);
void staticpro (Lisp_Object const *);
extern Lisp_Object make_specified_string (const char *, ptrdiff_t, ptrdiff_t,
                                          bool);
extern void mark_object (Lisp_Object);
extern void mark_objects (Lisp_Object *, ptrdiff_t);
extern Lisp_Object make_string (const char *, ptrdiff_t);
INLINE Lisp_Object
build_string (const char *str)
{
  return make_string (str, strlen (str));
}
#define ALLOCATE_PSEUDOVECTOR(type, field, tag)                 \
  ((type *) allocate_pseudovector (VECSIZE (type),              \
                                   PSEUDOVECSIZE (type, field), \
                                   PSEUDOVECSIZE (type, field), tag))
extern Lisp_Object empty_unibyte_string, empty_multibyte_string;
void free_cons (struct Lisp_Cons *ptr);
extern struct Lisp_Vector *allocate_nil_vector (ptrdiff_t);
INLINE Lisp_Object
make_nil_vector (ptrdiff_t size)
{
  return make_lisp_ptr (allocate_nil_vector (size), Lisp_Vectorlike);
}
void pin_string (Lisp_Object string);
extern struct Lisp_Vector *allocate_vector (ptrdiff_t);
INLINE Lisp_Object
make_uninit_vector (ptrdiff_t size)
{
  return make_lisp_ptr (allocate_vector (size), Lisp_Vectorlike);
}
INLINE void
maybe_gc (void)
{
  TODO_NELISP_LATER;
}

extern ptrdiff_t read_from_string_index;
extern ptrdiff_t read_from_string_index_byte;
extern ptrdiff_t read_from_string_limit;
#define READCHAR readchar (readcharfun, NULL)
#define UNREAD(c) unreadchar (readcharfun, c)
extern int readchar (Lisp_Object readcharfun, bool *multibyte);
void unreadchar (Lisp_Object readcharfun, int c);
Lisp_Object read0 (Lisp_Object readcharfun, bool locate_syms);
extern void init_obarray_once (void);
extern void syms_of_lread (void);
void *stack_top = NULL;
typedef enum
{
  Cookie_None,
  Cookie_Dyn,
  Cookie_Lex
} lexical_cookie_t;
extern lexical_cookie_t lisp_file_lexical_cookie (Lisp_Object readcharfun);

extern ptrdiff_t string_char_to_byte (Lisp_Object, ptrdiff_t);
extern void syms_of_fns (void);
extern ptrdiff_t list_length (Lisp_Object);
EMACS_UINT hash_string (char const *, ptrdiff_t);

INLINE AVOID
xsignal (Lisp_Object error_symbol, Lisp_Object data)
{
  Fsignal (error_symbol, data);
}
extern AVOID xsignal0 (Lisp_Object);
extern AVOID xsignal1 (Lisp_Object, Lisp_Object);
extern AVOID xsignal2 (Lisp_Object, Lisp_Object, Lisp_Object);
extern AVOID xsignal3 (Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);
extern AVOID signal_error (const char *, Lisp_Object);
extern AVOID error (const char *, ...) ATTRIBUTE_FORMAT_PRINTF (1, 2);
extern Lisp_Object eval_sub (Lisp_Object form);
extern void init_eval_once (void);
extern void init_eval (void);
extern void syms_of_eval (void);
extern Lisp_Object internal_catch (Lisp_Object, Lisp_Object (*) (Lisp_Object),
                                   Lisp_Object);
extern Lisp_Object internal_condition_case (Lisp_Object (*) (void), Lisp_Object,
                                            Lisp_Object (*) (Lisp_Object));
extern void specbind (Lisp_Object symbol, Lisp_Object value);
Lisp_Object funcall_general (Lisp_Object fun, ptrdiff_t numargs,
                             Lisp_Object *args);
extern Lisp_Object funcall_subr (struct Lisp_Subr *subr, ptrdiff_t numargs,
                                 Lisp_Object *arg_vector);

enum Arith_Comparison
{
  ARITH_EQUAL,
  ARITH_NOTEQUAL,
  ARITH_LESS,
  ARITH_GRTR,
  ARITH_LESS_OR_EQUAL,
  ARITH_GRTR_OR_EQUAL
};
extern void syms_of_data (void);
extern void set_internal (Lisp_Object symbol, Lisp_Object newval,
                          Lisp_Object where, enum Set_Internal_Bind bindflag);
extern Lisp_Object find_symbol_value (Lisp_Object symbol);

extern void syms_of_keyboard (void);
extern void init_keyboard (void);

extern void syms_of_editfns (void);

extern void syms_of_emacs (void);

extern void syms_of_fileio (void);

extern void syms_of_coding (void);

extern void syms_of_buffer (void);
extern void init_buffer (void);

extern int emacs_close (int fd);
extern int emacs_open (char const *file, int oflags, int mode);
extern FILE *emacs_fdopen (int fd, const char *mode);

extern void syms_of_bytecode (void);
void init_bytecode (void);

INLINE void
circular_list (Lisp_Object list)
{
  xsignal1 (Qcircular_list, list);
}

#define DEFUN(lname, fnname, sname, minargs, maxargs, intspec, doc) \
  static union Aligned_Lisp_Subr sname                              \
    = { { { PVEC_SUBR << PSEUDOVECTOR_AREA_BITS },                  \
          { .a##maxargs = fnname },                                 \
          minargs,                                                  \
          maxargs,                                                  \
          lname,                                                    \
          { intspec },                                              \
          lisp_h_Qnil,                                              \
          0 } };                                                    \
  DEFUN_LUA_N (fnname, maxargs)                                     \
  Lisp_Object fnname

struct Lisp_Objfwd
{
  enum Lisp_Fwd_Type type;
  Lisp_Object *objvar;
};
extern void defvar_lisp (struct Lisp_Objfwd const *, char const *);
#define DEFVAR_LISP(lname, vname, doc)          \
  do                                            \
    {                                           \
      static struct Lisp_Objfwd const o_fwd     \
        = { Lisp_Fwd_Obj, &globals.f_##vname }; \
      defvar_lisp (&o_fwd, lname);              \
    }                                           \
  while (false)
struct Lisp_Intfwd
{
  enum Lisp_Fwd_Type type;
  intmax_t *intvar;
};
extern void defvar_int (struct Lisp_Intfwd const *, char const *);
#define DEFVAR_INT(lname, vname, doc)           \
  do                                            \
    {                                           \
      static struct Lisp_Intfwd const i_fwd     \
        = { Lisp_Fwd_Int, &globals.f_##vname }; \
      defvar_int (&i_fwd, lname);               \
    }                                           \
  while (false)
struct Lisp_Boolfwd
{
  enum Lisp_Fwd_Type type; /* = Lisp_Fwd_Bool */
  bool *boolvar;
};
#define DEFSYM(sym, name)
extern void defsubr (union Aligned_Lisp_Subr *);

/* --- lua -- */
#ifdef ENABLE_CHECKING
# error "TODO"
#else
# define set_obj_check(L, idx)
# define check_obj(L, idx)
#endif

#define pub __attribute__ ((visibility ("default")))
#define ret(...)

extern thrd_t main_thread;
extern mtx_t main_mutex;
extern mtx_t thread_mutex;
extern cnd_t main_cond;
extern cnd_t thread_cond;
extern bool tcall_error;
extern void (*main_func) (void);
bool in_thread;
extern Lisp_Object userdata_to_obj (lua_State *L, int idx);
extern void push_obj (lua_State *L, Lisp_Object obj);
extern void check_nargs (lua_State *L, int nargs);
extern void check_isobject (lua_State *L, int n);
extern void check_istable_with_obj (lua_State *L, int n);
extern void tcall (lua_State *L, void (*f) (lua_State *L), int change);
extern void _lcheckstack (lua_State *L, int n);

#define DEF_TCALL_ARGS_PRE_0
#define DEF_TCALL_ARGS_PRE_1
#define DEF_TCALL_ARGS_PRE_2
#define DEF_TCALL_ARGS_PRE_3
#define DEF_TCALL_ARGS_PRE_4
#define DEF_TCALL_ARGS_PRE_5
#define DEF_TCALL_ARGS_PRE_6
#define DEF_TCALL_ARGS_PRE_7
#define DEF_TCALL_ARGS_PRE_8
#define DEF_TCALL_ARGS_PRE_UNEVALLED
#define DEF_TCALL_ARGS_PRE_MANY          \
  _lcheckstack (L, 5);                   \
  ptrdiff_t len = lua_objlen (L, 1);     \
  Lisp_Object args[len];                 \
  for (ptrdiff_t i = 0; i < len; i++)    \
    {                                    \
      lua_rawgeti (L, 1, i + 1);         \
      args[i] = userdata_to_obj (L, -1); \
      lua_pop (L, 1);                    \
    }
#define DEF_TCALL_ARGS_0(fname)
#define DEF_TCALL_ARGS_1(fname) userdata_to_obj (L, 1)
#define DEF_TCALL_ARGS_2(fname) DEF_TCALL_ARGS_1 (fname), userdata_to_obj (L, 2)
#define DEF_TCALL_ARGS_3(fname) DEF_TCALL_ARGS_2 (fname), userdata_to_obj (L, 3)
#define DEF_TCALL_ARGS_4(fname) DEF_TCALL_ARGS_3 (fname), userdata_to_obj (L, 4)
#define DEF_TCALL_ARGS_5(fname) DEF_TCALL_ARGS_4 (fname), userdata_to_obj (L, 5)
#define DEF_TCALL_ARGS_6(fname) DEF_TCALL_ARGS_5 (fname), userdata_to_obj (L, 6)
#define DEF_TCALL_ARGS_7(fname) DEF_TCALL_ARGS_6 (fname), userdata_to_obj (L, 7)
#define DEF_TCALL_ARGS_8(fname) DEF_TCALL_ARGS_7 (fname), userdata_to_obj (L, 8)
#define DEF_TCALL_ARGS_UNEVALLED(fname) userdata_to_obj (L, 1)
#define DEF_TCALL_ARGS_MANY(fname) len, args

#define DEFUN_LUA_N(fname, maxargs)                                    \
  void t_l##fname (lua_State *L)                                       \
  {                                                                    \
    DEF_TCALL_ARGS_PRE_##maxargs;                                      \
    Lisp_Object obj = fname (DEF_TCALL_ARGS_##maxargs (maxargs));      \
    push_obj (L, obj);                                                 \
  }                                                                    \
  int __attribute__ ((visibility ("default"))) l##fname (lua_State *L) \
  {                                                                    \
    if (maxargs >= 0)                                                  \
      check_nargs (L, maxargs);                                        \
    else                                                               \
      check_nargs (L, 1);                                              \
    static int i;                                                      \
    if (maxargs == UNEVALLED)                                          \
      check_isobject (L, 1);                                           \
    else if (maxargs == MANY)                                          \
      {                                                                \
        check_istable_with_obj (L, 1);                                 \
      }                                                                \
    else                                                               \
      for (i = 1; i <= maxargs; i++)                                   \
        {                                                              \
          check_isobject (L, i);                                       \
        }                                                              \
    tcall (L, t_l##fname, 1);                                          \
    return 1;                                                          \
  }

static inline _Noreturn void
TODO_ (const char *file, int line)
{
  _lcheckstack (_global_lua_state, 5);
  lua_pushfstring (_global_lua_state, "TODO at %s:%d", file, line);
  unrecoverable_error = true;
  if (!in_thread)
    {
      lua_error (_global_lua_state);
    }
  tcall_error = true;
  mtx_lock (&main_mutex);
  cnd_signal (&main_cond);
  mtx_unlock (&main_mutex);
  cnd_wait (&thread_cond, &thread_mutex);
  __builtin_unreachable ();
}

static inline _Noreturn void
TODO_msg (const char *file, int line, const char *msg)
{
  _lcheckstack (_global_lua_state, 5);
  lua_pushfstring (_global_lua_state, "TODO at %s:%d: %s", file, line, msg);
  unrecoverable_error = true;
  if (!in_thread)
    {
      lua_error (_global_lua_state);
    }
  tcall_error = true;
  mtx_lock (&main_mutex);
  cnd_signal (&main_cond);
  mtx_unlock (&main_mutex);
  cnd_wait (&thread_cond, &thread_mutex);
  __builtin_unreachable ();
}
#endif
