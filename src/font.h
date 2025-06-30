#ifndef EMACS_FONT_H
#define EMACS_FONT_H

#include "lisp.h"

enum font_property_index
{
  FONT_TYPE_INDEX,

  FONT_FOUNDRY_INDEX,

  FONT_FAMILY_INDEX,

  FONT_ADSTYLE_INDEX,

  FONT_REGISTRY_INDEX,

  FONT_WEIGHT_INDEX,

  FONT_SLANT_INDEX,

  FONT_WIDTH_INDEX,

  FONT_SIZE_INDEX,

  FONT_DPI_INDEX,

  FONT_SPACING_INDEX,

  FONT_AVGWIDTH_INDEX,

#if false

    FONT_STYLE_INDEX,

    FONT_METRICS_INDEX,
#endif

  FONT_EXTRA_INDEX, /* alist		alist */

  FONT_SPEC_MAX,

  FONT_OBJLIST_INDEX = FONT_SPEC_MAX,

  FONT_ENTITY_MAX,

  FONT_NAME_INDEX = FONT_ENTITY_MAX,

  FONT_FULLNAME_INDEX,

  FONT_FILE_INDEX,

  FONT_OBJECT_MAX
};

#define FONT_WEIGHT_NAME_NUMERIC(name) \
  (font_style_to_value (FONT_WEIGHT_INDEX, name, false) >> 8)
#define FONT_SLANT_NAME_NUMERIC(name) \
  (font_style_to_value (FONT_SLANT_INDEX, name, false) >> 8)
#define FONT_WIDTH_NAME_NUMERIC(name) \
  (font_style_to_value (FONT_WIDTH_INDEX, name, false) >> 8)

INLINE bool
FONTP (Lisp_Object x)
{
  return PSEUDOVECTORP (x, PVEC_FONT);
}

extern int font_style_to_value (enum font_property_index prop, Lisp_Object name,
                                bool noerror);
extern void font_clear_prop (Lisp_Object *attrs, enum font_property_index prop);
extern void font_update_sort_order (int *order);

#endif
