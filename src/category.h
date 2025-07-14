#ifndef EMACS_CATEGORY_H
#define EMACS_CATEGORY_H

#include "lisp.h"

#define CATEGORY_SET(c) char_category_set (c)

INLINE bool
CATEGORY_MEMBER (EMACS_INT category, Lisp_Object category_set)
{
  return bool_vector_bitref (category_set, category);
}

INLINE bool
CHAR_HAS_CATEGORY (int ch, int category)
{
  Lisp_Object category_set = CATEGORY_SET (ch);
  return CATEGORY_MEMBER (category, category_set);
}

// #define Vstandard_category_table BVAR (&buffer_defaults, category_table)
#define Vstandard_category_table buffer_defaults.category_table_

#define CATEGORYP(x) RANGED_FIXNUMP (0x20, x, 0x7E)

#define CHECK_CATEGORY(x) CHECK_TYPE (CATEGORYP (x), Qcategoryp, x)

#define MAKE_CATEGORY_SET (Fmake_bool_vector (make_fixnum (128), Qnil))

#define CATEGORY_DOCSTRING(table, category) \
  AREF (Fchar_table_extra_slot (table, make_fixnum (0)), ((category) - ' '))

#define SET_CATEGORY_DOCSTRING(table, category, value)                       \
  ASET (Fchar_table_extra_slot (table, make_fixnum (0)), ((category) - ' '), \
        value)

#endif
