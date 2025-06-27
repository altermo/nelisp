#include "lisp.h"
#include "dispextern.h"
#include "font.h"
#include "frame.h"

Lisp_Object Vface_alternative_font_family_alist;
Lisp_Object Vface_alternative_font_registry_alist;

static int next_lface_id;

static Lisp_Object *lface_id_to_name;
static ptrdiff_t lface_id_to_name_size;

enum xlfd_field
{
  XLFD_FOUNDRY,
  XLFD_FAMILY,
  XLFD_WEIGHT,
  XLFD_SLANT,
  XLFD_SWIDTH,
  XLFD_ADSTYLE,
  XLFD_PIXEL_SIZE,
  XLFD_POINT_SIZE,
  XLFD_RESX,
  XLFD_RESY,
  XLFD_SPACING,
  XLFD_AVGWIDTH,
  XLFD_REGISTRY,
  XLFD_ENCODING,
  XLFD_LAST
};

static int font_sort_order[4];

#define LFACEP(LFACE)                                    \
  (VECTORP (LFACE) && ASIZE (LFACE) == LFACE_VECTOR_SIZE \
   && EQ (AREF (LFACE, 0), Qface))

#define check_lface(lface) (void) 0

static Lisp_Object
resolve_face_name (Lisp_Object face_name, bool signal_p)
{
  Lisp_Object orig_face;
  Lisp_Object tortoise, hare;

  if (STRINGP (face_name))
    face_name = Fintern (face_name, Qnil);

  if (NILP (face_name) || !SYMBOLP (face_name))
    return face_name;

  orig_face = face_name;
  tortoise = hare = face_name;

  while (true)
    {
      face_name = hare;
      hare = Fget (hare, Qface_alias);
      if (NILP (hare) || !SYMBOLP (hare))
        break;

      face_name = hare;
      hare = Fget (hare, Qface_alias);
      if (NILP (hare) || !SYMBOLP (hare))
        break;

      tortoise = Fget (tortoise, Qface_alias);
      if (BASE_EQ (hare, tortoise))
        {
          if (signal_p)
            circular_list (orig_face);
          return Qdefault;
        }
    }

  return face_name;
}

static Lisp_Object
lface_from_face_name_no_resolve (struct frame *f, Lisp_Object face_name,
                                 bool signal_p)
{
  Lisp_Object lface;

  if (f)
    lface = Fgethash (face_name, f->face_hash_table, Qnil);
  else
    lface = CDR (Fgethash (face_name, Vface_new_frame_defaults, Qnil));

  if (signal_p && NILP (lface))
    signal_error ("Invalid face", face_name);

  check_lface (lface);

  return lface;
}

static Lisp_Object
lface_from_face_name (struct frame *f, Lisp_Object face_name, bool signal_p)
{
  face_name = resolve_face_name (face_name, signal_p);
  return lface_from_face_name_no_resolve (f, face_name, signal_p);
}

DEFUN ("internal-make-lisp-face", Finternal_make_lisp_face,
       Sinternal_make_lisp_face, 1, 2, 0,
       doc: /* Make FACE, a symbol, a Lisp face with all attributes nil.
If FACE was not known as a face before, create a new one.
If optional argument FRAME is specified, make a frame-local face
for that frame.  Otherwise operate on the global face definition.
Value is a vector of face attributes.  */)
(Lisp_Object face, Lisp_Object frame)
{
  Lisp_Object global_lface, lface;
  struct frame *f;
  int i;

  CHECK_SYMBOL (face);
  global_lface = lface_from_face_name (NULL, face, false);

  if (!NILP (frame))
    {
      CHECK_LIVE_FRAME (frame);
      f = XFRAME (frame);
      lface = lface_from_face_name (f, face, false);
    }
  else
    f = NULL, lface = Qnil;

  if (NILP (global_lface))
    {
      if (next_lface_id == lface_id_to_name_size)
        lface_id_to_name = xpalloc (lface_id_to_name, &lface_id_to_name_size, 1,
                                    MAX_FACE_ID, sizeof *lface_id_to_name);

      Lisp_Object face_id = make_fixnum (next_lface_id);
      lface_id_to_name[next_lface_id] = face;
      Fput (face, Qface, face_id);
      ++next_lface_id;

      global_lface = make_vector (LFACE_VECTOR_SIZE, Qunspecified);
      ASET (global_lface, 0, Qface);
      Fputhash (face, Fcons (face_id, global_lface), Vface_new_frame_defaults);
    }
  else if (f == NULL)
    for (i = 1; i < LFACE_VECTOR_SIZE; ++i)
      ASET (global_lface, i, Qunspecified);

  if (f)
    {
      if (NILP (lface))
        {
          lface = make_vector (LFACE_VECTOR_SIZE, Qunspecified);
          ASET (lface, 0, Qface);
          Fputhash (face, lface, f->face_hash_table);
        }
      else
        for (i = 1; i < LFACE_VECTOR_SIZE; ++i)
          ASET (lface, i, Qunspecified);
    }
  else
    lface = global_lface;

  if (NILP (Fget (face, Qface_no_inherit)))
    {
      TODO_NELISP_LATER;
    }

  eassert (LFACEP (lface));
  check_lface (lface);
  return lface;
}

DEFUN ("internal-lisp-face-p", Finternal_lisp_face_p,
       Sinternal_lisp_face_p, 1, 2, 0,
       doc: /* Return non-nil if FACE names a face.
FACE should be a symbol or string.
If optional second argument FRAME is non-nil, check for the
existence of a frame-local face with name FACE on that frame.
Otherwise check for the existence of a global face.  */)
(Lisp_Object face, Lisp_Object frame)
{
  Lisp_Object lface;

  face = resolve_face_name (face, true);

  if (!NILP (frame))
    {
      TODO; // CHECK_LIVE_FRAME (frame);
      // lface = lface_from_face_name (XFRAME (frame), face, false);
    }
  else
    lface = lface_from_face_name (NULL, face, false);

  return lface;
}

void
free_all_realized_faces (Lisp_Object frame)
{
  TODO_NELISP_LATER;
}

DEFUN ("internal-set-font-selection-order",
       Finternal_set_font_selection_order,
       Sinternal_set_font_selection_order, 1, 1, 0,
       doc: /* Set font selection order for face font selection to ORDER.
ORDER must be a list of length 4 containing the symbols `:width',
`:height', `:weight', and `:slant'.  Face attributes appearing
first in ORDER are matched first, e.g. if `:height' appears before
`:weight' in ORDER, font selection first tries to find a font with
a suitable height, and then tries to match the font weight.
Value is ORDER.  */)
(Lisp_Object order)
{
  Lisp_Object list;
  int i;
  int indices[ARRAYELTS (font_sort_order)];

  CHECK_LIST (order);
  memset (indices, 0, sizeof indices);
  i = 0;

  for (list = order; CONSP (list) && i < ARRAYELTS (indices);
       list = XCDR (list), ++i)
    {
      Lisp_Object attr = XCAR (list);
      int xlfd;

      if (EQ (attr, QCwidth))
        xlfd = XLFD_SWIDTH;
      else if (EQ (attr, QCheight))
        xlfd = XLFD_POINT_SIZE;
      else if (EQ (attr, QCweight))
        xlfd = XLFD_WEIGHT;
      else if (EQ (attr, QCslant))
        xlfd = XLFD_SLANT;
      else
        break;

      if (indices[i] != 0)
        break;
      indices[i] = xlfd;
    }

  if (!NILP (list) || i != ARRAYELTS (indices))
    signal_error ("Invalid font sort order", order);
  for (i = 0; i < ARRAYELTS (font_sort_order); ++i)
    if (indices[i] == 0)
      signal_error ("Invalid font sort order", order);

  if (memcmp (indices, font_sort_order, sizeof indices) != 0)
    {
      memcpy (font_sort_order, indices, sizeof font_sort_order);
      free_all_realized_faces (Qnil);
    }

  font_update_sort_order (font_sort_order);

  return Qnil;
}

DEFUN ("internal-set-alternative-font-family-alist",
       Finternal_set_alternative_font_family_alist,
       Sinternal_set_alternative_font_family_alist, 1, 1, 0,
       doc: /* Define alternative font families to try in face font selection.
ALIST is an alist of (FAMILY ALTERNATIVE1 ALTERNATIVE2 ...) entries.
Each ALTERNATIVE is tried in order if no fonts of font family FAMILY can
be found.  Value is ALIST.  */)
(Lisp_Object alist)
{
  Lisp_Object entry, tail, tail2;

  CHECK_LIST (alist);
  alist = Fcopy_sequence (alist);
  for (tail = alist; CONSP (tail); tail = XCDR (tail))
    {
      entry = XCAR (tail);
      CHECK_LIST (entry);
      entry = Fcopy_sequence (entry);
      XSETCAR (tail, entry);
      for (tail2 = entry; CONSP (tail2); tail2 = XCDR (tail2))
        XSETCAR (tail2, Fintern (XCAR (tail2), Qnil));
    }

  Vface_alternative_font_family_alist = alist;
  free_all_realized_faces (Qnil);
  return alist;
}

DEFUN ("internal-set-alternative-font-registry-alist",
       Finternal_set_alternative_font_registry_alist,
       Sinternal_set_alternative_font_registry_alist, 1, 1, 0,
       doc: /* Define alternative font registries to try in face font selection.
ALIST is an alist of (REGISTRY ALTERNATIVE1 ALTERNATIVE2 ...) entries.
Each ALTERNATIVE is tried in order if no fonts of font registry REGISTRY can
be found.  Value is ALIST.  */)
(Lisp_Object alist)
{
  Lisp_Object entry, tail, tail2;

  CHECK_LIST (alist);
  alist = Fcopy_sequence (alist);
  for (tail = alist; CONSP (tail); tail = XCDR (tail))
    {
      entry = XCAR (tail);
      CHECK_LIST (entry);
      entry = Fcopy_sequence (entry);
      XSETCAR (tail, entry);
      for (tail2 = entry; CONSP (tail2); tail2 = XCDR (tail2))
        XSETCAR (tail2, Fdowncase (XCAR (tail2)));
    }
  Vface_alternative_font_registry_alist = alist;
  free_all_realized_faces (Qnil);
  return alist;
}

void
syms_of_xfaces (void)
{
  DEFSYM (Qface, "face");

  DEFSYM (Qface_no_inherit, "face-no-inherit");

  DEFSYM (Qbitmap_spec_p, "bitmap-spec-p");

  DEFSYM (Qframe_set_background_mode, "frame-set-background-mode");

  DEFSYM (QCfamily, ":family");
  DEFSYM (QCheight, ":height");
  DEFSYM (QCweight, ":weight");
  DEFSYM (QCslant, ":slant");
  DEFSYM (QCunderline, ":underline");
  DEFSYM (QCinverse_video, ":inverse-video");
  DEFSYM (QCreverse_video, ":reverse-video");
  DEFSYM (QCforeground, ":foreground");
  DEFSYM (QCbackground, ":background");
  DEFSYM (QCstipple, ":stipple");
  DEFSYM (QCwidth, ":width");
  DEFSYM (QCfont, ":font");
  DEFSYM (QCfontset, ":fontset");
  DEFSYM (QCdistant_foreground, ":distant-foreground");
  DEFSYM (QCbold, ":bold");
  DEFSYM (QCitalic, ":italic");
  DEFSYM (QCoverline, ":overline");
  DEFSYM (QCstrike_through, ":strike-through");
  DEFSYM (QCbox, ":box");
  DEFSYM (QCinherit, ":inherit");
  DEFSYM (QCextend, ":extend");

  DEFSYM (QCcolor, ":color");
  DEFSYM (QCline_width, ":line-width");
  DEFSYM (QCstyle, ":style");
  DEFSYM (QCposition, ":position");
  DEFSYM (Qline, "line");
  DEFSYM (Qwave, "wave");
  DEFSYM (Qdouble_line, "double-line");
  DEFSYM (Qdots, "dots");
  DEFSYM (Qdashes, "dashes");
  DEFSYM (Qreleased_button, "released-button");
  DEFSYM (Qpressed_button, "pressed-button");
  DEFSYM (Qflat_button, "flat-button");
  DEFSYM (Qnormal, "normal");
  DEFSYM (Qthin, "thin");
  DEFSYM (Qextra_light, "extra-light");
  DEFSYM (Qultra_light, "ultra-light");
  DEFSYM (Qlight, "light");
  DEFSYM (Qsemi_light, "semi-light");
  DEFSYM (Qmedium, "medium");
  DEFSYM (Qsemi_bold, "semi-bold");
  DEFSYM (Qbook, "book");
  DEFSYM (Qbold, "bold");
  DEFSYM (Qextra_bold, "extra-bold");
  DEFSYM (Qultra_bold, "ultra-bold");
  DEFSYM (Qheavy, "heavy");
  DEFSYM (Qultra_heavy, "ultra-heavy");
  DEFSYM (Qblack, "black");
  DEFSYM (Qoblique, "oblique");
  DEFSYM (Qitalic, "italic");
  DEFSYM (Qreset, "reset");

  DEFSYM (Qbackground_color, "background-color");
  DEFSYM (Qforeground_color, "foreground-color");

  DEFSYM (Qunspecified, "unspecified");
  DEFSYM (QCignore_defface, ":ignore-defface");

  DEFSYM (QCwindow, ":window");
  DEFSYM (QCfiltered, ":filtered");

  DEFSYM (Qface_alias, "face-alias");

  DEFSYM (Qdefault, "default");
  DEFSYM (Qtool_bar, "tool-bar");
  DEFSYM (Qtab_bar, "tab-bar");
  DEFSYM (Qfringe, "fringe");
  DEFSYM (Qtab_line, "tab-line");
  DEFSYM (Qheader_line, "header-line");
  DEFSYM (Qscroll_bar, "scroll-bar");
  DEFSYM (Qmenu, "menu");
  DEFSYM (Qcursor, "cursor");
  DEFSYM (Qborder, "border");
  DEFSYM (Qmouse, "mouse");
  DEFSYM (Qmode_line_inactive, "mode-line-inactive");
  DEFSYM (Qmode_line_active, "mode-line-active");
  DEFSYM (Qvertical_border, "vertical-border");
  DEFSYM (Qwindow_divider, "window-divider");
  DEFSYM (Qwindow_divider_first_pixel, "window-divider-first-pixel");
  DEFSYM (Qwindow_divider_last_pixel, "window-divider-last-pixel");
  DEFSYM (Qinternal_border, "internal-border");
  DEFSYM (Qchild_frame_border, "child-frame-border");

  DEFSYM (Qtty_color_desc, "tty-color-desc");
  DEFSYM (Qtty_color_standard_values, "tty-color-standard-values");
  DEFSYM (Qtty_color_by_index, "tty-color-by-index");

  DEFSYM (Qtty_color_alist, "tty-color-alist");
  DEFSYM (Qtty_defined_color_alist, "tty-defined-color-alist");

  Vface_alternative_font_family_alist = Qnil;
  staticpro (&Vface_alternative_font_family_alist);
  Vface_alternative_font_registry_alist = Qnil;
  staticpro (&Vface_alternative_font_registry_alist);

  defsubr (&Sinternal_make_lisp_face);
  defsubr (&Sinternal_lisp_face_p);
  defsubr (&Sinternal_set_font_selection_order);
  defsubr (&Sinternal_set_alternative_font_family_alist);
  defsubr (&Sinternal_set_alternative_font_registry_alist);

  DEFVAR_LISP ("face--new-frame-defaults", Vface_new_frame_defaults,
    doc: /* Hash table of global face definitions (for internal use only.)  */);
  Vface_new_frame_defaults
    = make_hash_table (&hashtest_eq, 33, Weak_None, false);
}
