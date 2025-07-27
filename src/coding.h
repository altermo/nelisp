#ifndef EMACS_CODING_H
#define EMACS_CODING_H

#include "lisp.h"

enum define_coding_system_arg_index
{
  coding_arg_name,
  coding_arg_mnemonic,
  coding_arg_coding_type,
  coding_arg_charset_list,
  coding_arg_ascii_compatible_p,
  coding_arg_decode_translation_table,
  coding_arg_encode_translation_table,
  coding_arg_post_read_conversion,
  coding_arg_pre_write_conversion,
  coding_arg_default_char,
  coding_arg_for_unibyte,
  coding_arg_plist,
  coding_arg_eol_type,
  coding_arg_max
};

enum define_coding_iso2022_arg_index
{
  coding_arg_iso2022_initial = coding_arg_max,
  coding_arg_iso2022_reg_usage,
  coding_arg_iso2022_request,
  coding_arg_iso2022_flags,
  coding_arg_iso2022_max
};

enum define_coding_utf8_arg_index
{
  coding_arg_utf8_bom = coding_arg_max,
  coding_arg_utf8_max
};

enum define_coding_utf16_arg_index
{
  coding_arg_utf16_bom = coding_arg_max,
  coding_arg_utf16_endian,
  coding_arg_utf16_max
};

enum define_coding_ccl_arg_index
{
  coding_arg_ccl_decoder = coding_arg_max,
  coding_arg_ccl_encoder,
  coding_arg_ccl_valids,
  coding_arg_ccl_max
};

enum define_coding_undecided_arg_index
{
  coding_arg_undecided_inhibit_null_byte_detection = coding_arg_max,
  coding_arg_undecided_inhibit_iso_escape_detection,
  coding_arg_undecided_prefer_utf_8,
  coding_arg_undecided_max
};

enum coding_attr_index
{
  coding_attr_base_name,
  coding_attr_docstring,
  coding_attr_mnemonic,
  coding_attr_type,
  coding_attr_charset_list,
  coding_attr_ascii_compat,
  coding_attr_decode_tbl,
  coding_attr_encode_tbl,
  coding_attr_trans_tbl,
  coding_attr_post_read,
  coding_attr_pre_write,
  coding_attr_default_char,
  coding_attr_for_unibyte,
  coding_attr_plist,

  coding_attr_category,
  coding_attr_safe_charsets,

  coding_attr_charset_valids,

  coding_attr_ccl_decoder,
  coding_attr_ccl_encoder,
  coding_attr_ccl_valids,

  coding_attr_iso_initial,
  coding_attr_iso_usage,
  coding_attr_iso_request,
  coding_attr_iso_flags,

  coding_attr_utf_bom,
  coding_attr_utf_16_endian,

  coding_attr_emacs_mule_full,

  coding_attr_undecided_inhibit_null_byte_detection,
  coding_attr_undecided_inhibit_iso_escape_detection,
  coding_attr_undecided_prefer_utf_8,

  coding_attr_last_index
};

#define CODING_ATTR_BASE_NAME(attrs) AREF (attrs, coding_attr_base_name)
#define CODING_ATTR_TYPE(attrs) AREF (attrs, coding_attr_type)
#define CODING_ATTR_CHARSET_LIST(attrs) AREF (attrs, coding_attr_charset_list)
#define CODING_ATTR_MNEMONIC(attrs) AREF (attrs, coding_attr_mnemonic)
#define CODING_ATTR_DOCSTRING(attrs) AREF (attrs, coding_attr_docstring)
#define CODING_ATTR_ASCII_COMPAT(attrs) AREF (attrs, coding_attr_ascii_compat)
#define CODING_ATTR_DECODE_TBL(attrs) AREF (attrs, coding_attr_decode_tbl)
#define CODING_ATTR_ENCODE_TBL(attrs) AREF (attrs, coding_attr_encode_tbl)
#define CODING_ATTR_TRANS_TBL(attrs) AREF (attrs, coding_attr_trans_tbl)
#define CODING_ATTR_POST_READ(attrs) AREF (attrs, coding_attr_post_read)
#define CODING_ATTR_PRE_WRITE(attrs) AREF (attrs, coding_attr_pre_write)
#define CODING_ATTR_DEFAULT_CHAR(attrs) AREF (attrs, coding_attr_default_char)
#define CODING_ATTR_FOR_UNIBYTE(attrs) AREF (attrs, coding_attr_for_unibyte)
#define CODING_ATTR_PLIST(attrs) AREF (attrs, coding_attr_plist)
#define CODING_ATTR_CATEGORY(attrs) AREF (attrs, coding_attr_category)
#define CODING_ATTR_SAFE_CHARSETS(attrs) AREF (attrs, coding_attr_safe_charsets)

#define CODING_ID_NAME(id) \
  HASH_KEY (XHASH_TABLE (Vcoding_system_hash_table), id)

#define CODING_ID_ATTRS(id) \
  AREF (HASH_VALUE (XHASH_TABLE (Vcoding_system_hash_table), id), 0)

#define CODING_ID_EOL_TYPE(id) \
  AREF (HASH_VALUE (XHASH_TABLE (Vcoding_system_hash_table), id), 2)

#define CODING_SYSTEM_SPEC(coding_system_symbol) \
  Fgethash (coding_system_symbol, Vcoding_system_hash_table, Qnil)

#define CODING_SYSTEM_ID(coding_system_symbol) \
  hash_lookup (XHASH_TABLE (Vcoding_system_hash_table), coding_system_symbol)

#define CODING_SYSTEM_P(coding_system_symbol)   \
  (CODING_SYSTEM_ID (coding_system_symbol) >= 0 \
   || (!NILP (coding_system_symbol)             \
       && !NILP (Fcoding_system_p (coding_system_symbol))))

#define CHECK_CODING_SYSTEM(x)                                         \
  do                                                                   \
    {                                                                  \
      if (CODING_SYSTEM_ID (x) < 0 && NILP (Fcheck_coding_system (x))) \
        wrong_type_argument (Qcoding_system_p, x);                     \
    }                                                                  \
  while (false)

#define CHECK_CODING_SYSTEM_GET_SPEC(x, spec)      \
  do                                               \
    {                                              \
      spec = CODING_SYSTEM_SPEC (x);               \
      if (NILP (spec))                             \
        {                                          \
          Fcheck_coding_system (x);                \
          spec = CODING_SYSTEM_SPEC (x);           \
        }                                          \
      if (NILP (spec))                             \
        wrong_type_argument (Qcoding_system_p, x); \
    }                                              \
  while (false)

#define CHECK_CODING_SYSTEM_GET_ID(x, id)          \
  do                                               \
    {                                              \
      id = CODING_SYSTEM_ID (x);                   \
      if (id < 0)                                  \
        {                                          \
          Fcheck_coding_system (x);                \
          id = CODING_SYSTEM_ID (x);               \
        }                                          \
      if (id < 0)                                  \
        wrong_type_argument (Qcoding_system_p, x); \
    }                                              \
  while (false)

#include "composite.h"

enum composition_state
{
  COMPOSING_NO,
  COMPOSING_CHAR,
  COMPOSING_RULE,
  COMPOSING_COMPONENT_CHAR,
  COMPOSING_COMPONENT_RULE
};

struct composition_status
{
  enum composition_state state;
  enum composition_method method;
  bool old_form;
  int length;
  int nchars;
  int ncomps;
  int carryover[4 + MAX_COMPOSITION_COMPONENTS * 3 - 2 + 2
                + MAX_COMPOSITION_COMPONENTS];
};

enum coding_category
{
  coding_category_iso_7,
  coding_category_iso_7_tight,
  coding_category_iso_8_1,
  coding_category_iso_8_2,
  coding_category_iso_7_else,
  coding_category_iso_8_else,
  coding_category_utf_8_auto,
  coding_category_utf_8_nosig,
  coding_category_utf_8_sig,
  coding_category_utf_16_auto,
  coding_category_utf_16_be,
  coding_category_utf_16_le,
  coding_category_utf_16_be_nosig,
  coding_category_utf_16_le_nosig,
  coding_category_charset,
  coding_category_sjis,
  coding_category_big5,
  coding_category_ccl,
  coding_category_emacs_mule,
  coding_category_raw_text,
  coding_category_undecided,
  coding_category_max
};

enum coding_result_code
{
  CODING_RESULT_SUCCESS,
  CODING_RESULT_INSUFFICIENT_SRC,
  CODING_RESULT_INSUFFICIENT_DST,
  CODING_RESULT_INVALID_SRC,
  CODING_RESULT_INTERRUPT
};

#define CODING_MODE_LAST_BLOCK 0x01

struct iso_2022_spec
{
  unsigned flags;

  int current_invocation[2];

  int current_designation[4];

  int ctext_extended_segment_len;

  /* True temporarily only when graphic register 2 or 3 is invoked by
     single-shift while encoding.  */
  bool_bf single_shifting : 1;

  /* True temporarily only when processing at beginning of line.  */
  bool_bf bol : 1;

  /* If true, we are now scanning embedded UTF-8 sequence.  */
  bool_bf embedded_utf_8 : 1;

  /* The current composition.  */
  struct composition_status cmp_status;
};

struct emacs_mule_spec
{
  struct composition_status cmp_status;
};

struct undecided_spec
{
  /* Inhibit null byte detection.  1 means always inhibit,
     -1 means do not inhibit, 0 means rely on user variable.  */
  int inhibit_nbd;

  /* Inhibit ISO escape detection.  -1, 0, 1 as above.  */
  int inhibit_ied;

  /* Prefer UTF-8 when the input could be other encodings.  */
  bool prefer_utf_8;
};

enum utf_bom_type
{
  utf_detect_bom,
  utf_without_bom,
  utf_with_bom
};

enum utf_16_endian_type
{
  utf_16_big_endian,
  utf_16_little_endian
};

struct utf_16_spec
{
  enum utf_bom_type bom;
  enum utf_16_endian_type endian;
  int surrogate;
};

struct coding_detection_info
{
  /* Values of these members are bitwise-OR of CATEGORY_MASK_XXXs.  */
  /* Which categories are already checked.  */
  int checked;
  /* Which categories are strongly found.  */
  int found;
  /* Which categories are rejected.  */
  int rejected;
};

struct coding_system
{
  ptrdiff_t id;

  unsigned common_flags : 14;

  unsigned mode : 5;

  bool_bf src_multibyte : 1;
  bool_bf dst_multibyte : 1;

  bool_bf chars_at_source : 1;

  bool_bf raw_destination : 1;

  bool_bf annotated : 1;

  bool_bf insert_before_markers : 1;

  unsigned eol_seen : 3;

  ENUM_BF (coding_result_code) result : 3;

  int max_charset_id;

  union
  {
    struct iso_2022_spec iso_2022;
    struct ccl_spec *ccl;
    struct utf_16_spec utf_16;
    enum utf_bom_type utf_8_bom;
    struct emacs_mule_spec emacs_mule;
    struct undecided_spec undecided;
  } spec;

  unsigned char *safe_charsets;

  ptrdiff_t head_ascii;

  ptrdiff_t detected_utf8_bytes, detected_utf8_chars;

  ptrdiff_t produced, produced_char, consumed, consumed_char;

  ptrdiff_t src_pos, src_pos_byte, src_chars, src_bytes;
  Lisp_Object src_object;
  const unsigned char *source;

  ptrdiff_t dst_pos, dst_pos_byte, dst_bytes;
  Lisp_Object dst_object;
  unsigned char *destination;

  int *charbuf;
  int charbuf_size, charbuf_used;

  unsigned char carryover[64];
  int carryover_bytes;

  int default_char;

#if TODO_NELISP_LATER_AND
  bool (*detector) (struct coding_system *, struct coding_detection_info *);
  void (*decoder) (struct coding_system *);
  bool (*encoder) (struct coding_system *);
#endif
};

#define CODING_ANNOTATION_MASK 0x00FF
#define CODING_ANNOTATE_COMPOSITION_MASK 0x0001
#define CODING_ANNOTATE_DIRECTION_MASK 0x0002
#define CODING_ANNOTATE_CHARSET_MASK 0x0003
#define CODING_FOR_UNIBYTE_MASK 0x0100
#define CODING_REQUIRE_FLUSHING_MASK 0x0200
#define CODING_REQUIRE_DECODING_MASK 0x0400
#define CODING_REQUIRE_ENCODING_MASK 0x0800
#define CODING_REQUIRE_DETECTION_MASK 0x1000

#define CODING_MODE_SAFE_ENCODING 0x10

INLINE Lisp_Object
encode_file_name (Lisp_Object name)
{
  if (STRING_MULTIBYTE (name))
    TODO;
  CHECK_STRING_NULL_BYTES (name);
  return name;
}

#define ENCODE_FILE(NAME) encode_file_name (NAME)

extern char emacs_mule_bytes[256];

#endif
