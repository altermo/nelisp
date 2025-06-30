#include "font.h"
#include "lisp.h"
#include "dispextern.h"

static Lisp_Object font_style_table;

struct table_entry
{
  int numeric;
  const char *names[6];
};

static const struct table_entry weight_table[]
  = { { 0, { "thin" } },
      { 40, { "ultra-light", "ultralight", "extra-light", "extralight" } },
      { 50, { "light" } },
      { 55, { "semi-light", "semilight", "demilight" } },
      { 80, { "regular", "normal", "unspecified", "book" } },
      { 100, { "medium" } },
      { 180, { "semi-bold", "semibold", "demibold", "demi-bold", "demi" } },
      { 200, { "bold" } },
      { 205, { "extra-bold", "extrabold", "ultra-bold", "ultrabold" } },
      { 210, { "black", "heavy" } },
      { 250, { "ultra-heavy", "ultraheavy" } } };

static const struct table_entry slant_table[]
  = { { 0, { "reverse-oblique", "ro" } },
      { 10, { "reverse-italic", "ri" } },
      { 100, { "normal", "r", "unspecified" } },
      { 200, { "italic", "i", "ot" } },
      { 210, { "oblique", "o" } } };

static const struct table_entry width_table[]
  = { { 50, { "ultra-condensed", "ultracondensed" } },
      { 63, { "extra-condensed", "extracondensed" } },
      { 75, { "condensed", "compressed", "narrow" } },
      { 87, { "semi-condensed", "semicondensed", "demicondensed" } },
      { 100, { "normal", "medium", "regular", "unspecified" } },
      { 113, { "semi-expanded", "semiexpanded", "demiexpanded" } },
      { 125, { "expanded" } },
      { 150, { "extra-expanded", "extraexpanded" } },
      { 200, { "ultra-expanded", "ultraexpanded", "wide" } } };

enum xlfd_field_index
{
  XLFD_FOUNDRY_INDEX,
  XLFD_FAMILY_INDEX,
  XLFD_WEIGHT_INDEX,
  XLFD_SLANT_INDEX,
  XLFD_SWIDTH_INDEX,
  XLFD_ADSTYLE_INDEX,
  XLFD_PIXEL_INDEX,
  XLFD_POINT_INDEX,
  XLFD_RESX_INDEX,
  XLFD_RESY_INDEX,
  XLFD_SPACING_INDEX,
  XLFD_AVGWIDTH_INDEX,
  XLFD_REGISTRY_INDEX,
  XLFD_ENCODING_INDEX,
  XLFD_LAST_INDEX
};

Lisp_Object
font_style_symbolic (Lisp_Object font, enum font_property_index prop,
                     bool for_face)
{
  Lisp_Object val = AREF (font, prop);
  Lisp_Object table, elt;
  int i;

  if (NILP (val))
    return Qnil;
  table = AREF (font_style_table, prop - FONT_WEIGHT_INDEX);
  CHECK_VECTOR (table);
  i = XFIXNUM (val) & 0xFF;
  eassert (((i >> 4) & 0xF) < ASIZE (table));
  elt = AREF (table, ((i >> 4) & 0xF));
  CHECK_VECTOR (elt);
  eassert ((i & 0xF) + 1 < ASIZE (elt));
  elt = (for_face ? AREF (elt, 1) : AREF (elt, (i & 0xF) + 1));
  CHECK_SYMBOL (elt);
  return elt;
}

int
font_style_to_value (enum font_property_index prop, Lisp_Object val,
                     bool noerror)
{
  Lisp_Object table = AREF (font_style_table, prop - FONT_WEIGHT_INDEX);
  int len;

  CHECK_VECTOR (table);
  len = ASIZE (table);

  if (SYMBOLP (val))
    {
      int i, j;
      char *s;
      Lisp_Object elt;

      for (i = 0; i < len; i++)
        {
          CHECK_VECTOR (AREF (table, i));
          for (j = 1; j < ASIZE (AREF (table, i)); j++)
            if (EQ (val, AREF (AREF (table, i), j)))
              {
                CHECK_FIXNUM (AREF (AREF (table, i), 0));
                return ((XFIXNUM (AREF (AREF (table, i), 0)) << 8) | (i << 4)
                        | (j - 1));
              }
        }
      TODO;
    }
  else
    {
      int i, last_n;
      EMACS_INT numeric = XFIXNUM (val);

      for (i = 0, last_n = -1; i < len; i++)
        {
          int n;

          CHECK_VECTOR (AREF (table, i));
          CHECK_FIXNUM (AREF (AREF (table, i), 0));
          n = XFIXNUM (AREF (AREF (table, i), 0));
          if (numeric == n)
            return (n << 8) | (i << 4);
          if (numeric < n)
            {
              if (!noerror)
                return -1;
              return ((i == 0 || n - numeric < numeric - last_n)
                        ? (n << 8) | (i << 4)
                        : (last_n << 8 | ((i - 1) << 4)));
            }
          last_n = n;
        }
      if (!noerror)
        return -1;
      return ((last_n << 8) | ((i - 1) << 4));
    }
}

void
font_clear_prop (Lisp_Object *attrs, enum font_property_index prop)
{
  Lisp_Object font = attrs[LFACE_FONT_INDEX];

  if (!FONTP (font))
    return;

  TODO;
}

static int sort_shift_bits[FONT_SIZE_INDEX + 1];
void
font_update_sort_order (int *order)
{
  int i, shift_bits;

  for (i = 0, shift_bits = 23; i < 4; i++, shift_bits -= 7)
    {
      int xlfd_idx = order[i];

      if (xlfd_idx == XLFD_WEIGHT_INDEX)
        sort_shift_bits[FONT_WEIGHT_INDEX] = shift_bits;
      else if (xlfd_idx == XLFD_SLANT_INDEX)
        sort_shift_bits[FONT_SLANT_INDEX] = shift_bits;
      else if (xlfd_idx == XLFD_SWIDTH_INDEX)
        sort_shift_bits[FONT_WIDTH_INDEX] = shift_bits;
      else
        sort_shift_bits[FONT_SIZE_INDEX] = shift_bits;
    }
}

#define BUILD_STYLE_TABLE(TBL) build_style_table (TBL, ARRAYELTS (TBL))

static Lisp_Object
build_style_table (const struct table_entry *entry, int nelement)
{
  Lisp_Object table = make_nil_vector (nelement);
  for (int i = 0; i < nelement; i++)
    {
      int j;
      for (j = 0; entry[i].names[j]; j++)
        continue;
      Lisp_Object elt = make_nil_vector (j + 1);
      ASET (elt, 0, make_fixnum (entry[i].numeric));
      for (j = 0; entry[i].names[j]; j++)
        ASET (elt, j + 1, intern_c_string (entry[i].names[j]));
      ASET (table, i, elt);
    }
  return table;
}

void
syms_of_font (void)
{
  sort_shift_bits[FONT_TYPE_INDEX] = 0;
  sort_shift_bits[FONT_SLANT_INDEX] = 2;
  sort_shift_bits[FONT_WEIGHT_INDEX] = 9;
  sort_shift_bits[FONT_SIZE_INDEX] = 16;
  sort_shift_bits[FONT_WIDTH_INDEX] = 23;

  DEFSYM (Qopentype, "opentype");

  DEFSYM (Qcanonical_combining_class, "canonical-combining-class");

  DEFSYM (Qascii_0, "ascii-0");
  DEFSYM (Qiso8859_1, "iso8859-1");
  DEFSYM (Qiso10646_1, "iso10646-1");
  DEFSYM (Qunicode_bmp, "unicode-bmp");
  DEFSYM (Qemoji, "emoji");

  DEFSYM (QCotf, ":otf");
  DEFSYM (QClang, ":lang");
  DEFSYM (QCscript, ":script");
  DEFSYM (QCantialias, ":antialias");
  DEFSYM (QCfoundry, ":foundry");
  DEFSYM (QCadstyle, ":adstyle");
  DEFSYM (QCregistry, ":registry");
  DEFSYM (QCspacing, ":spacing");
  DEFSYM (QCdpi, ":dpi");
  DEFSYM (QCscalable, ":scalable");
  DEFSYM (QCavgwidth, ":avgwidth");
  DEFSYM (QCfont_entity, ":font-entity");
  DEFSYM (QCcombining_capability, ":combining-capability");

  DEFSYM (Qc, "c");
  DEFSYM (Qm, "m");
  DEFSYM (Qp, "p");
  DEFSYM (Qd, "d");

  DEFSYM (Qja, "ja");
  DEFSYM (Qko, "ko");

  DEFSYM (QCuser_spec, ":user-spec");

  DEFSYM (QL2R, "L2R");
  DEFSYM (QR2L, "R2L");

  DEFSYM (Qfont_extra_type, "font-extra-type");
  DEFSYM (Qfont_driver_superseded_by, "font-driver-superseded-by");

  DEFVAR_LISP_NOPRO ("font-weight-table", Vfont_weight_table,
        doc: /*  Vector of valid font weight values.
Each element has the form:
    [NUMERIC-VALUE SYMBOLIC-NAME ALIAS-NAME ...]
NUMERIC-VALUE is an integer, and SYMBOLIC-NAME and ALIAS-NAME are symbols.
This variable cannot be set; trying to do so will signal an error.  */);
  Vfont_weight_table = BUILD_STYLE_TABLE (weight_table);
  make_symbol_constant (intern_c_string ("font-weight-table"));

  DEFVAR_LISP_NOPRO ("font-slant-table", Vfont_slant_table,
        doc: /*  Vector of font slant symbols vs the corresponding numeric values.
See `font-weight-table' for the format of the vector.
This variable cannot be set; trying to do so will signal an error.  */);
  Vfont_slant_table = BUILD_STYLE_TABLE (slant_table);
  make_symbol_constant (intern_c_string ("font-slant-table"));

  DEFVAR_LISP_NOPRO ("font-width-table", Vfont_width_table,
        doc: /*  Alist of font width symbols vs the corresponding numeric values.
See `font-weight-table' for the format of the vector.
This variable cannot be set; trying to do so will signal an error.  */);
  Vfont_width_table = BUILD_STYLE_TABLE (width_table);
  make_symbol_constant (intern_c_string ("font-width-table"));

  staticpro (&font_style_table);
  font_style_table
    = CALLN (Fvector, Vfont_weight_table, Vfont_slant_table, Vfont_width_table);
}
