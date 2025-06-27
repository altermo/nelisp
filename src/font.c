#include "font.h"
#include "lisp.h"

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
}
