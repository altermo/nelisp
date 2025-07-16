#include "coding.h"
#include "lisp.h"
#include "character.h"
#include "charset.h"

static Lisp_Object Vsjis_coding_system;
static Lisp_Object Vbig5_coding_system;

#define CODING_ISO_INITIAL(coding, reg)                                   \
  XFIXNUM (                                                               \
    AREF (AREF (CODING_ID_ATTRS ((coding)->id), coding_attr_iso_initial), \
          reg))

#define CODING_ISO_FLAGS(coding) ((coding)->spec.iso_2022.flags)
#define CODING_ISO_DESIGNATION(coding, reg) \
  ((coding)->spec.iso_2022.current_designation[reg])
#define CODING_ISO_INVOCATION(coding, plane) \
  ((coding)->spec.iso_2022.current_invocation[plane])
#define CODING_ISO_SINGLE_SHIFTING(coding) \
  ((coding)->spec.iso_2022.single_shifting)
#define CODING_ISO_BOL(coding) ((coding)->spec.iso_2022.bol)
#define CODING_ISO_INVOKED_CHARSET(coding, plane) \
  (CODING_ISO_INVOCATION (coding, plane) < 0      \
     ? -1                                         \
     : CODING_ISO_DESIGNATION (coding, CODING_ISO_INVOCATION (coding, plane)))
#define CODING_ISO_CMP_STATUS(coding) (&(coding)->spec.iso_2022.cmp_status)
#define CODING_ISO_EXTSEGMENT_LEN(coding) \
  ((coding)->spec.iso_2022.ctext_extended_segment_len)
#define CODING_ISO_EMBEDDED_UTF_8(coding) \
  ((coding)->spec.iso_2022.embedded_utf_8)

#define CODING_ISO_FLAG_SEVEN_BITS 0x0008
#define CODING_ISO_FLAG_LOCKING_SHIFT 0x0010
#define CODING_ISO_FLAG_SINGLE_SHIFT 0x0020
#define CODING_ISO_FLAG_DESIGNATION 0x0040
#define CODING_ISO_FLAG_SAFE 0x0800
#define CODING_ISO_FLAG_COMPOSITION 0x2000
#define CODING_ISO_FLAG_FULL_SUPPORT 0x100000

#define CODING_UTF_8_BOM(coding) ((coding)->spec.utf_8_bom)

#define CODING_UTF_16_BOM(coding) ((coding)->spec.utf_16.bom)

#define CODING_UTF_16_ENDIAN(coding) ((coding)->spec.utf_16.endian)

#define CODING_UTF_16_SURROGATE(coding) ((coding)->spec.utf_16.surrogate)

Lisp_Object Vcoding_system_hash_table;

struct coding_system safe_terminal_coding;

static Lisp_Object Vcoding_category_table;

static enum coding_category coding_priorities[coding_category_max];

static struct coding_system coding_categories[coding_category_max];

char emacs_mule_bytes[256];

static int
encode_inhibit_flag (Lisp_Object flag)
{
  return NILP (flag) ? -1 : EQ (flag, Qt);
}

DEFUN ("coding-system-p", Fcoding_system_p, Scoding_system_p, 1, 1, 0,
       doc: /* Return t if OBJECT is nil or a coding-system.
See the documentation of `define-coding-system' for information
about coding-system objects.  */)
(Lisp_Object object)
{
  if (NILP (object) || CODING_SYSTEM_ID (object) >= 0)
    return Qt;
  if (!SYMBOLP (object) || NILP (Fget (object, Qcoding_system_define_form)))
    return Qnil;
  return Qt;
}

DEFUN ("check-coding-system", Fcheck_coding_system, Scheck_coding_system,
       1, 1, 0,
       doc: /* Check validity of CODING-SYSTEM.
If valid, return CODING-SYSTEM, else signal a `coding-system-error' error.
It is valid if it is nil or a symbol defined as a coding system by the
function `define-coding-system'.  */)
(Lisp_Object coding_system)
{
  Lisp_Object define_form;

  define_form = Fget (coding_system, Qcoding_system_define_form);
  if (!NILP (define_form))
    {
      Fput (coding_system, Qcoding_system_define_form, Qnil);
      safe_eval (define_form);
    }
  if (!NILP (Fcoding_system_p (coding_system)))
    return coding_system;
  xsignal1 (Qcoding_system_error, coding_system);
}

static void setup_iso_safe_charsets (Lisp_Object attrs);

void
setup_coding_system (Lisp_Object coding_system, struct coding_system *coding)
{
  Lisp_Object attrs;
  Lisp_Object eol_type;
  Lisp_Object coding_type;
  Lisp_Object val;

  if (NILP (coding_system))
    coding_system = Qundecided;

  CHECK_CODING_SYSTEM_GET_ID (coding_system, coding->id);

  attrs = CODING_ID_ATTRS (coding->id);
  eol_type = inhibit_eol_conversion ? Qunix : CODING_ID_EOL_TYPE (coding->id);

  coding->mode = 0;
  if (VECTORP (eol_type))
    coding->common_flags
      = (CODING_REQUIRE_DECODING_MASK | CODING_REQUIRE_DETECTION_MASK);
  else if (!EQ (eol_type, Qunix))
    coding->common_flags
      = (CODING_REQUIRE_DECODING_MASK | CODING_REQUIRE_ENCODING_MASK);
  else
    coding->common_flags = 0;
  if (!NILP (CODING_ATTR_POST_READ (attrs)))
    coding->common_flags |= CODING_REQUIRE_DECODING_MASK;
  if (!NILP (CODING_ATTR_PRE_WRITE (attrs)))
    coding->common_flags |= CODING_REQUIRE_ENCODING_MASK;
  if (!NILP (CODING_ATTR_FOR_UNIBYTE (attrs)))
    coding->common_flags |= CODING_FOR_UNIBYTE_MASK;

  val = CODING_ATTR_SAFE_CHARSETS (attrs);
  coding->max_charset_id = SCHARS (val) - 1;
  coding->safe_charsets = SDATA (val);
  coding->default_char = XFIXNUM (CODING_ATTR_DEFAULT_CHAR (attrs));
  coding->carryover_bytes = 0;
  coding->raw_destination = 0;
  coding->insert_before_markers = 0;

  coding_type = CODING_ATTR_TYPE (attrs);
  if (EQ (coding_type, Qundecided))
    {
#if TODO_NELISP_LATER_AND
      coding->detector = NULL;
      coding->decoder = decode_coding_raw_text;
      coding->encoder = encode_coding_raw_text;
#endif
      coding->common_flags |= CODING_REQUIRE_DETECTION_MASK;
      coding->spec.undecided.inhibit_nbd = (encode_inhibit_flag (
        AREF (attrs, coding_attr_undecided_inhibit_null_byte_detection)));
      coding->spec.undecided.inhibit_ied = (encode_inhibit_flag (
        AREF (attrs, coding_attr_undecided_inhibit_iso_escape_detection)));
      coding->spec.undecided.prefer_utf_8
        = !NILP (AREF (attrs, coding_attr_undecided_prefer_utf_8));
    }
  else if (EQ (coding_type, Qiso_2022))
    {
      int i;
      int flags = XFIXNUM (AREF (attrs, coding_attr_iso_flags));

      CODING_ISO_INVOCATION (coding, 0) = 0;
      CODING_ISO_INVOCATION (coding, 1)
        = (flags & CODING_ISO_FLAG_SEVEN_BITS ? -1 : 1);
      for (i = 0; i < 4; i++)
        CODING_ISO_DESIGNATION (coding, i) = CODING_ISO_INITIAL (coding, i);
      CODING_ISO_SINGLE_SHIFTING (coding) = 0;
      CODING_ISO_BOL (coding) = 1;
#if TODO_NELISP_LATER_AND
      coding->detector = detect_coding_iso_2022;
      coding->decoder = decode_coding_iso_2022;
      coding->encoder = encode_coding_iso_2022;
#endif
      if (flags & CODING_ISO_FLAG_SAFE)
        coding->mode |= CODING_MODE_SAFE_ENCODING;
      coding->common_flags
        |= (CODING_REQUIRE_DECODING_MASK | CODING_REQUIRE_ENCODING_MASK
            | CODING_REQUIRE_FLUSHING_MASK);
      if (flags & CODING_ISO_FLAG_COMPOSITION)
        coding->common_flags |= CODING_ANNOTATE_COMPOSITION_MASK;
      if (flags & CODING_ISO_FLAG_DESIGNATION)
        coding->common_flags |= CODING_ANNOTATE_CHARSET_MASK;
      if (flags & CODING_ISO_FLAG_FULL_SUPPORT)
        {
          setup_iso_safe_charsets (attrs);
          val = CODING_ATTR_SAFE_CHARSETS (attrs);
          coding->max_charset_id = SCHARS (val) - 1;
          coding->safe_charsets = SDATA (val);
        }
      CODING_ISO_FLAGS (coding) = flags;
      CODING_ISO_CMP_STATUS (coding)->state = COMPOSING_NO;
      CODING_ISO_CMP_STATUS (coding)->method = COMPOSITION_NO;
      CODING_ISO_EXTSEGMENT_LEN (coding) = 0;
      CODING_ISO_EMBEDDED_UTF_8 (coding) = 0;
    }
  else if (EQ (coding_type, Qcharset))
    {
#if TODO_NELISP_LATER_AND
      coding->detector = detect_coding_charset;
      coding->decoder = decode_coding_charset;
      coding->encoder = encode_coding_charset;
#endif
      coding->common_flags
        |= (CODING_REQUIRE_DECODING_MASK | CODING_REQUIRE_ENCODING_MASK);
    }
  else if (EQ (coding_type, Qutf_8))
    {
      val = AREF (attrs, coding_attr_utf_bom);
      CODING_UTF_8_BOM (coding) = (CONSP (val)    ? utf_detect_bom
                                   : EQ (val, Qt) ? utf_with_bom
                                                  : utf_without_bom);
#if TODO_NELISP_LATER_AND
      coding->detector = detect_coding_utf_8;
      coding->decoder = decode_coding_utf_8;
      coding->encoder = encode_coding_utf_8;
#endif
      coding->common_flags
        |= (CODING_REQUIRE_DECODING_MASK | CODING_REQUIRE_ENCODING_MASK);
      if (CODING_UTF_8_BOM (coding) == utf_detect_bom)
        coding->common_flags |= CODING_REQUIRE_DETECTION_MASK;
    }
  else if (EQ (coding_type, Qutf_16))
    {
      val = AREF (attrs, coding_attr_utf_bom);
      CODING_UTF_16_BOM (coding) = (CONSP (val)    ? utf_detect_bom
                                    : EQ (val, Qt) ? utf_with_bom
                                                   : utf_without_bom);
      val = AREF (attrs, coding_attr_utf_16_endian);
      CODING_UTF_16_ENDIAN (coding)
        = (EQ (val, Qbig) ? utf_16_big_endian : utf_16_little_endian);
      CODING_UTF_16_SURROGATE (coding) = 0;
#if TODO_NELISP_LATER_AND
      coding->detector = detect_coding_utf_16;
      coding->decoder = decode_coding_utf_16;
      coding->encoder = encode_coding_utf_16;
#endif
      coding->common_flags
        |= (CODING_REQUIRE_DECODING_MASK | CODING_REQUIRE_ENCODING_MASK);
      if (CODING_UTF_16_BOM (coding) == utf_detect_bom)
        coding->common_flags |= CODING_REQUIRE_DETECTION_MASK;
    }
  else if (EQ (coding_type, Qccl))
    {
#if TODO_NELISP_LATER_AND
      coding->detector = detect_coding_ccl;
      coding->decoder = decode_coding_ccl;
      coding->encoder = encode_coding_ccl;
#endif
      coding->common_flags
        |= (CODING_REQUIRE_DECODING_MASK | CODING_REQUIRE_ENCODING_MASK
            | CODING_REQUIRE_FLUSHING_MASK);
    }
  else if (EQ (coding_type, Qemacs_mule))
    {
#if TODO_NELISP_LATER_AND
      coding->detector = detect_coding_emacs_mule;
      coding->decoder = decode_coding_emacs_mule;
      coding->encoder = encode_coding_emacs_mule;
#endif
      coding->common_flags
        |= (CODING_REQUIRE_DECODING_MASK | CODING_REQUIRE_ENCODING_MASK);
      if (!NILP (AREF (attrs, coding_attr_emacs_mule_full))
          && !EQ (CODING_ATTR_CHARSET_LIST (attrs), Vemacs_mule_charset_list))
        {
          Lisp_Object tail, safe_charsets;
          int max_charset_id = 0;

          for (tail = Vemacs_mule_charset_list; CONSP (tail);
               tail = XCDR (tail))
            if (max_charset_id < XFIXNAT (XCAR (tail)))
              max_charset_id = XFIXNAT (XCAR (tail));
          safe_charsets = make_uninit_string (max_charset_id + 1);
          memset (SDATA (safe_charsets), 255, max_charset_id + 1);
          for (tail = Vemacs_mule_charset_list; CONSP (tail);
               tail = XCDR (tail))
            SSET (safe_charsets, XFIXNAT (XCAR (tail)), 0);
          coding->max_charset_id = max_charset_id;
          coding->safe_charsets = SDATA (safe_charsets);
        }
      coding->spec.emacs_mule.cmp_status.state = COMPOSING_NO;
      coding->spec.emacs_mule.cmp_status.method = COMPOSITION_NO;
    }
  else if (EQ (coding_type, Qshift_jis))
    {
#if TODO_NELISP_LATER_AND
      coding->detector = detect_coding_sjis;
      coding->decoder = decode_coding_sjis;
      coding->encoder = encode_coding_sjis;
#endif
      coding->common_flags
        |= (CODING_REQUIRE_DECODING_MASK | CODING_REQUIRE_ENCODING_MASK);
    }
  else if (EQ (coding_type, Qbig5))
    {
#if TODO_NELISP_LATER_AND
      coding->detector = detect_coding_big5;
      coding->decoder = decode_coding_big5;
      coding->encoder = encode_coding_big5;
#endif
      coding->common_flags
        |= (CODING_REQUIRE_DECODING_MASK | CODING_REQUIRE_ENCODING_MASK);
    }
  else
    {
#if TODO_NELISP_LATER_AND
      coding->detector = NULL;
      coding->decoder = decode_coding_raw_text;
      coding->encoder = encode_coding_raw_text;
#endif
      if (!EQ (eol_type, Qunix))
        {
          coding->common_flags |= CODING_REQUIRE_DECODING_MASK;
          if (!VECTORP (eol_type))
            coding->common_flags |= CODING_REQUIRE_ENCODING_MASK;
        }
    }

  return;
}

static void
setup_iso_safe_charsets (Lisp_Object attrs)
{
  Lisp_Object charset_list, safe_charsets;
  Lisp_Object request;
  Lisp_Object reg_usage;
  Lisp_Object tail;
  EMACS_INT reg94, reg96;
  int flags = XFIXNUM (AREF (attrs, coding_attr_iso_flags));
  int max_charset_id;

  charset_list = CODING_ATTR_CHARSET_LIST (attrs);
  if ((flags & CODING_ISO_FLAG_FULL_SUPPORT)
      && !EQ (charset_list, Viso_2022_charset_list))
    {
      charset_list = Viso_2022_charset_list;
      ASET (attrs, coding_attr_charset_list, charset_list);
      ASET (attrs, coding_attr_safe_charsets, Qnil);
    }

  if (STRINGP (AREF (attrs, coding_attr_safe_charsets)))
    return;

  max_charset_id = 0;
  for (tail = charset_list; CONSP (tail); tail = XCDR (tail))
    {
      int id = XFIXNUM (XCAR (tail));
      if (max_charset_id < id)
        max_charset_id = id;
    }

  safe_charsets = make_uninit_string (max_charset_id + 1);
  memset (SDATA (safe_charsets), 255, max_charset_id + 1);
  request = AREF (attrs, coding_attr_iso_request);
  reg_usage = AREF (attrs, coding_attr_iso_usage);
  reg94 = XFIXNUM (XCAR (reg_usage));
  reg96 = XFIXNUM (XCDR (reg_usage));

  for (tail = charset_list; CONSP (tail); tail = XCDR (tail))
    {
      Lisp_Object id;
      Lisp_Object reg;
      struct charset *charset;

      id = XCAR (tail);
      charset = CHARSET_FROM_ID (XFIXNUM (id));
      reg = Fcdr (Fassq (id, request));
      if (!NILP (reg))
        SSET (safe_charsets, XFIXNUM (id), XFIXNUM (reg));
      else if (charset->iso_chars_96)
        {
          if (reg96 < 4)
            SSET (safe_charsets, XFIXNUM (id), reg96);
        }
      else
        {
          if (reg94 < 4)
            SSET (safe_charsets, XFIXNUM (id), reg94);
        }
    }
  ASET (attrs, coding_attr_safe_charsets, safe_charsets);
}

static Lisp_Object
make_subsidiaries (Lisp_Object base)
{
  static char const suffixes[][8] = { "-unix", "-dos", "-mac" };
  ptrdiff_t base_name_len = SBYTES (SYMBOL_NAME (base));
  USE_SAFE_ALLOCA;
  char *buf = SAFE_ALLOCA (base_name_len + 6);

  memcpy (buf, SDATA (SYMBOL_NAME (base)), base_name_len);
  Lisp_Object subsidiaries = make_nil_vector (3);
  for (int i = 0; i < 3; i++)
    {
      strcpy (buf + base_name_len, suffixes[i]);
      ASET (subsidiaries, i, intern (buf));
    }
  SAFE_FREE ();
  return subsidiaries;
}

DEFUN ("define-coding-system-internal", Fdefine_coding_system_internal,
       Sdefine_coding_system_internal, coding_arg_max, MANY, 0,
       doc: /* For internal use only.
usage: (define-coding-system-internal ...)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  enum coding_category category;
  int max_charset_id = 0;

  if (nargs < coding_arg_max)
    goto short_args;

  Lisp_Object attrs = make_nil_vector (coding_attr_last_index);

  Lisp_Object name = args[coding_arg_name];
  CHECK_SYMBOL (name);
  ASET (attrs, coding_attr_base_name, name);

  Lisp_Object val = args[coding_arg_mnemonic];
  if (STRINGP (val))
    val = make_fixnum (STRING_CHAR (SDATA (val)));
  else
    CHECK_CHARACTER (val);
  ASET (attrs, coding_attr_mnemonic, val);

  Lisp_Object coding_type = args[coding_arg_coding_type];
  CHECK_SYMBOL (coding_type);
  ASET (attrs, coding_attr_type, coding_type);

  Lisp_Object charset_list = args[coding_arg_charset_list];
  if (SYMBOLP (charset_list))
    {
      if (EQ (charset_list, Qiso_2022))
        {
          if (!EQ (coding_type, Qiso_2022))
            error ("Invalid charset-list");
          charset_list = Viso_2022_charset_list;
        }
      else if (EQ (charset_list, Qemacs_mule))
        {
          if (!EQ (coding_type, Qemacs_mule))
            error ("Invalid charset-list");
          charset_list = Vemacs_mule_charset_list;
        }
      for (Lisp_Object tail = charset_list; CONSP (tail); tail = XCDR (tail))
        {
          if (!RANGED_FIXNUMP (0, XCAR (tail), INT_MAX - 1))
            error ("Invalid charset-list");
          if (max_charset_id < XFIXNAT (XCAR (tail)))
            max_charset_id = XFIXNAT (XCAR (tail));
        }
    }
  else
    {
      charset_list = Fcopy_sequence (charset_list);
      for (Lisp_Object tail = charset_list; CONSP (tail); tail = XCDR (tail))
        {
          struct charset *charset;

          val = XCAR (tail);
          CHECK_CHARSET_GET_CHARSET (val, charset);
          if (EQ (coding_type, Qiso_2022) ? CHARSET_ISO_FINAL (charset) < 0
              : EQ (coding_type, Qemacs_mule)
                ? CHARSET_EMACS_MULE_ID (charset) < 0
                : 0)
            error ("Can't handle charset `%s'",
                   SDATA (SYMBOL_NAME (CHARSET_NAME (charset))));

          XSETCAR (tail, make_fixnum (charset->id));
          if (max_charset_id < charset->id)
            max_charset_id = charset->id;
        }
    }
  ASET (attrs, coding_attr_charset_list, charset_list);

  Lisp_Object safe_charsets = make_uninit_string (max_charset_id + 1);
  memset (SDATA (safe_charsets), 255, max_charset_id + 1);
  for (Lisp_Object tail = charset_list; CONSP (tail); tail = XCDR (tail))
    SSET (safe_charsets, XFIXNAT (XCAR (tail)), 0);
  ASET (attrs, coding_attr_safe_charsets, safe_charsets);

  ASET (attrs, coding_attr_ascii_compat, args[coding_arg_ascii_compatible_p]);

  val = args[coding_arg_decode_translation_table];
  if (!CHAR_TABLE_P (val) && !CONSP (val))
    CHECK_SYMBOL (val);
  ASET (attrs, coding_attr_decode_tbl, val);

  val = args[coding_arg_encode_translation_table];
  if (!CHAR_TABLE_P (val) && !CONSP (val))
    CHECK_SYMBOL (val);
  ASET (attrs, coding_attr_encode_tbl, val);

  val = args[coding_arg_post_read_conversion];
  CHECK_SYMBOL (val);
  ASET (attrs, coding_attr_post_read, val);

  val = args[coding_arg_pre_write_conversion];
  CHECK_SYMBOL (val);
  ASET (attrs, coding_attr_pre_write, val);

  val = args[coding_arg_default_char];
  if (NILP (val))
    ASET (attrs, coding_attr_default_char, make_fixnum (' '));
  else
    {
      CHECK_CHARACTER (val);
      ASET (attrs, coding_attr_default_char, val);
    }

  val = args[coding_arg_for_unibyte];
  ASET (attrs, coding_attr_for_unibyte, NILP (val) ? Qnil : Qt);

  val = args[coding_arg_plist];
  CHECK_LIST (val);
  ASET (attrs, coding_attr_plist, val);

  if (EQ (coding_type, Qcharset))
    {
      val = make_nil_vector (256);

      for (Lisp_Object tail = charset_list; CONSP (tail); tail = XCDR (tail))
        {
          struct charset *charset = CHARSET_FROM_ID (XFIXNAT (XCAR (tail)));
          int dim = CHARSET_DIMENSION (charset);
          int idx = (dim - 1) * 4;

          if (CHARSET_ASCII_COMPATIBLE_P (charset))
            ASET (attrs, coding_attr_ascii_compat, Qt);

          for (int i = charset->code_space[idx];
               i <= charset->code_space[idx + 1]; i++)
            {
              Lisp_Object tmp, tmp2;
              int dim2;

              tmp = AREF (val, i);
              if (NILP (tmp))
                tmp = XCAR (tail);
              else if (FIXNATP (tmp))
                {
                  dim2 = CHARSET_DIMENSION (CHARSET_FROM_ID (XFIXNAT (tmp)));
                  if (dim < dim2)
                    tmp = list2 (XCAR (tail), tmp);
                  else
                    tmp = list2 (tmp, XCAR (tail));
                }
              else
                {
                  for (tmp2 = tmp; CONSP (tmp2); tmp2 = XCDR (tmp2))
                    {
                      dim2 = CHARSET_DIMENSION (
                        CHARSET_FROM_ID (XFIXNAT (XCAR (tmp2))));
                      if (dim < dim2)
                        break;
                    }
                  if (NILP (tmp2))
                    tmp = nconc2 (tmp, list1 (XCAR (tail)));
                  else
                    {
                      XSETCDR (tmp2, Fcons (XCAR (tmp2), XCDR (tmp2)));
                      XSETCAR (tmp2, XCAR (tail));
                    }
                }
              ASET (val, i, tmp);
            }
        }
      ASET (attrs, coding_attr_charset_valids, val);
      category = coding_category_charset;
    }
  else if (EQ (coding_type, Qccl))
    {
      Lisp_Object valids;

      if (nargs < coding_arg_ccl_max)
        goto short_args;

      val = args[coding_arg_ccl_decoder];
      TODO; // CHECK_CCL_PROGRAM (val);
      if (VECTORP (val))
        val = Fcopy_sequence (val);
      ASET (attrs, coding_attr_ccl_decoder, val);

      val = args[coding_arg_ccl_encoder];
      TODO; // CHECK_CCL_PROGRAM (val);
      if (VECTORP (val))
        val = Fcopy_sequence (val);
      ASET (attrs, coding_attr_ccl_encoder, val);

      val = args[coding_arg_ccl_valids];
      TODO; // valids = Fmake_string (make_fixnum (256), make_fixnum (0), Qnil);
      for (Lisp_Object tail = val; CONSP (tail); tail = XCDR (tail))
        {
          int from, to;

          val = XCAR (tail);
          if (FIXNUMP (val))
            {
              if (!(0 <= XFIXNUM (val) && XFIXNUM (val) <= 255))
                args_out_of_range_3 (val, make_fixnum (0), make_fixnum (255));
              from = to = XFIXNUM (val);
            }
          else
            {
              CHECK_CONS (val);
              from = check_integer_range (XCAR (val), 0, 255);
              to = check_integer_range (XCDR (val), from, 255);
            }
          for (int i = from; i <= to; i++)
            SSET (valids, i, 1);
        }
      ASET (attrs, coding_attr_ccl_valids, valids);

      category = coding_category_ccl;
    }
  else if (EQ (coding_type, Qutf_16))
    {
      Lisp_Object bom, endian;

      ASET (attrs, coding_attr_ascii_compat, Qnil);

      if (nargs < coding_arg_utf16_max)
        goto short_args;

      bom = args[coding_arg_utf16_bom];
      if (!NILP (bom) && !EQ (bom, Qt))
        {
          CHECK_CONS (bom);
          val = XCAR (bom);
          CHECK_CODING_SYSTEM (val);
          val = XCDR (bom);
          CHECK_CODING_SYSTEM (val);
        }
      ASET (attrs, coding_attr_utf_bom, bom);

      endian = args[coding_arg_utf16_endian];
      CHECK_SYMBOL (endian);
      if (NILP (endian))
        endian = Qbig;
      else if (!EQ (endian, Qbig) && !EQ (endian, Qlittle))
        error ("Invalid endian: %s", SDATA (SYMBOL_NAME (endian)));
      ASET (attrs, coding_attr_utf_16_endian, endian);

      category
        = (CONSP (bom)  ? coding_category_utf_16_auto
           : NILP (bom) ? (EQ (endian, Qbig) ? coding_category_utf_16_be_nosig
                                             : coding_category_utf_16_le_nosig)
                        : (EQ (endian, Qbig) ? coding_category_utf_16_be
                                             : coding_category_utf_16_le));
    }
  else if (EQ (coding_type, Qiso_2022))
    {
      Lisp_Object initial, reg_usage, request, flags;

      if (nargs < coding_arg_iso2022_max)
        goto short_args;

      initial = Fcopy_sequence (args[coding_arg_iso2022_initial]);
      CHECK_VECTOR (initial);
      for (int i = 0; i < 4; i++)
        {
          val = AREF (initial, i);
          if (!NILP (val))
            {
              struct charset *charset;

              CHECK_CHARSET_GET_CHARSET (val, charset);
              ASET (initial, i, make_fixnum (CHARSET_ID (charset)));
              if (i == 0 && CHARSET_ASCII_COMPATIBLE_P (charset))
                ASET (attrs, coding_attr_ascii_compat, Qt);
            }
          else
            ASET (initial, i, make_fixnum (-1));
        }

      reg_usage = args[coding_arg_iso2022_reg_usage];
      CHECK_CONS (reg_usage);
      CHECK_FIXNUM (XCAR (reg_usage));
      CHECK_FIXNUM (XCDR (reg_usage));

      request = Fcopy_sequence (args[coding_arg_iso2022_request]);
      for (Lisp_Object tail = request; CONSP (tail); tail = XCDR (tail))
        {
          int id;

          val = XCAR (tail);
          CHECK_CONS (val);
          CHECK_CHARSET_GET_ID (XCAR (val), id);
          check_integer_range (XCDR (val), 0, 3);
          XSETCAR (val, make_fixnum (id));
        }

      flags = args[coding_arg_iso2022_flags];
      CHECK_FIXNAT (flags);
      int i = XFIXNUM (flags) & INT_MAX;
      if (EQ (args[coding_arg_charset_list], Qiso_2022))
        i |= CODING_ISO_FLAG_FULL_SUPPORT;
      flags = make_fixnum (i);

      ASET (attrs, coding_attr_iso_initial, initial);
      ASET (attrs, coding_attr_iso_usage, reg_usage);
      ASET (attrs, coding_attr_iso_request, request);
      ASET (attrs, coding_attr_iso_flags, flags);
      setup_iso_safe_charsets (attrs);

      if (i & CODING_ISO_FLAG_SEVEN_BITS)
        category
          = ((i
              & (CODING_ISO_FLAG_LOCKING_SHIFT | CODING_ISO_FLAG_SINGLE_SHIFT))
               ? coding_category_iso_7_else
             : EQ (args[coding_arg_charset_list], Qiso_2022)
               ? coding_category_iso_7
               : coding_category_iso_7_tight);
      else
        {
          int id = XFIXNUM (AREF (initial, 1));

          category
            = (((i & CODING_ISO_FLAG_LOCKING_SHIFT)
                || EQ (args[coding_arg_charset_list], Qiso_2022) || id < 0)
                 ? coding_category_iso_8_else
               : (CHARSET_DIMENSION (CHARSET_FROM_ID (id)) == 1)
                 ? coding_category_iso_8_1
                 : coding_category_iso_8_2);
        }
      if (category != coding_category_iso_8_1
          && category != coding_category_iso_8_2)
        ASET (attrs, coding_attr_ascii_compat, Qnil);
    }
  else if (EQ (coding_type, Qemacs_mule))
    {
      if (EQ (args[coding_arg_charset_list], Qemacs_mule))
        ASET (attrs, coding_attr_emacs_mule_full, Qt);
      ASET (attrs, coding_attr_ascii_compat, Qt);
      category = coding_category_emacs_mule;
    }
  else if (EQ (coding_type, Qshift_jis))
    {
      ptrdiff_t charset_list_len = list_length (charset_list);
      if (charset_list_len != 3 && charset_list_len != 4)
        error ("There should be three or four charsets");

      struct charset *charset = CHARSET_FROM_ID (XFIXNUM (XCAR (charset_list)));
      if (CHARSET_DIMENSION (charset) != 1)
        error ("Dimension of charset %s is not one",
               SDATA (SYMBOL_NAME (CHARSET_NAME (charset))));
      if (CHARSET_ASCII_COMPATIBLE_P (charset))
        ASET (attrs, coding_attr_ascii_compat, Qt);

      charset_list = XCDR (charset_list);
      charset = CHARSET_FROM_ID (XFIXNUM (XCAR (charset_list)));
      if (CHARSET_DIMENSION (charset) != 1)
        error ("Dimension of charset %s is not one",
               SDATA (SYMBOL_NAME (CHARSET_NAME (charset))));

      charset_list = XCDR (charset_list);
      charset = CHARSET_FROM_ID (XFIXNUM (XCAR (charset_list)));
      if (CHARSET_DIMENSION (charset) != 2)
        error ("Dimension of charset %s is not two",
               SDATA (SYMBOL_NAME (CHARSET_NAME (charset))));

      charset_list = XCDR (charset_list);
      if (!NILP (charset_list))
        {
          charset = CHARSET_FROM_ID (XFIXNUM (XCAR (charset_list)));
          if (CHARSET_DIMENSION (charset) != 2)
            error ("Dimension of charset %s is not two",
                   SDATA (SYMBOL_NAME (CHARSET_NAME (charset))));
        }

      category = coding_category_sjis;
      Vsjis_coding_system = name;
    }
  else if (EQ (coding_type, Qbig5))
    {
      struct charset *charset;

      if (list_length (charset_list) != 2)
        error ("There should be just two charsets");

      charset = CHARSET_FROM_ID (XFIXNUM (XCAR (charset_list)));
      if (CHARSET_DIMENSION (charset) != 1)
        error ("Dimension of charset %s is not one",
               SDATA (SYMBOL_NAME (CHARSET_NAME (charset))));
      if (CHARSET_ASCII_COMPATIBLE_P (charset))
        ASET (attrs, coding_attr_ascii_compat, Qt);

      charset_list = XCDR (charset_list);
      charset = CHARSET_FROM_ID (XFIXNUM (XCAR (charset_list)));
      if (CHARSET_DIMENSION (charset) != 2)
        error ("Dimension of charset %s is not two",
               SDATA (SYMBOL_NAME (CHARSET_NAME (charset))));

      category = coding_category_big5;
      Vbig5_coding_system = name;
    }
  else if (EQ (coding_type, Qraw_text))
    {
      category = coding_category_raw_text;
      ASET (attrs, coding_attr_ascii_compat, Qt);
    }
  else if (EQ (coding_type, Qutf_8))
    {
      Lisp_Object bom;

      if (nargs < coding_arg_utf8_max)
        goto short_args;

      bom = args[coding_arg_utf8_bom];
      if (!NILP (bom) && !EQ (bom, Qt))
        {
          CHECK_CONS (bom);
          val = XCAR (bom);
          CHECK_CODING_SYSTEM (val);
          val = XCDR (bom);
          CHECK_CODING_SYSTEM (val);
        }
      ASET (attrs, coding_attr_utf_bom, bom);
      if (NILP (bom))
        ASET (attrs, coding_attr_ascii_compat, Qt);

      category = (CONSP (bom)  ? coding_category_utf_8_auto
                  : NILP (bom) ? coding_category_utf_8_nosig
                               : coding_category_utf_8_sig);
    }
  else if (EQ (coding_type, Qundecided))
    {
      if (nargs < coding_arg_undecided_max)
        goto short_args;
      ASET (attrs, coding_attr_undecided_inhibit_null_byte_detection,
            args[coding_arg_undecided_inhibit_null_byte_detection]);
      ASET (attrs, coding_attr_undecided_inhibit_iso_escape_detection,
            args[coding_arg_undecided_inhibit_iso_escape_detection]);
      ASET (attrs, coding_attr_undecided_prefer_utf_8,
            args[coding_arg_undecided_prefer_utf_8]);
      category = coding_category_undecided;
    }
  else
    error ("Invalid coding system type: %s", SDATA (SYMBOL_NAME (coding_type)));

  ASET (attrs, coding_attr_category, make_fixnum (category));
  ASET (attrs, coding_attr_plist,
        Fcons (QCcategory, Fcons (AREF (Vcoding_category_table, category),
                                  CODING_ATTR_PLIST (attrs))));
  ASET (attrs, coding_attr_plist,
        Fcons (QCascii_compatible_p, Fcons (CODING_ATTR_ASCII_COMPAT (attrs),
                                            CODING_ATTR_PLIST (attrs))));

  Lisp_Object eol_type = args[coding_arg_eol_type];
  if (!NILP (eol_type) && !EQ (eol_type, Qunix) && !EQ (eol_type, Qdos)
      && !EQ (eol_type, Qmac))
    error ("Invalid eol-type");

  Lisp_Object aliases = list1 (name);

  if (NILP (eol_type))
    {
      eol_type = make_subsidiaries (name);
      for (int i = 0; i < 3; i++)
        {
          Lisp_Object this_spec, this_name, this_aliases, this_eol_type;

          this_name = AREF (eol_type, i);
          this_aliases = list1 (this_name);
          this_eol_type = (i == 0 ? Qunix : i == 1 ? Qdos : Qmac);
          this_spec = make_uninit_vector (3);
          ASET (this_spec, 0, attrs);
          ASET (this_spec, 1, this_aliases);
          ASET (this_spec, 2, this_eol_type);
          Fputhash (this_name, this_spec, Vcoding_system_hash_table);
          Vcoding_system_list = Fcons (this_name, Vcoding_system_list);
          val = Fassoc (Fsymbol_name (this_name), Vcoding_system_alist, Qnil);
          if (NILP (val))
            Vcoding_system_alist
              = Fcons (Fcons (Fsymbol_name (this_name), Qnil),
                       Vcoding_system_alist);
        }
    }

  Lisp_Object spec_vec = make_uninit_vector (3);
  ASET (spec_vec, 0, attrs);
  ASET (spec_vec, 1, aliases);
  ASET (spec_vec, 2, eol_type);

  Fputhash (name, spec_vec, Vcoding_system_hash_table);
  Vcoding_system_list = Fcons (name, Vcoding_system_list);
  val = Fassoc (Fsymbol_name (name), Vcoding_system_alist, Qnil);
  if (NILP (val))
    Vcoding_system_alist
      = Fcons (Fcons (Fsymbol_name (name), Qnil), Vcoding_system_alist);

  int id = coding_categories[category].id;
  if (id < 0 || EQ (name, CODING_ID_NAME (id)))
    setup_coding_system (name, &coding_categories[category]);

  return Qnil;

short_args:
  Fsignal (Qwrong_number_of_arguments,
           Fcons (Qdefine_coding_system_internal, make_fixnum (nargs)));
}

DEFUN ("coding-system-put", Fcoding_system_put, Scoding_system_put,
       3, 3, 0,
       doc: /* Change value of CODING-SYSTEM's property PROP to VAL.

The following properties, if set by this function, override the values
of the corresponding attributes set by `define-coding-system':

  `:mnemonic', `:default-char', `:ascii-compatible-p'
  `:decode-translation-table', `:encode-translation-table',
  `:post-read-conversion', `:pre-write-conversion'

See `define-coding-system' for the description of these properties.
See `coding-system-get' and `coding-system-plist' for accessing the
property list of a coding-system.  */)
(Lisp_Object coding_system, Lisp_Object prop, Lisp_Object val)
{
  Lisp_Object spec, attrs;

  CHECK_CODING_SYSTEM_GET_SPEC (coding_system, spec);
  attrs = AREF (spec, 0);
  if (EQ (prop, QCmnemonic))
    {
      /* decode_mode_spec_coding assumes the mnemonic is a single character.  */
      if (STRINGP (val))
        val = make_fixnum (STRING_CHAR (SDATA (val)));
      else
        CHECK_CHARACTER (val);
      ASET (attrs, coding_attr_mnemonic, val);
    }
  else if (EQ (prop, QCdefault_char))
    {
      if (NILP (val))
        val = make_fixnum (' ');
      else
        CHECK_CHARACTER (val);
      ASET (attrs, coding_attr_default_char, val);
    }
  else if (EQ (prop, QCdecode_translation_table))
    {
      if (!CHAR_TABLE_P (val) && !CONSP (val))
        CHECK_SYMBOL (val);
      ASET (attrs, coding_attr_decode_tbl, val);
    }
  else if (EQ (prop, QCencode_translation_table))
    {
      if (!CHAR_TABLE_P (val) && !CONSP (val))
        CHECK_SYMBOL (val);
      ASET (attrs, coding_attr_encode_tbl, val);
    }
  else if (EQ (prop, QCpost_read_conversion))
    {
      CHECK_SYMBOL (val);
      ASET (attrs, coding_attr_post_read, val);
    }
  else if (EQ (prop, QCpre_write_conversion))
    {
      CHECK_SYMBOL (val);
      ASET (attrs, coding_attr_pre_write, val);
    }
  else if (EQ (prop, QCascii_compatible_p))
    {
      ASET (attrs, coding_attr_ascii_compat, val);
    }

  ASET (attrs, coding_attr_plist,
        plist_put (CODING_ATTR_PLIST (attrs), prop, val));
  return val;
}

DEFUN ("define-coding-system-alias", Fdefine_coding_system_alias,
       Sdefine_coding_system_alias, 2, 2, 0,
       doc: /* Define ALIAS as an alias for CODING-SYSTEM.  */)
(Lisp_Object alias, Lisp_Object coding_system)
{
  Lisp_Object spec, aliases, eol_type, val;

  CHECK_SYMBOL (alias);
  CHECK_CODING_SYSTEM_GET_SPEC (coding_system, spec);
  aliases = AREF (spec, 1);

  while (!NILP (XCDR (aliases)))
    aliases = XCDR (aliases);
  XSETCDR (aliases, list1 (alias));

  eol_type = AREF (spec, 2);
  if (VECTORP (eol_type))
    {
      Lisp_Object subsidiaries;
      int i;

      subsidiaries = make_subsidiaries (alias);
      for (i = 0; i < 3; i++)
        Fdefine_coding_system_alias (AREF (subsidiaries, i),
                                     AREF (eol_type, i));
    }

  Fputhash (alias, spec, Vcoding_system_hash_table);
  Vcoding_system_list = Fcons (alias, Vcoding_system_list);
  val = Fassoc (Fsymbol_name (alias), Vcoding_system_alist, Qnil);
  if (NILP (val))
    Vcoding_system_alist
      = Fcons (Fcons (Fsymbol_name (alias), Qnil), Vcoding_system_alist);

  return Qnil;
}

DEFUN ("coding-system-base", Fcoding_system_base, Scoding_system_base,
       1, 1, 0,
       doc: /* Return the base of CODING-SYSTEM.
Any alias or subsidiary coding system is not a base coding system.  */)
(Lisp_Object coding_system)
{
  Lisp_Object spec, attrs;

  if (NILP (coding_system))
    return (Qno_conversion);
  CHECK_CODING_SYSTEM_GET_SPEC (coding_system, spec);
  attrs = AREF (spec, 0);
  return CODING_ATTR_BASE_NAME (attrs);
}

DEFUN ("coding-system-eol-type", Fcoding_system_eol_type,
       Scoding_system_eol_type, 1, 1, 0,
       doc: /* Return eol-type of CODING-SYSTEM.
An eol-type is an integer 0, 1, 2, or a vector of coding systems.

Integer values 0, 1, and 2 indicate a format of end-of-line; LF, CRLF,
and CR respectively.

A vector value indicates that a format of end-of-line should be
detected automatically.  Nth element of the vector is the subsidiary
coding system whose eol-type is N.  */)
(Lisp_Object coding_system)
{
  Lisp_Object spec, eol_type;
  int n;

  if (NILP (coding_system))
    coding_system = Qno_conversion;
  if (!CODING_SYSTEM_P (coding_system))
    return Qnil;
  spec = CODING_SYSTEM_SPEC (coding_system);
  eol_type = AREF (spec, 2);
  if (VECTORP (eol_type))
    return Fcopy_sequence (eol_type);
  n = EQ (eol_type, Qunix) ? 0 : EQ (eol_type, Qdos) ? 1 : 2;
  return make_fixnum (n);
}

DEFUN ("set-coding-system-priority", Fset_coding_system_priority,
       Sset_coding_system_priority, 0, MANY, 0,
       doc: /* Assign higher priority to the coding systems given as arguments.
If multiple coding systems belong to the same category,
all but the first one are ignored.

usage: (set-coding-system-priority &rest coding-systems)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  ptrdiff_t i, j;
  bool changed[coding_category_max];
  enum coding_category priorities[coding_category_max];

  memset (changed, 0, sizeof changed);

  for (i = j = 0; i < nargs; i++)
    {
      enum coding_category category;
      Lisp_Object spec, attrs;

      CHECK_CODING_SYSTEM_GET_SPEC (args[i], spec);
      attrs = AREF (spec, 0);
      category = XFIXNUM (CODING_ATTR_CATEGORY (attrs));
      if (changed[category])
        continue;
      changed[category] = 1;
      priorities[j++] = category;
      if (coding_categories[category].id >= 0
          && !EQ (args[i], CODING_ID_NAME (coding_categories[category].id)))
        setup_coding_system (args[i], &coding_categories[category]);
      Fset (AREF (Vcoding_category_table, category), args[i]);
    }

  for (i = j, j = 0; i < coding_category_max; i++, j++)
    {
      while (j < coding_category_max && changed[coding_priorities[j]])
        j++;
      if (j == coding_category_max)
        emacs_abort ();
      priorities[i] = coding_priorities[j];
    }

  memcpy (coding_priorities, priorities, sizeof priorities);

  Vcoding_category_list = Qnil;
  for (i = coding_category_max; i-- > 0;)
    Vcoding_category_list = Fcons (AREF (Vcoding_category_table, priorities[i]),
                                   Vcoding_category_list);

  return Qnil;
}

DEFUN ("set-safe-terminal-coding-system-internal",
       Fset_safe_terminal_coding_system_internal,
       Sset_safe_terminal_coding_system_internal, 1, 1, 0,
       doc: /* Internal use only.  */)
(Lisp_Object coding_system)
{
  CHECK_SYMBOL (coding_system);
  setup_coding_system (Fcheck_coding_system (coding_system),
                       &safe_terminal_coding);
  safe_terminal_coding.common_flags &= ~CODING_ANNOTATE_COMPOSITION_MASK;
  safe_terminal_coding.src_multibyte = 1;
  safe_terminal_coding.dst_multibyte = 0;
  return Qnil;
}

bool
string_ascii_p (Lisp_Object string)
{
  ptrdiff_t nbytes = SBYTES (string);
  for (ptrdiff_t i = 0; i < nbytes; i++)
    if (SREF (string, i) > 127)
      return false;
  return true;
}

void
init_coding_once (void)
{
  int i;

  for (i = 0; i < coding_category_max; i++)
    {
      coding_categories[i].id = -1;
      coding_priorities[i] = i;
    }
}

void
syms_of_coding (void)
{
  staticpro (&Vcoding_system_hash_table);
  Vcoding_system_hash_table = CALLN (Fmake_hash_table, QCtest, Qeq);

  staticpro (&Vsjis_coding_system);
  Vsjis_coding_system = Qnil;

  staticpro (&Vbig5_coding_system);
  Vbig5_coding_system = Qnil;

  DEFSYM (Qfilenamep, "filenamep");

  DEFSYM (QCcategory, ":category");
  DEFSYM (QCmnemonic, ":mnemonic");
  DEFSYM (QCdefault_char, ":default-char");
  DEFSYM (QCdecode_translation_table, ":decode-translation-table");
  DEFSYM (QCencode_translation_table, ":encode-translation-table");
  DEFSYM (QCpost_read_conversion, ":post-read-conversion");
  DEFSYM (QCpre_write_conversion, ":pre-write-conversion");
  DEFSYM (QCascii_compatible_p, ":ascii-compatible-p");

  DEFSYM (Qunix, "unix");
  DEFSYM (Qdos, "dos");
  DEFSYM (Qmac, "mac");

  DEFSYM (Qbuffer_file_coding_system, "buffer-file-coding-system");
  DEFSYM (Qundecided, "undecided");
  DEFSYM (Qno_conversion, "no-conversion");
  DEFSYM (Qraw_text, "raw-text");
  DEFSYM (Qus_ascii, "us-ascii");

  DEFSYM (Qiso_2022, "iso-2022");
  DEFSYM (Qutf_8, "utf-8");
  DEFSYM (Qutf_8_unix, "utf-8-unix");
  DEFSYM (Qutf_8_emacs, "utf-8-emacs");
  DEFSYM (Qemacs_mule, "emacs-mule");

  DEFSYM (Qutf_16, "utf-16");
  DEFSYM (Qbig, "big");
  DEFSYM (Qlittle, "little");
  DEFSYM (Qshift_jis, "shift-jis");
  DEFSYM (Qbig5, "big5");

  DEFSYM (Qcoding_system_p, "coding-system-p");

  DEFSYM (Qtranslation_table, "translation-table");
  Fput (Qtranslation_table, Qchar_table_extra_slots, make_fixnum (2));
  DEFSYM (Qtranslation_table_id, "translation-table-id");

  DEFSYM (Qcoding_system_error, "coding-system-error");
  Fput (Qcoding_system_error, Qerror_conditions,
        pure_list (Qcoding_system_error, Qerror));
  Fput (Qcoding_system_error, Qerror_message,
        build_pure_c_string ("Invalid coding system"));

  DEFSYM (Qcharset, "charset");

  Vcoding_category_table = make_nil_vector (coding_category_max);
  staticpro (&Vcoding_category_table);
  ASET (Vcoding_category_table, coding_category_iso_7,
        intern_c_string ("coding-category-iso-7"));
  ASET (Vcoding_category_table, coding_category_iso_7_tight,
        intern_c_string ("coding-category-iso-7-tight"));
  ASET (Vcoding_category_table, coding_category_iso_8_1,
        intern_c_string ("coding-category-iso-8-1"));
  ASET (Vcoding_category_table, coding_category_iso_8_2,
        intern_c_string ("coding-category-iso-8-2"));
  ASET (Vcoding_category_table, coding_category_iso_7_else,
        intern_c_string ("coding-category-iso-7-else"));
  ASET (Vcoding_category_table, coding_category_iso_8_else,
        intern_c_string ("coding-category-iso-8-else"));
  ASET (Vcoding_category_table, coding_category_utf_8_auto,
        intern_c_string ("coding-category-utf-8-auto"));
  ASET (Vcoding_category_table, coding_category_utf_8_nosig,
        intern_c_string ("coding-category-utf-8"));
  ASET (Vcoding_category_table, coding_category_utf_8_sig,
        intern_c_string ("coding-category-utf-8-sig"));
  ASET (Vcoding_category_table, coding_category_utf_16_be,
        intern_c_string ("coding-category-utf-16-be"));
  ASET (Vcoding_category_table, coding_category_utf_16_auto,
        intern_c_string ("coding-category-utf-16-auto"));
  ASET (Vcoding_category_table, coding_category_utf_16_le,
        intern_c_string ("coding-category-utf-16-le"));
  ASET (Vcoding_category_table, coding_category_utf_16_be_nosig,
        intern_c_string ("coding-category-utf-16-be-nosig"));
  ASET (Vcoding_category_table, coding_category_utf_16_le_nosig,
        intern_c_string ("coding-category-utf-16-le-nosig"));
  ASET (Vcoding_category_table, coding_category_charset,
        intern_c_string ("coding-category-charset"));
  ASET (Vcoding_category_table, coding_category_sjis,
        intern_c_string ("coding-category-sjis"));
  ASET (Vcoding_category_table, coding_category_big5,
        intern_c_string ("coding-category-big5"));
  ASET (Vcoding_category_table, coding_category_ccl,
        intern_c_string ("coding-category-ccl"));
  ASET (Vcoding_category_table, coding_category_emacs_mule,
        intern_c_string ("coding-category-emacs-mule"));
  ASET (Vcoding_category_table, coding_category_raw_text,
        intern_c_string ("coding-category-raw-text"));
  ASET (Vcoding_category_table, coding_category_undecided,
        intern_c_string ("coding-category-undecided"));

  DEFSYM (Qcoding_system_define_form, "coding-system-define-form");

  defsubr (&Scoding_system_p);
  defsubr (&Scheck_coding_system);
  defsubr (&Sdefine_coding_system_internal);
  defsubr (&Scoding_system_put);
  defsubr (&Sdefine_coding_system_alias);
  defsubr (&Scoding_system_base);
  defsubr (&Scoding_system_eol_type);
  defsubr (&Sset_coding_system_priority);
  defsubr (&Sset_safe_terminal_coding_system_internal);

  DEFVAR_LISP ("coding-system-list", Vcoding_system_list,
        doc: /* List of coding systems.

Do not alter the value of this variable manually.  This variable should be
updated by the functions `define-coding-system' and
`define-coding-system-alias'.  */);
  Vcoding_system_list = Qnil;

  DEFVAR_LISP ("coding-system-alist", Vcoding_system_alist,
        doc: /* Alist of coding system names.
Each element is one element list of coding system name.
This variable is given to `completing-read' as COLLECTION argument.

Do not alter the value of this variable manually.  This variable should be
updated by `define-coding-system-alias'.  */);
  Vcoding_system_alist = Qnil;

  DEFVAR_LISP ("coding-category-list", Vcoding_category_list,
	       doc: /* List of coding-categories (symbols) ordered by priority.

On detecting a coding system, Emacs tries code detection algorithms
associated with each coding-category one by one in this order.  When
one algorithm agrees with a byte sequence of source text, the coding
system bound to the corresponding coding-category is selected.

Don't modify this variable directly, but use `set-coding-system-priority'.  */);
  {
    int i;

    Vcoding_category_list = Qnil;
    for (i = coding_category_max - 1; i >= 0; i--)
      Vcoding_category_list
        = Fcons (AREF (Vcoding_category_table, i), Vcoding_category_list);
  }

  DEFVAR_BOOL ("inhibit-eol-conversion", inhibit_eol_conversion,
        doc: /*
Non-nil means always inhibit code conversion of end-of-line format.
See info node `Coding Systems' and info node `Text and Binary' concerning
such conversion.  */);
  inhibit_eol_conversion = 0;

  DEFVAR_LISP ("default-process-coding-system",
        Vdefault_process_coding_system,
        doc: /* Cons of coding systems used for process I/O by default.
The car part is used for decoding a process output,
the cdr part is used for encoding a text to be sent to a process.  */);
  Vdefault_process_coding_system = Qnil;

  DEFVAR_LISP ("latin-extra-code-table", Vlatin_extra_code_table,
        doc: /*
Table of extra Latin codes in the range 128..159 (inclusive).
This is a vector of length 256.
If Nth element is non-nil, the existence of code N in a file
\(or output of subprocess) doesn't prevent it to be detected as
a coding system of ISO 2022 variant which has a flag
`accept-latin-extra-code' t (e.g. iso-latin-1) on reading a file
or reading output of a subprocess.
Only 128th through 159th elements have a meaning.  */);
  Vlatin_extra_code_table = make_nil_vector (256);

  Lisp_Object args[coding_arg_undecided_max];
  memclear (args, sizeof args);

  Lisp_Object plist[] = {
    QCname,
    args[coding_arg_name] = Qno_conversion,
    QCmnemonic,
    args[coding_arg_mnemonic] = make_fixnum ('='),
    intern_c_string (":coding-type"),
    args[coding_arg_coding_type] = Qraw_text,
    QCascii_compatible_p,
    args[coding_arg_ascii_compatible_p] = Qt,
    QCdefault_char,
    args[coding_arg_default_char] = make_fixnum (0),
    intern_c_string (":for-unibyte"),
    args[coding_arg_for_unibyte] = Qt,
    intern_c_string (":docstring"),
    (build_pure_c_string (
      "Do no conversion.\n"
      "\n"
      "When you visit a file with this coding, the file is read into a\n"
      "unibyte buffer as is, thus each byte of a file is treated as a\n"
      "character.")),
    intern_c_string (":eol-type"),
    args[coding_arg_eol_type] = Qunix,
  };
  args[coding_arg_plist] = CALLMANY (Flist, plist);
  Fdefine_coding_system_internal (coding_arg_max, args);

  plist[1] = args[coding_arg_name] = Qundecided;
  plist[3] = args[coding_arg_mnemonic] = make_fixnum ('-');
  plist[5] = args[coding_arg_coding_type] = Qundecided;
  plist[8] = intern_c_string (":charset-list");
  plist[9] = args[coding_arg_charset_list] = list1 (Qascii);
  plist[11] = args[coding_arg_for_unibyte] = Qnil;
  plist[13] = build_pure_c_string ("No conversion on encoding, "
                                   "automatic conversion on decoding.");
  plist[15] = args[coding_arg_eol_type] = Qnil;
  args[coding_arg_plist] = CALLMANY (Flist, plist);
  args[coding_arg_undecided_inhibit_null_byte_detection] = make_fixnum (0);
  args[coding_arg_undecided_inhibit_iso_escape_detection] = make_fixnum (0);
  Fdefine_coding_system_internal (coding_arg_undecided_max, args);

  setup_coding_system (Qno_conversion, &safe_terminal_coding);

  for (int i = 0; i < coding_category_max; i++)
    Fset (AREF (Vcoding_category_table, i), Qno_conversion);

  DEFSYM (Qdefine_coding_system_internal, "define-coding-system-internal");
}
