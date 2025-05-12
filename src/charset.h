#ifndef EMACS_CHARSET_H
#define EMACS_CHARSET_H

#include "lisp.h"

enum define_charset_arg_index
{
  charset_arg_name,
  charset_arg_dimension,
  charset_arg_code_space,
  charset_arg_min_code,
  charset_arg_max_code,
  charset_arg_iso_final,
  charset_arg_iso_revision,
  charset_arg_emacs_mule_id,
  charset_arg_ascii_compatible_p,
  charset_arg_supplementary_p,
  charset_arg_invalid_code,
  charset_arg_code_offset,
  charset_arg_map,
  charset_arg_subset,
  charset_arg_superset,
  charset_arg_unify_map,
  charset_arg_plist,
  charset_arg_max
};

enum charset_attr_index
{
  charset_id,

  charset_name,

  charset_plist,

  charset_map,

  charset_decoder,

  charset_encoder,

  charset_subset,

  charset_superset,

  charset_unify_map,

  charset_deunifier,

  charset_attr_max
};

enum charset_method
{
  CHARSET_METHOD_OFFSET,

  CHARSET_METHOD_MAP,

  CHARSET_METHOD_SUBSET,

  CHARSET_METHOD_SUPERSET
};

struct charset
{
  int id;

  Lisp_Object attributes;

  int dimension;

  int code_space[15];

  unsigned char *code_space_mask;

  bool_bf code_linear_p : 1;

  bool_bf iso_chars_96 : 1;

  bool_bf ascii_compatible_p : 1;

  bool_bf supplementary_p : 1;

  bool_bf compact_codes_p : 1;

  bool_bf unified_p : 1;

  int iso_final;

  int iso_revision;

  int emacs_mule_id;

  enum charset_method method;

  unsigned min_code, max_code;

  unsigned char_index_offset;

  int min_char, max_char;

  unsigned invalid_code;

  unsigned char fast_map[190];

  int code_offset;
};

#define CHARSET_FROM_ID(id) (charset_table + (id))

#define CHARSET_SYMBOL_ATTRIBUTES(symbol) \
  Fgethash (symbol, Vcharset_hash_table, Qnil)

#define CHARSET_ATTR_ID(attrs) AREF (attrs, charset_id)
#define CHARSET_ATTR_NAME(attrs) AREF (attrs, charset_name)
#define CHARSET_ATTR_PLIST(attrs) AREF (attrs, charset_plist)
#define CHARSET_ATTR_MAP(attrs) AREF (attrs, charset_map)
#define CHARSET_ATTR_DECODER(attrs) AREF (attrs, charset_decoder)
#define CHARSET_ATTR_ENCODER(attrs) AREF (attrs, charset_encoder)
#define CHARSET_ATTR_SUBSET(attrs) AREF (attrs, charset_subset)
#define CHARSET_ATTR_SUPERSET(attrs) AREF (attrs, charset_superset)
#define CHARSET_ATTR_UNIFY_MAP(attrs) AREF (attrs, charset_unify_map)
#define CHARSET_ATTR_DEUNIFIER(attrs) AREF (attrs, charset_deunifier)

#define CHARSET_SYMBOL_ID(symbol) \
  CHARSET_ATTR_ID (CHARSET_SYMBOL_ATTRIBUTES (symbol))

#define CHARSET_FAST_MAP_SET(c, fast_map)                       \
  do                                                            \
    {                                                           \
      if ((c) < 0x10000)                                        \
        (fast_map)[(c) >> 10] |= 1 << (((c) >> 7) & 7);         \
      else                                                      \
        (fast_map)[((c) >> 15) + 62] |= 1 << (((c) >> 12) & 7); \
    }                                                           \
  while (false)

#define CHECK_CHARSET_GET_ATTR(x, attr)                                \
  do                                                                   \
    {                                                                  \
      if (!SYMBOLP (x) || NILP (attr = CHARSET_SYMBOL_ATTRIBUTES (x))) \
        wrong_type_argument (Qcharsetp, x);                            \
    }                                                                  \
  while (false)

#define ISO_MAX_DIMENSION 3
#define ISO_MAX_CHARS 2
#define ISO_MAX_FINAL 0x80
extern int iso_charset_table[ISO_MAX_DIMENSION][ISO_MAX_CHARS][ISO_MAX_FINAL];

#define ISO_CHARSET_TABLE(dimension, chars_96, final) \
  iso_charset_table[(dimension) - 1][chars_96][final]

extern int charset_unibyte;

#endif
