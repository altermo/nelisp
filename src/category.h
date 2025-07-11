#ifndef EMACS_CATEGORY_H
#define EMACS_CATEGORY_H

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
