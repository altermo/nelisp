#ifndef EMACS_LISP_H
#define EMACS_LISP_H

#include <limits.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

// LSP being anyoing about them existing and not existing, so just define them here
#ifndef LONG_WIDTH
#define LONG_WIDTH   __LONG_WIDTH__
#define ULONG_WIDTH  __LONG_WIDTH__
#endif


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

#endif /* EMACS_LISP_H */
