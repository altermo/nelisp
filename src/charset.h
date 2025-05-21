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

#define CHARSET_ATTRIBUTES(charset) (charset)->attributes

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

#define CHARSET_SYMBOL_HASH_INDEX(symbol) \
  hash_lookup (XHASH_TABLE (Vcharset_hash_table), symbol)

#define CHARSET_ID(charset) ((charset)->id)
#define CHARSET_DIMENSION(charset) ((charset)->dimension)
#define CHARSET_CODE_SPACE(charset) ((charset)->code_space)
#define CHARSET_CODE_LINEAR_P(charset) ((charset)->code_linear_p)
#define CHARSET_ISO_CHARS_96(charset) ((charset)->iso_chars_96)
#define CHARSET_ISO_FINAL(charset) ((charset)->iso_final)
#define CHARSET_ISO_PLANE(charset) ((charset)->iso_plane)
#define CHARSET_ISO_REVISION(charset) ((charset)->iso_revision)
#define CHARSET_EMACS_MULE_ID(charset) ((charset)->emacs_mule_id)
#define CHARSET_ASCII_COMPATIBLE_P(charset) ((charset)->ascii_compatible_p)
#define CHARSET_COMPACT_CODES_P(charset) ((charset)->compact_codes_p)
#define CHARSET_METHOD(charset) ((charset)->method)
#define CHARSET_MIN_CODE(charset) ((charset)->min_code)
#define CHARSET_MAX_CODE(charset) ((charset)->max_code)
#define CHARSET_INVALID_CODE(charset) ((charset)->invalid_code)
#define CHARSET_MIN_CHAR(charset) ((charset)->min_char)
#define CHARSET_MAX_CHAR(charset) ((charset)->max_char)
#define CHARSET_CODE_OFFSET(charset) ((charset)->code_offset)
#define CHARSET_UNIFIED_P(charset) ((charset)->unified_p)

#define CHARSET_NAME(charset) CHARSET_ATTR_NAME (CHARSET_ATTRIBUTES (charset))
#define CHARSET_MAP(charset) CHARSET_ATTR_MAP (CHARSET_ATTRIBUTES (charset))
#define CHARSET_DECODER(charset) \
  CHARSET_ATTR_DECODER (CHARSET_ATTRIBUTES (charset))
#define CHARSET_ENCODER(charset) \
  CHARSET_ATTR_ENCODER (CHARSET_ATTRIBUTES (charset))
#define CHARSET_SUBSET(charset) \
  CHARSET_ATTR_SUBSET (CHARSET_ATTRIBUTES (charset))
#define CHARSET_SUPERSET(charset) \
  CHARSET_ATTR_SUPERSET (CHARSET_ATTRIBUTES (charset))
#define CHARSET_UNIFY_MAP(charset) \
  CHARSET_ATTR_UNIFY_MAP (CHARSET_ATTRIBUTES (charset))
#define CHARSET_DEUNIFIER(charset) \
  CHARSET_ATTR_DEUNIFIER (CHARSET_ATTRIBUTES (charset))

INLINE void
set_charset_attr (struct charset *charset, enum charset_attr_index idx,
                  Lisp_Object val)
{
  ASET (CHARSET_ATTRIBUTES (charset), idx, val);
}

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

#define CHECK_CHARSET_GET_ID(x, id)                                            \
  do                                                                           \
    {                                                                          \
      ptrdiff_t idx;                                                           \
                                                                               \
      if (!SYMBOLP (x) || (idx = CHARSET_SYMBOL_HASH_INDEX (x)) < 0)           \
        wrong_type_argument (Qcharsetp, x);                                    \
      id = XFIXNUM (AREF (HASH_VALUE (XHASH_TABLE (Vcharset_hash_table), idx), \
                          charset_id));                                        \
    }                                                                          \
  while (false)

#define CHECK_CHARSET_GET_ATTR(x, attr)                                \
  do                                                                   \
    {                                                                  \
      if (!SYMBOLP (x) || NILP (attr = CHARSET_SYMBOL_ATTRIBUTES (x))) \
        wrong_type_argument (Qcharsetp, x);                            \
    }                                                                  \
  while (false)

#define CHECK_CHARSET_GET_CHARSET(x, charset) \
  do                                          \
    {                                         \
      int csid;                               \
      CHECK_CHARSET_GET_ID (x, csid);         \
      charset = CHARSET_FROM_ID (csid);       \
    }                                         \
  while (false)

#define DECODE_CHAR(charset, code)                                            \
  ((ASCII_CHAR_P (code) && (charset)->ascii_compatible_p)           ? (code)  \
   : ((code) < (charset)->min_code || (code) > (charset)->max_code) ? -1      \
   : (charset)->unified_p ? decode_char (charset, code)                       \
   : (charset)->method == CHARSET_METHOD_OFFSET                               \
     ? ((charset)->code_linear_p                                              \
          ? (int) ((code) - (charset)->min_code) + (charset)->code_offset     \
          : decode_char (charset, code))                                      \
   : (charset)->method == CHARSET_METHOD_MAP                                  \
     ? (((charset)->code_linear_p && VECTORP (CHARSET_DECODER (charset)))     \
          ? XFIXNUM (                                                         \
              AREF (CHARSET_DECODER (charset), (code) - (charset)->min_code)) \
          : decode_char (charset, code))                                      \
     : decode_char (charset, code))

#define ISO_MAX_DIMENSION 3
#define ISO_MAX_CHARS 2
#define ISO_MAX_FINAL 0x80
extern int iso_charset_table[ISO_MAX_DIMENSION][ISO_MAX_CHARS][ISO_MAX_FINAL];

#define ISO_CHARSET_TABLE(dimension, chars_96, final) \
  iso_charset_table[(dimension) - 1][chars_96][final]

extern int charset_unibyte;

#endif
