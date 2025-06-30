#include "lisp.h"
#include "dispextern.h"
#include "font.h"
#include "frame.h"

#define UNSPECIFIEDP(ATTR) EQ (ATTR, Qunspecified)
#define IGNORE_DEFFACE_P(ATTR) EQ (ATTR, QCignore_defface)
#define RESET_P(ATTR) EQ (ATTR, Qreset)

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

#define LFACE_FAMILY(LFACE) AREF (LFACE, LFACE_FAMILY_INDEX)
#define LFACE_FOUNDRY(LFACE) AREF (LFACE, LFACE_FOUNDRY_INDEX)
#define LFACE_HEIGHT(LFACE) AREF (LFACE, LFACE_HEIGHT_INDEX)
#define LFACE_WEIGHT(LFACE) AREF (LFACE, LFACE_WEIGHT_INDEX)
#define LFACE_SLANT(LFACE) AREF (LFACE, LFACE_SLANT_INDEX)
#define LFACE_UNDERLINE(LFACE) AREF (LFACE, LFACE_UNDERLINE_INDEX)
#define LFACE_INVERSE(LFACE) AREF (LFACE, LFACE_INVERSE_INDEX)
#define LFACE_FOREGROUND(LFACE) AREF (LFACE, LFACE_FOREGROUND_INDEX)
#define LFACE_BACKGROUND(LFACE) AREF (LFACE, LFACE_BACKGROUND_INDEX)
#define LFACE_STIPPLE(LFACE) AREF (LFACE, LFACE_STIPPLE_INDEX)
#define LFACE_SWIDTH(LFACE) AREF (LFACE, LFACE_SWIDTH_INDEX)
#define LFACE_OVERLINE(LFACE) AREF (LFACE, LFACE_OVERLINE_INDEX)
#define LFACE_STRIKE_THROUGH(LFACE) AREF (LFACE, LFACE_STRIKE_THROUGH_INDEX)
#define LFACE_BOX(LFACE) AREF (LFACE, LFACE_BOX_INDEX)
#define LFACE_FONT(LFACE) AREF (LFACE, LFACE_FONT_INDEX)
#define LFACE_INHERIT(LFACE) AREF (LFACE, LFACE_INHERIT_INDEX)
#define LFACE_FONTSET(LFACE) AREF (LFACE, LFACE_FONTSET_INDEX)
#define LFACE_EXTEND(LFACE) AREF (LFACE, LFACE_EXTEND_INDEX)
#define LFACE_DISTANT_FOREGROUND(LFACE) \
  AREF (LFACE, LFACE_DISTANT_FOREGROUND_INDEX)

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

static Lisp_Object
merge_face_heights (Lisp_Object from, Lisp_Object to, Lisp_Object invalid)
{
  Lisp_Object result = invalid;

  if (FIXNUMP (from))
    result = from;
  else if (FLOATP (from))
    {
      if (FIXNUMP (to))
        result = make_fixnum (XFLOAT_DATA (from) * XFIXNUM (to));
      else if (FLOATP (to))
        result = make_float (XFLOAT_DATA (from) * XFLOAT_DATA (to));
      else if (UNSPECIFIEDP (to))
        result = from;
    }
  else if (FUNCTIONP (from))
    {
      result = safe_calln (from, to);

      if (FIXNUMP (to) && !FIXNUMP (result))
        result = invalid;
    }

  return result;
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

#define HANDLE_INVALID_NIL_VALUE(A, F) \
  if (NILP (value))                    \
    {                                  \
      TODO;                            \
    }

DEFUN ("internal-set-lisp-face-attribute", Finternal_set_lisp_face_attribute,
       Sinternal_set_lisp_face_attribute, 3, 4, 0,
       doc: /* Set attribute ATTR of FACE to VALUE.
FRAME being a frame means change the face on that frame.
FRAME nil means change the face of the selected frame.
FRAME t means change the default for new frames.
FRAME 0 means change the face on all frames, and change the default
  for new frames.  */)
(Lisp_Object face, Lisp_Object attr, Lisp_Object value, Lisp_Object frame)
{
  Lisp_Object lface;
  Lisp_Object old_value = Qnil;

  enum font_property_index prop_index = 0;
  struct frame *f;

  CHECK_SYMBOL (face);
  CHECK_SYMBOL (attr);

  face = resolve_face_name (face, true);

  if (FIXNUMP (frame) && XFIXNUM (frame) == 0)
    {
      TODO;
    }

  if (EQ (frame, Qt))
    {
      TODO;
    }
  else
    {
      if (NILP (frame))
        frame = selected_frame;

      CHECK_LIVE_FRAME (frame);
      f = XFRAME (frame);

      lface = lface_from_face_name (f, face, false);

      if (NILP (lface))
        lface = Finternal_make_lisp_face (face, frame);
    }

  if (EQ (attr, QCfamily))
    {
      if (!UNSPECIFIEDP (value) && !IGNORE_DEFFACE_P (value)
          && !RESET_P (value))
        {
          CHECK_STRING (value);
          if (SCHARS (value) == 0)
            signal_error ("Invalid face family", value);
        }
      old_value = LFACE_FAMILY (lface);
      ASET (lface, LFACE_FAMILY_INDEX, value);
      prop_index = FONT_FAMILY_INDEX;
    }
  else if (EQ (attr, QCfoundry))
    {
      if (!UNSPECIFIEDP (value) && !IGNORE_DEFFACE_P (value)
          && !RESET_P (value))
        {
          CHECK_STRING (value);
          if (SCHARS (value) == 0)
            signal_error ("Invalid face foundry", value);
        }
      old_value = LFACE_FOUNDRY (lface);
      ASET (lface, LFACE_FOUNDRY_INDEX, value);
      prop_index = FONT_FOUNDRY_INDEX;
    }
  else if (EQ (attr, QCheight))
    {
      if (!UNSPECIFIEDP (value) && !IGNORE_DEFFACE_P (value)
          && !RESET_P (value))
        {
          if (EQ (face, Qdefault))
            {
              if (!FIXNUMP (value) || XFIXNUM (value) <= 0)
                signal_error ("Default face height not absolute and positive",
                              value);
            }
          else
            {
              Lisp_Object test
                = merge_face_heights (value, make_fixnum (10), Qnil);
              if (!FIXNUMP (test) || XFIXNUM (test) <= 0)
                signal_error ("Face height does not produce a positive integer",
                              value);
            }
        }

      old_value = LFACE_HEIGHT (lface);
      ASET (lface, LFACE_HEIGHT_INDEX, value);
      prop_index = FONT_SIZE_INDEX;
    }
  else if (EQ (attr, QCweight))
    {
      if (!UNSPECIFIEDP (value) && !IGNORE_DEFFACE_P (value)
          && !RESET_P (value))
        {
          CHECK_SYMBOL (value);
          if (FONT_WEIGHT_NAME_NUMERIC (value) < 0)
            signal_error ("Invalid face weight", value);
        }
      old_value = LFACE_WEIGHT (lface);
      ASET (lface, LFACE_WEIGHT_INDEX, value);
      prop_index = FONT_WEIGHT_INDEX;
    }
  else if (EQ (attr, QCslant))
    {
      if (!UNSPECIFIEDP (value) && !IGNORE_DEFFACE_P (value)
          && !RESET_P (value))
        {
          CHECK_SYMBOL (value);
          if (FONT_SLANT_NAME_NUMERIC (value) < 0)
            signal_error ("Invalid face slant", value);
        }
      old_value = LFACE_SLANT (lface);
      ASET (lface, LFACE_SLANT_INDEX, value);
      prop_index = FONT_SLANT_INDEX;
    }
  else if (EQ (attr, QCunderline))
    {
      bool valid_p = false;

      if (UNSPECIFIEDP (value) || IGNORE_DEFFACE_P (value) || RESET_P (value))
        valid_p = true;
      else if (NILP (value) || EQ (value, Qt))
        valid_p = true;
      else if (STRINGP (value) && SCHARS (value) > 0)
        valid_p = true;
      else if (CONSP (value))
        {
          Lisp_Object key, val, list;

          list = value;
          valid_p = true;

          while (!NILP (CAR_SAFE (list)))
            {
              key = CAR_SAFE (list);
              list = CDR_SAFE (list);
              val = CAR_SAFE (list);
              list = CDR_SAFE (list);

              if (NILP (key) || (NILP (val) && !EQ (key, QCposition)))
                {
                  valid_p = false;
                  break;
                }

              else if (EQ (key, QCcolor)
                       && !(EQ (val, Qforeground_color)
                            || (STRINGP (val) && SCHARS (val) > 0)))
                {
                  valid_p = false;
                  break;
                }

              else if (EQ (key, QCstyle)
                       && !(EQ (val, Qline) || EQ (val, Qdouble_line)
                            || EQ (val, Qwave) || EQ (val, Qdots)
                            || EQ (val, Qdashes)))
                {
                  valid_p = false;
                  break;
                }
            }
        }

      if (!valid_p)
        signal_error ("Invalid face underline", value);

      old_value = LFACE_UNDERLINE (lface);
      ASET (lface, LFACE_UNDERLINE_INDEX, value);
    }
  else if (EQ (attr, QCoverline))
    {
      if (!UNSPECIFIEDP (value) && !IGNORE_DEFFACE_P (value)
          && !RESET_P (value))
        if ((SYMBOLP (value) && !EQ (value, Qt) && !NILP (value))
            || (STRINGP (value) && SCHARS (value) == 0))
          signal_error ("Invalid face overline", value);

      old_value = LFACE_OVERLINE (lface);
      ASET (lface, LFACE_OVERLINE_INDEX, value);
    }
  else if (EQ (attr, QCstrike_through))
    {
      if (!UNSPECIFIEDP (value) && !IGNORE_DEFFACE_P (value)
          && !RESET_P (value))
        if ((SYMBOLP (value) && !EQ (value, Qt) && !NILP (value))
            || (STRINGP (value) && SCHARS (value) == 0))
          signal_error ("Invalid face strike-through", value);

      old_value = LFACE_STRIKE_THROUGH (lface);
      ASET (lface, LFACE_STRIKE_THROUGH_INDEX, value);
    }
  else if (EQ (attr, QCbox))
    {
      bool valid_p;

      if (EQ (value, Qt))
        value = make_fixnum (1);

      if (UNSPECIFIEDP (value) || IGNORE_DEFFACE_P (value) || RESET_P (value))
        valid_p = true;
      else if (NILP (value))
        valid_p = true;
      else if (FIXNUMP (value))
        valid_p = XFIXNUM (value) != 0;
      else if (STRINGP (value))
        valid_p = SCHARS (value) > 0;
      else if (CONSP (value) && FIXNUMP (XCAR (value))
               && FIXNUMP (XCDR (value)))
        valid_p = true;
      else if (CONSP (value))
        {
          Lisp_Object tem;

          tem = value;
          while (CONSP (tem))
            {
              Lisp_Object k, v;

              k = XCAR (tem);
              tem = XCDR (tem);
              if (!CONSP (tem))
                break;
              v = XCAR (tem);

              if (EQ (k, QCline_width))
                {
                  if ((!CONSP (v) || !FIXNUMP (XCAR (v))
                       || XFIXNUM (XCAR (v)) == 0 || !FIXNUMP (XCDR (v))
                       || XFIXNUM (XCDR (v)) == 0)
                      && (!FIXNUMP (v) || XFIXNUM (v) == 0))
                    break;
                }
              else if (EQ (k, QCcolor))
                {
                  if (!NILP (v) && (!STRINGP (v) || SCHARS (v) == 0))
                    break;
                }
              else if (EQ (k, QCstyle))
                {
                  if (!NILP (v) && !EQ (v, Qpressed_button)
                      && !EQ (v, Qreleased_button) && !EQ (v, Qflat_button))
                    break;
                }
              else
                break;

              tem = XCDR (tem);
            }

          valid_p = NILP (tem);
        }
      else
        valid_p = false;

      if (!valid_p)
        signal_error ("Invalid face box", value);

      old_value = LFACE_BOX (lface);
      ASET (lface, LFACE_BOX_INDEX, value);
    }
  else if (EQ (attr, QCinverse_video) || EQ (attr, QCreverse_video))
    {
      if (!UNSPECIFIEDP (value) && !IGNORE_DEFFACE_P (value)
          && !RESET_P (value))
        {
          CHECK_SYMBOL (value);
          if (!EQ (value, Qt) && !NILP (value))
            signal_error ("Invalid inverse-video face attribute value", value);
        }
      old_value = LFACE_INVERSE (lface);
      ASET (lface, LFACE_INVERSE_INDEX, value);
    }
  else if (EQ (attr, QCextend))
    {
      if (!UNSPECIFIEDP (value) && !IGNORE_DEFFACE_P (value)
          && !RESET_P (value))
        {
          CHECK_SYMBOL (value);
          if (!EQ (value, Qt) && !NILP (value))
            signal_error ("Invalid extend face attribute value", value);
        }
      old_value = LFACE_EXTEND (lface);
      ASET (lface, LFACE_EXTEND_INDEX, value);
    }
  else if (EQ (attr, QCforeground))
    {
      HANDLE_INVALID_NIL_VALUE (QCforeground, face);
      if (!UNSPECIFIEDP (value) && !IGNORE_DEFFACE_P (value)
          && !RESET_P (value))
        {
          CHECK_STRING (value);
          if (SCHARS (value) == 0)
            signal_error ("Empty foreground color value", value);
        }
      old_value = LFACE_FOREGROUND (lface);
      ASET (lface, LFACE_FOREGROUND_INDEX, value);
    }
  else if (EQ (attr, QCdistant_foreground))
    {
      HANDLE_INVALID_NIL_VALUE (QCdistant_foreground, face);
      if (!UNSPECIFIEDP (value) && !IGNORE_DEFFACE_P (value)
          && !RESET_P (value))
        {
          CHECK_STRING (value);
          if (SCHARS (value) == 0)
            signal_error ("Empty distant-foreground color value", value);
        }
      old_value = LFACE_DISTANT_FOREGROUND (lface);
      ASET (lface, LFACE_DISTANT_FOREGROUND_INDEX, value);
    }
  else if (EQ (attr, QCbackground))
    {
      HANDLE_INVALID_NIL_VALUE (QCbackground, face);
      if (!UNSPECIFIEDP (value) && !IGNORE_DEFFACE_P (value)
          && !RESET_P (value))
        {
          CHECK_STRING (value);
          if (SCHARS (value) == 0)
            signal_error ("Empty background color value", value);
        }
      old_value = LFACE_BACKGROUND (lface);
      ASET (lface, LFACE_BACKGROUND_INDEX, value);
    }
  else if (EQ (attr, QCstipple))
    {
    }
  else if (EQ (attr, QCwidth))
    {
      if (!UNSPECIFIEDP (value) && !IGNORE_DEFFACE_P (value)
          && !RESET_P (value))
        {
          CHECK_SYMBOL (value);
          if (FONT_WIDTH_NAME_NUMERIC (value) < 0)
            signal_error ("Invalid face width", value);
        }
      old_value = LFACE_SWIDTH (lface);
      ASET (lface, LFACE_SWIDTH_INDEX, value);
      prop_index = FONT_WIDTH_INDEX;
    }
  else if (EQ (attr, QCfont))
    {
    }
  else if (EQ (attr, QCfontset))
    {
    }
  else if (EQ (attr, QCinherit))
    {
      Lisp_Object tail;
      if (SYMBOLP (value))
        tail = Qnil;
      else
        for (tail = value; CONSP (tail); tail = XCDR (tail))
          if (!SYMBOLP (XCAR (tail)))
            break;
      if (NILP (tail))
        ASET (lface, LFACE_INHERIT_INDEX, value);
      else
        signal_error ("Invalid face inheritance", value);
    }
  else if (EQ (attr, QCbold))
    {
      old_value = LFACE_WEIGHT (lface);
      if (RESET_P (value))
        ASET (lface, LFACE_WEIGHT_INDEX, value);
      else
        ASET (lface, LFACE_WEIGHT_INDEX, NILP (value) ? Qnormal : Qbold);
      prop_index = FONT_WEIGHT_INDEX;
    }
  else if (EQ (attr, QCitalic))
    {
      attr = QCslant;
      old_value = LFACE_SLANT (lface);
      if (RESET_P (value))
        ASET (lface, LFACE_SLANT_INDEX, value);
      else
        ASET (lface, LFACE_SLANT_INDEX, NILP (value) ? Qnormal : Qitalic);
      prop_index = FONT_SLANT_INDEX;
    }
  else
    signal_error ("Invalid face attribute name", attr);

  if (prop_index)
    {
      font_clear_prop (XVECTOR (lface)->contents, prop_index);
    }

  if (!EQ (frame, Qt) && NILP (Fget (face, Qface_no_inherit))
      && NILP (Fequal (old_value, value)))
    {
      TODO_NELISP_LATER;
    }

  if (!UNSPECIFIEDP (value) && !IGNORE_DEFFACE_P (value)
      && NILP (Fequal (old_value, value)))
    {
      Lisp_Object param;

      param = Qnil;

      if (EQ (face, Qdefault))
        {
          if (EQ (attr, QCforeground))
            param = Qforeground_color;
          else if (EQ (attr, QCbackground))
            param = Qbackground_color;
        }
      else if (EQ (face, Qmenu))
        {
          TODO_NELISP_LATER;
        }

      if (!NILP (param))
        {
          TODO;
        }
    }

  return face;
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
  defsubr (&Sinternal_set_lisp_face_attribute);
  defsubr (&Sinternal_set_font_selection_order);
  defsubr (&Sinternal_set_alternative_font_family_alist);
  defsubr (&Sinternal_set_alternative_font_registry_alist);

  DEFVAR_LISP ("face--new-frame-defaults", Vface_new_frame_defaults,
    doc: /* Hash table of global face definitions (for internal use only.)  */);
  Vface_new_frame_defaults
    = make_hash_table (&hashtest_eq, 33, Weak_None, false);
}
