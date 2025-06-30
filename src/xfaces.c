#include "lisp.h"
#include "blockinput.h"
#include "dispextern.h"
#include "font.h"
#include "frame.h"

#include <c-ctype.h>

#define UNSPECIFIEDP(ATTR) EQ (ATTR, Qunspecified)
#define IGNORE_DEFFACE_P(ATTR) EQ (ATTR, QCignore_defface)
#define RESET_P(ATTR) EQ (ATTR, Qreset)

#define FACE_CACHE_BUCKETS_SIZE 1009

char unspecified_fg[] = "unspecified-fg", unspecified_bg[] = "unspecified-bg";

Lisp_Object Vface_alternative_font_family_alist;
Lisp_Object Vface_alternative_font_registry_alist;

static int next_lface_id;

static Lisp_Object *lface_id_to_name;
static ptrdiff_t lface_id_to_name_size;

static bool tty_suppress_bold_inverse_default_colors_p = false;

static bool realize_basic_faces (struct frame *);

static struct face_cache *
make_face_cache (struct frame *f)
{
  struct face_cache *c = xmalloc (sizeof *c);

  c->buckets = xzalloc (FACE_CACHE_BUCKETS_SIZE * sizeof *c->buckets);
  c->size = 50;
  c->used = 0;
  c->faces_by_id = xmalloc (c->size * sizeof *c->faces_by_id);
  c->f = f;
#if TODO_NELISP_LATER_AND
  c->menu_face_changed_p = menu_face_changed_default;
#endif
  return c;
}

void
init_frame_faces (struct frame *f)
{
  if (FRAME_FACE_CACHE (f) == NULL)
    FRAME_FACE_CACHE (f) = make_face_cache (f);

  if (!realize_basic_faces (f))
    emacs_abort ();
}

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

static Lisp_Object face_attr_sym[LFACE_VECTOR_SIZE];

#define check_lface_attrs(attrs) (void) 0
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

static bool
get_lface_attributes_no_remap (struct frame *f, Lisp_Object face_name,
                               Lisp_Object attrs[LFACE_VECTOR_SIZE],
                               bool signal_p)
{
  Lisp_Object lface;

  lface = lface_from_face_name_no_resolve (f, face_name, signal_p);

  if (!NILP (lface))
    memcpy (attrs, xvector_contents (lface), LFACE_VECTOR_SIZE * sizeof *attrs);

  return !NILP (lface);
}

static bool
lface_fully_specified_p (Lisp_Object attrs[LFACE_VECTOR_SIZE])
{
  int i;

  for (i = 1; i < LFACE_VECTOR_SIZE; ++i)
    if (i != LFACE_FONT_INDEX && i != LFACE_INHERIT_INDEX
        && i != LFACE_DISTANT_FOREGROUND_INDEX)
      if ((UNSPECIFIEDP (attrs[i]) || IGNORE_DEFFACE_P (attrs[i])))
        break;

  return i == LFACE_VECTOR_SIZE;
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

static Lisp_Object
filter_face_ref (Lisp_Object face_ref, struct window *w, bool *ok,
                 bool err_msgs)
{
  Lisp_Object orig_face_ref = face_ref;
  if (!CONSP (face_ref))
    return face_ref;

  {
    if (!EQ (XCAR (face_ref), QCfiltered))
      return face_ref;
    face_ref = XCDR (face_ref);

    if (!CONSP (face_ref))
      goto err;
    Lisp_Object filter = XCAR (face_ref);
    face_ref = XCDR (face_ref);

    if (!CONSP (face_ref))
      goto err;
    Lisp_Object filtered_face_ref = XCAR (face_ref);
    face_ref = XCDR (face_ref);

    if (!NILP (face_ref))
      goto err;

    TODO; // return evaluate_face_filter (filter, w, ok, err_msgs)
          //  ? filtered_face_ref : Qnil;
  }

err:
  if (err_msgs)
    TODO; // add_to_log ("Invalid face ref %S", orig_face_ref);
  *ok = false;
  return Qnil;
}

#if TODO_NELISP_LATER_ELSE
struct named_merge_point;
#endif
static bool
merge_face_ref (struct window *w, struct frame *f, Lisp_Object face_ref,
                Lisp_Object *to, bool err_msgs,
                struct named_merge_point *named_merge_points,
                enum lface_attribute_index attr_filter)
{
  bool ok = true;
  Lisp_Object filtered_face_ref;
  bool attr_filter_passed = false;

  filtered_face_ref = face_ref;
  do
    {
      face_ref = filtered_face_ref;
      filtered_face_ref = filter_face_ref (face_ref, w, &ok, err_msgs);
    }
  while (ok && !EQ (face_ref, filtered_face_ref));

  if (!ok)
    return false;

  if (NILP (face_ref))
    return true;

  if (CONSP (face_ref))
    {
      Lisp_Object first = XCAR (face_ref);

      if (EQ (first, Qforeground_color) || EQ (first, Qbackground_color))
        {
          Lisp_Object color_name = XCDR (face_ref);
          Lisp_Object color = first;

          if (STRINGP (color_name))
            {
              if (EQ (color, Qforeground_color))
                to[LFACE_FOREGROUND_INDEX] = color_name;
              else
                to[LFACE_BACKGROUND_INDEX] = color_name;
            }
          else
            {
              if (err_msgs)
                TODO; // add_to_log ("Invalid face color %S", color_name);
              ok = false;
            }
        }
      else if (SYMBOLP (first) && *SDATA (SYMBOL_NAME (first)) == ':')
        {
          if (attr_filter > 0)
            {
              eassert (attr_filter < LFACE_VECTOR_SIZE);

              Lisp_Object parent_face = Qnil;
              bool attr_filter_seen = false;
              Lisp_Object face_ref_tem = face_ref;
              while (CONSP (face_ref_tem) && CONSP (XCDR (face_ref_tem)))
                {
                  Lisp_Object keyword = XCAR (face_ref_tem);
                  Lisp_Object value = XCAR (XCDR (face_ref_tem));

                  if (EQ (keyword, face_attr_sym[attr_filter])
                      || (attr_filter == LFACE_INVERSE_INDEX
                          && EQ (keyword, QCreverse_video)))
                    {
                      attr_filter_seen = true;
                      if (NILP (value))
                        return true;
                    }
                  else if (EQ (keyword, QCinherit))
                    parent_face = value;
                  face_ref_tem = XCDR (XCDR (face_ref_tem));
                }
              if (!attr_filter_seen)
                {
                  if (NILP (parent_face))
                    return true;

                  Lisp_Object scratch_attrs[LFACE_VECTOR_SIZE];
                  int i;

                  scratch_attrs[0] = Qface;
                  for (i = 1; i < LFACE_VECTOR_SIZE; i++)
                    scratch_attrs[i] = Qunspecified;
                  if (!merge_face_ref (w, f, parent_face, scratch_attrs,
                                       err_msgs, named_merge_points, 0))
                    {
                      TODO; // add_to_log ("Invalid face attribute %S %S",
                            // QCinherit,
                      //             parent_face);
                      return false;
                    }
                  if (NILP (scratch_attrs[attr_filter])
                      || UNSPECIFIEDP (scratch_attrs[attr_filter]))
                    return true;
                }
              attr_filter_passed = true;
            }
          while (CONSP (face_ref) && CONSP (XCDR (face_ref)))
            {
              Lisp_Object keyword = XCAR (face_ref);
              Lisp_Object value = XCAR (XCDR (face_ref));
              bool err = false;

              if (EQ (value, Qunspecified))
                ;
              else if (EQ (keyword, QCfamily))
                {
                  if (STRINGP (value))
                    {
                      to[LFACE_FAMILY_INDEX] = value;
                      font_clear_prop (to, FONT_FAMILY_INDEX);
                    }
                  else
                    err = true;
                }
              else if (EQ (keyword, QCfoundry))
                {
                  if (STRINGP (value))
                    {
                      to[LFACE_FOUNDRY_INDEX] = value;
                      font_clear_prop (to, FONT_FOUNDRY_INDEX);
                    }
                  else
                    err = true;
                }
              else if (EQ (keyword, QCheight))
                {
                  Lisp_Object new_height
                    = merge_face_heights (value, to[LFACE_HEIGHT_INDEX], Qnil);

                  if (!NILP (new_height))
                    {
                      to[LFACE_HEIGHT_INDEX] = new_height;
                      font_clear_prop (to, FONT_SIZE_INDEX);
                    }
                  else
                    err = true;
                }
              else if (EQ (keyword, QCweight))
                {
                  if (SYMBOLP (value) && FONT_WEIGHT_NAME_NUMERIC (value) >= 0)
                    {
                      to[LFACE_WEIGHT_INDEX] = value;
                      font_clear_prop (to, FONT_WEIGHT_INDEX);
                    }
                  else
                    err = true;
                }
              else if (EQ (keyword, QCslant))
                {
                  if (SYMBOLP (value) && FONT_SLANT_NAME_NUMERIC (value) >= 0)
                    {
                      to[LFACE_SLANT_INDEX] = value;
                      font_clear_prop (to, FONT_SLANT_INDEX);
                    }
                  else
                    err = true;
                }
              else if (EQ (keyword, QCunderline))
                {
                  if (EQ (value, Qt) || NILP (value) || STRINGP (value)
                      || CONSP (value))
                    to[LFACE_UNDERLINE_INDEX] = value;
                  else
                    err = true;
                }
              else if (EQ (keyword, QCoverline))
                {
                  if (EQ (value, Qt) || NILP (value) || STRINGP (value))
                    to[LFACE_OVERLINE_INDEX] = value;
                  else
                    err = true;
                }
              else if (EQ (keyword, QCstrike_through))
                {
                  if (EQ (value, Qt) || NILP (value) || STRINGP (value))
                    to[LFACE_STRIKE_THROUGH_INDEX] = value;
                  else
                    err = true;
                }
              else if (EQ (keyword, QCbox))
                {
                  if (EQ (value, Qt))
                    value = make_fixnum (1);
                  if ((FIXNUMP (value) && XFIXNUM (value) != 0)
                      || STRINGP (value) || CONSP (value) || NILP (value))
                    to[LFACE_BOX_INDEX] = value;
                  else
                    err = true;
                }
              else if (EQ (keyword, QCinverse_video)
                       || EQ (keyword, QCreverse_video))
                {
                  if (EQ (value, Qt) || NILP (value))
                    to[LFACE_INVERSE_INDEX] = value;
                  else
                    err = true;
                }
              else if (EQ (keyword, QCforeground))
                {
                  if (STRINGP (value))
                    to[LFACE_FOREGROUND_INDEX] = value;
                  else
                    err = true;
                }
              else if (EQ (keyword, QCdistant_foreground))
                {
                  if (STRINGP (value))
                    to[LFACE_DISTANT_FOREGROUND_INDEX] = value;
                  else
                    err = true;
                }
              else if (EQ (keyword, QCbackground))
                {
                  if (STRINGP (value))
                    to[LFACE_BACKGROUND_INDEX] = value;
                  else
                    err = true;
                }
              else if (EQ (keyword, QCstipple))
                {
#if defined(HAVE_WINDOW_SYSTEM)
                  if (NILP (value) || !NILP (Fbitmap_spec_p (value)))
                    to[LFACE_STIPPLE_INDEX] = value;
                  else
                    err = true;
#endif
                }
              else if (EQ (keyword, QCwidth))
                {
                  if (SYMBOLP (value) && FONT_WIDTH_NAME_NUMERIC (value) >= 0)
                    {
                      to[LFACE_SWIDTH_INDEX] = value;
                      font_clear_prop (to, FONT_WIDTH_INDEX);
                    }
                  else
                    err = true;
                }
              else if (EQ (keyword, QCfont))
                {
                  if (FONTP (value))
                    to[LFACE_FONT_INDEX] = value;
                  else
                    err = true;
                }
              else if (EQ (keyword, QCinherit))
                {
                  if (attr_filter_passed)
                    {
                      if (!merge_face_ref (w, f, value, to, err_msgs,
                                           named_merge_points, 0))
                        err = true;
                    }
                  else if (!merge_face_ref (w, f, value, to, err_msgs,
                                            named_merge_points, attr_filter))
                    err = true;
                }
              else if (EQ (keyword, QCextend))
                {
                  if (EQ (value, Qt) || NILP (value))
                    to[LFACE_EXTEND_INDEX] = value;
                  else
                    err = true;
                }
              else
                err = true;

              if (err)
                {
                  TODO; // add_to_log ("Invalid face attribute %S %S", keyword,
                        // value);
                  ok = false;
                }

              face_ref = XCDR (XCDR (face_ref));
            }
        }
      else
        {
          Lisp_Object next = XCDR (face_ref);

          if (!NILP (next))
            ok = merge_face_ref (w, f, next, to, err_msgs, named_merge_points,
                                 attr_filter);

          if (!merge_face_ref (w, f, first, to, err_msgs, named_merge_points,
                               attr_filter))
            ok = false;
        }
    }
  else
    {
      TODO;
    }

  return ok;
}

static void
merge_face_vectors (struct window *w, struct frame *f, const Lisp_Object *from,
                    Lisp_Object *to,
                    struct named_merge_point *named_merge_points)
{
  int i;
  Lisp_Object font = Qnil, tospec, adstyle;

  if (!UNSPECIFIEDP (from[LFACE_INHERIT_INDEX])
      && !NILP (from[LFACE_INHERIT_INDEX]))
    merge_face_ref (w, f, from[LFACE_INHERIT_INDEX], to, false,
                    named_merge_points, 0);

  if (FONT_SPEC_P (from[LFACE_FONT_INDEX]))
    TODO;

  for (i = 1; i < LFACE_VECTOR_SIZE; ++i)
    if (!UNSPECIFIEDP (from[i]))
      {
        if (i == LFACE_HEIGHT_INDEX && !FIXNUMP (from[i]))
          {
            to[i] = merge_face_heights (from[i], to[i], to[i]);
            font_clear_prop (to, FONT_SIZE_INDEX);
          }
        else if (i != LFACE_FONT_INDEX && !EQ (to[i], from[i]))
          {
            to[i] = from[i];
            if (i >= LFACE_FAMILY_INDEX && i <= LFACE_SLANT_INDEX)
              font_clear_prop (to,
                               (i == LFACE_FAMILY_INDEX    ? FONT_FAMILY_INDEX
                                : i == LFACE_FOUNDRY_INDEX ? FONT_FOUNDRY_INDEX
                                : i == LFACE_SWIDTH_INDEX  ? FONT_WIDTH_INDEX
                                : i == LFACE_HEIGHT_INDEX  ? FONT_SIZE_INDEX
                                : i == LFACE_WEIGHT_INDEX  ? FONT_WEIGHT_INDEX
                                                           : FONT_SLANT_INDEX));
          }
      }

  if (!NILP (font))
    {
      if (!NILP (AREF (font, FONT_FOUNDRY_INDEX)))
        to[LFACE_FOUNDRY_INDEX] = SYMBOL_NAME (AREF (font, FONT_FOUNDRY_INDEX));
      if (!NILP (AREF (font, FONT_FAMILY_INDEX)))
        to[LFACE_FAMILY_INDEX] = SYMBOL_NAME (AREF (font, FONT_FAMILY_INDEX));
      if (!NILP (AREF (font, FONT_WEIGHT_INDEX)))
        to[LFACE_WEIGHT_INDEX] = FONT_WEIGHT_FOR_FACE (font);
      if (!NILP (AREF (font, FONT_SLANT_INDEX)))
        to[LFACE_SLANT_INDEX] = FONT_SLANT_FOR_FACE (font);
      if (!NILP (AREF (font, FONT_WIDTH_INDEX)))
        to[LFACE_SWIDTH_INDEX] = FONT_WIDTH_FOR_FACE (font);

      if (!NILP (AREF (font, FONT_ADSTYLE_INDEX)))
        {
          tospec = to[LFACE_FONT_INDEX];
          adstyle = AREF (font, FONT_ADSTYLE_INDEX);

          TODO;

          to[LFACE_FONT_INDEX] = tospec;
          ASET (tospec, FONT_ADSTYLE_INDEX, adstyle);
        }

      ASET (font, FONT_SIZE_INDEX, Qnil);
    }

  to[LFACE_INHERIT_INDEX] = Qnil;
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

static bool
face_attr_equal_p (Lisp_Object v1, Lisp_Object v2)
{
  if (XTYPE (v1) != XTYPE (v2))
    return false;

  if (EQ (v1, v2))
    return true;

  switch (XTYPE (v1))
    {
    case Lisp_String:
      if (SBYTES (v1) != SBYTES (v2))
        return false;

      return memcmp (SDATA (v1), SDATA (v2), SBYTES (v1)) == 0;

    case_Lisp_Int:
    case Lisp_Symbol:
      return false;

    default:
      return !NILP (Fequal (v1, v2));
    }
}

static bool
tty_supports_face_attributes_p (struct frame *f,
                                Lisp_Object attrs[LFACE_VECTOR_SIZE],
                                struct face *def_face)
{
  int weight, slant;
  Lisp_Object val, fg, bg;
  Emacs_Color fg_tty_color, fg_std_color;
  Emacs_Color bg_tty_color, bg_std_color;
  unsigned test_caps = 0;
  UNUSED (test_caps);
  Lisp_Object *def_attrs = def_face->lface;

  if (!UNSPECIFIEDP (attrs[LFACE_FAMILY_INDEX])
      || !UNSPECIFIEDP (attrs[LFACE_FOUNDRY_INDEX])
      || !UNSPECIFIEDP (attrs[LFACE_STIPPLE_INDEX])
      || !UNSPECIFIEDP (attrs[LFACE_HEIGHT_INDEX])
      || !UNSPECIFIEDP (attrs[LFACE_SWIDTH_INDEX])
      || !UNSPECIFIEDP (attrs[LFACE_OVERLINE_INDEX])
      || !UNSPECIFIEDP (attrs[LFACE_BOX_INDEX]))
    return false;

  val = attrs[LFACE_WEIGHT_INDEX];
  if (!UNSPECIFIEDP (val)
      && (weight = FONT_WEIGHT_NAME_NUMERIC (val), weight >= 0))
    {
      int def_weight = FONT_WEIGHT_NAME_NUMERIC (def_attrs[LFACE_WEIGHT_INDEX]);

      if (weight > 100)
        {
          if (def_weight > 100)
            return false;
          test_caps = TTY_CAP_BOLD;
        }
      else if (weight < 100)
        {
          if (def_weight < 100)
            return false;
          test_caps = TTY_CAP_DIM;
        }
      else if (def_weight == 100)
        return false;
    }

  val = attrs[LFACE_SLANT_INDEX];
  if (!UNSPECIFIEDP (val)
      && (slant = FONT_SLANT_NAME_NUMERIC (val), slant >= 0))
    {
      int def_slant = FONT_SLANT_NAME_NUMERIC (def_attrs[LFACE_SLANT_INDEX]);
      if (slant == 100 || slant == def_slant)
        return false;
      else
        test_caps |= TTY_CAP_ITALIC;
    }

  val = attrs[LFACE_UNDERLINE_INDEX];
  if (!UNSPECIFIEDP (val))
    {
      if (STRINGP (val))
        test_caps |= TTY_CAP_UNDERLINE_STYLED;
      else if (EQ (CAR_SAFE (val), QCstyle))
        {
          if (!(EQ (CAR_SAFE (CDR_SAFE (val)), Qline)
                || EQ (CAR_SAFE (CDR_SAFE (val)), Qdouble_line)
                || EQ (CAR_SAFE (CDR_SAFE (val)), Qwave)
                || EQ (CAR_SAFE (CDR_SAFE (val)), Qdots)
                || EQ (CAR_SAFE (CDR_SAFE (val)), Qdashes)))
            return false;

          test_caps |= TTY_CAP_UNDERLINE_STYLED;
        }
      else if (face_attr_equal_p (val, def_attrs[LFACE_UNDERLINE_INDEX]))
        return false;
      else
        test_caps |= TTY_CAP_UNDERLINE;
    }

  val = attrs[LFACE_INVERSE_INDEX];
  if (!UNSPECIFIEDP (val))
    {
      if (face_attr_equal_p (val, def_attrs[LFACE_INVERSE_INDEX]))
        return false;
      else
        test_caps |= TTY_CAP_INVERSE;
    }

  val = attrs[LFACE_STRIKE_THROUGH_INDEX];
  if (!UNSPECIFIEDP (val))
    {
      if (face_attr_equal_p (val, def_attrs[LFACE_STRIKE_THROUGH_INDEX]))
        return false;
      else
        test_caps |= TTY_CAP_STRIKE_THROUGH;
    }

  fg = attrs[LFACE_FOREGROUND_INDEX];
  if (STRINGP (fg))
    {
      Lisp_Object def_fg = def_attrs[LFACE_FOREGROUND_INDEX];

      if (face_attr_equal_p (fg, def_fg))
        return false;
      TODO;
    }

  bg = attrs[LFACE_BACKGROUND_INDEX];
  if (STRINGP (bg))
    {
      Lisp_Object def_bg = def_attrs[LFACE_BACKGROUND_INDEX];

      if (face_attr_equal_p (bg, def_bg))
        return false;
      TODO;
    }

  if (STRINGP (fg) && STRINGP (bg))
    {
      TODO;
    }

#if TODO_NELISP_LATER_AND
  return tty_capable_p (FRAME_TTY (f), test_caps);
#else
  return true;
#endif
}

DEFUN ("display-supports-face-attributes-p",
       Fdisplay_supports_face_attributes_p, Sdisplay_supports_face_attributes_p,
       1, 2, 0,
       doc: /* Return non-nil if all the face attributes in ATTRIBUTES are supported.
The optional argument DISPLAY can be a display name, a frame, or
nil (meaning the selected frame's display).

For instance, to check whether the display supports underlining:

  (display-supports-face-attributes-p \\='(:underline t))

The definition of `supported' is somewhat heuristic, but basically means
that a face containing all the attributes in ATTRIBUTES, when merged
with the default face for display, can be represented in a way that's

 (1) different in appearance from the default face, and
 (2) `close in spirit' to what the attributes specify, if not exact.

Point (2) implies that a `:weight black' attribute will be satisfied by
any display that can display bold, and a `:foreground \"yellow\"' as long
as it can display a yellowish color, but `:slant italic' will _not_ be
satisfied by the tty display code's automatic substitution of a `dim'
face for italic.  */)
(Lisp_Object attributes, Lisp_Object display)
{
  bool supports = false;
  int i;
  Lisp_Object frame;
  struct frame *f;
  struct face *def_face;
  Lisp_Object attrs[LFACE_VECTOR_SIZE];

  if (noninteractive)
    TODO;

  if (NILP (display))
    frame = selected_frame;
  else if (FRAMEP (display))
    frame = display;
  else
    {
      TODO;
    }

  CHECK_LIVE_FRAME (frame);
  f = XFRAME (frame);

  for (i = 0; i < LFACE_VECTOR_SIZE; i++)
    attrs[i] = Qunspecified;
  merge_face_ref (NULL, f, attributes, attrs, true, NULL, 0);

  def_face = FACE_FROM_ID_OR_NULL (f, DEFAULT_FACE_ID);
  if (def_face == NULL)
    {
      if (!realize_basic_faces (f))
        error ("Cannot realize default face");
      def_face = FACE_FROM_ID (f, DEFAULT_FACE_ID);
    }

  if (FRAME_TERMCAP_P (f) || FRAME_MSDOS_P (f))
    supports = tty_supports_face_attributes_p (f, attrs, def_face);

  return supports ? Qt : Qnil;
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

static struct face *
make_realized_face (Lisp_Object *attr)
{
  enum
  {
    off = offsetof (struct face, id)
  };
  struct face *face = xmalloc (sizeof *face);

  memcpy (face->lface, attr, sizeof face->lface);
  memset (&face->id, 0, sizeof *face - off);
  face->ascii_face = face;

  return face;
}

static void
map_tty_color (struct frame *f, struct face *face, Lisp_Object color,
               enum lface_attribute_index idx, bool *defaulted)
{
  Lisp_Object frame, def;
  bool foreground_p = idx != LFACE_BACKGROUND_INDEX;
  unsigned long default_pixel
    = foreground_p ? FACE_TTY_DEFAULT_FG_COLOR : FACE_TTY_DEFAULT_BG_COLOR;
  unsigned long pixel = default_pixel;

  eassert (idx == LFACE_FOREGROUND_INDEX || idx == LFACE_BACKGROUND_INDEX
           || idx == LFACE_UNDERLINE_INDEX);

  XSETFRAME (frame, f);

  if (STRINGP (color) && SCHARS (color) && CONSP (Vtty_defined_color_alist)
      && (TODO, false))
    {
      pixel = XFIXNUM (XCAR (XCDR (def)));
    }

  if (pixel == default_pixel && STRINGP (color))
    {
      TODO; // pixel = load_color (f, face, color, idx);
    }

  switch (idx)
    {
    case LFACE_FOREGROUND_INDEX:
      face->foreground = pixel;
      break;
    case LFACE_UNDERLINE_INDEX:
      face->underline_color = pixel;
      break;
    case LFACE_BACKGROUND_INDEX:
    default:
      face->background = pixel;
      break;
    }
}

static struct face *
realize_tty_face (struct face_cache *cache,
                  Lisp_Object attrs[LFACE_VECTOR_SIZE])
{
  struct face *face;
  int weight, slant;
  Lisp_Object underline;
  bool face_colors_defaulted = false;
  struct frame *f = cache->f;

  eassert (FRAME_TERMCAP_P (cache->f) || FRAME_MSDOS_P (cache->f));

  face = make_realized_face (attrs);

  weight = FONT_WEIGHT_NAME_NUMERIC (attrs[LFACE_WEIGHT_INDEX]);
  slant = FONT_SLANT_NAME_NUMERIC (attrs[LFACE_SLANT_INDEX]);
  if (weight > 100)
    face->tty_bold_p = true;
  if (slant != 100)
    face->tty_italic_p = true;
  if (!NILP (attrs[LFACE_INVERSE_INDEX]))
    face->tty_reverse_p = true;
  if (!NILP (attrs[LFACE_STRIKE_THROUGH_INDEX]))
    face->tty_strike_through_p = true;

  underline = attrs[LFACE_UNDERLINE_INDEX];
  if (NILP (underline))
    {
      face->underline = FACE_NO_UNDERLINE;
      face->underline_color = 0;
    }
  else if (EQ (underline, Qt))
    {
      face->underline = FACE_UNDERLINE_SINGLE;
      face->underline_color = 0;
    }
  else if (STRINGP (underline))
    {
      face->underline = FACE_UNDERLINE_SINGLE;
      bool underline_color_defaulted;
      map_tty_color (f, face, underline, LFACE_UNDERLINE_INDEX,
                     &underline_color_defaulted);
    }
  else if (CONSP (underline))
    {
      face->underline = FACE_UNDERLINE_SINGLE;
      face->underline_color = 0;

      while (CONSP (underline))
        {
          Lisp_Object keyword, value;

          keyword = XCAR (underline);
          underline = XCDR (underline);

          if (!CONSP (underline))
            break;
          value = XCAR (underline);
          underline = XCDR (underline);

          if (EQ (keyword, QCcolor))
            {
              if (EQ (value, Qforeground_color))
                face->underline_color = 0;
              else if (STRINGP (value))
                {
                  bool underline_color_defaulted;
                  map_tty_color (f, face, value, LFACE_UNDERLINE_INDEX,
                                 &underline_color_defaulted);
                }
            }
          else if (EQ (keyword, QCstyle))
            {
              if (EQ (value, Qline))
                face->underline = FACE_UNDERLINE_SINGLE;
              else if (EQ (value, Qdouble_line))
                face->underline = FACE_UNDERLINE_DOUBLE_LINE;
              else if (EQ (value, Qwave))
                face->underline = FACE_UNDERLINE_WAVE;
              else if (EQ (value, Qdots))
                face->underline = FACE_UNDERLINE_DOTS;
              else if (EQ (value, Qdashes))
                face->underline = FACE_UNDERLINE_DASHES;
              else
                face->underline = FACE_UNDERLINE_SINGLE;
            }
        }
    }

  map_tty_color (f, face, face->lface[LFACE_FOREGROUND_INDEX],
                 LFACE_FOREGROUND_INDEX, &face_colors_defaulted);
  map_tty_color (f, face, face->lface[LFACE_BACKGROUND_INDEX],
                 LFACE_BACKGROUND_INDEX, &face_colors_defaulted);

  if (face->tty_reverse_p && !face_colors_defaulted)
    {
      unsigned long tem = face->foreground;
      face->foreground = face->background;
      face->background = tem;
    }

  if (tty_suppress_bold_inverse_default_colors_p && face->tty_bold_p
      && face->background == FACE_TTY_DEFAULT_FG_COLOR
      && face->foreground == FACE_TTY_DEFAULT_BG_COLOR)
    face->tty_bold_p = false;

  return face;
}

static void
free_realized_face (struct frame *f, struct face *face)
{
  if (face)
    {
      xfree (face);
    }
}

static void
cache_face (struct face_cache *c, struct face *face, uintptr_t hash)
{
  int i = hash % FACE_CACHE_BUCKETS_SIZE;

  face->hash = hash;

  if (face->ascii_face != face)
    {
      struct face *last = c->buckets[i];
      if (last)
        {
          while (last->next)
            last = last->next;
          last->next = face;
          face->prev = last;
          face->next = NULL;
        }
      else
        {
          c->buckets[i] = face;
          face->prev = face->next = NULL;
        }
    }
  else
    {
      face->prev = NULL;
      face->next = c->buckets[i];
      if (face->next)
        face->next->prev = face;
      c->buckets[i] = face;
    }

  for (i = 0; i < c->used; ++i)
    if (c->faces_by_id[i] == NULL)
      break;
  face->id = i;

  if (i == c->used)
    {
      if (c->used == c->size)
        c->faces_by_id = xpalloc (c->faces_by_id, &c->size, 1, MAX_FACE_ID,
                                  sizeof *c->faces_by_id);
      c->used++;
    }

  c->faces_by_id[i] = face;
}

static void
uncache_face (struct face_cache *c, struct face *face)
{
  int i = face->hash % FACE_CACHE_BUCKETS_SIZE;

  if (face->prev)
    face->prev->next = face->next;
  else
    c->buckets[i] = face->next;

  if (face->next)
    face->next->prev = face->prev;

  c->faces_by_id[face->id] = NULL;
  if (face->id == c->used)
    --c->used;
}

static uintptr_t
hash_string_case_insensitive (Lisp_Object string)
{
  const unsigned char *s;
  uintptr_t hash = 0;
  eassert (STRINGP (string));
  for (s = SDATA (string); *s; ++s)
    hash = (hash << 1) ^ c_tolower (*s);
  return hash;
}

static uintptr_t
lface_hash (Lisp_Object *v)
{
  return (hash_string_case_insensitive (v[LFACE_FAMILY_INDEX])
          ^ hash_string_case_insensitive (v[LFACE_FOUNDRY_INDEX])
          ^ hash_string_case_insensitive (v[LFACE_FOREGROUND_INDEX])
          ^ hash_string_case_insensitive (v[LFACE_BACKGROUND_INDEX])
          ^ XHASH (v[LFACE_WEIGHT_INDEX]) ^ XHASH (v[LFACE_SLANT_INDEX])
          ^ XHASH (v[LFACE_SWIDTH_INDEX]) ^ XHASH (v[LFACE_HEIGHT_INDEX]));
}

static struct face *
realize_face (struct face_cache *cache, Lisp_Object attrs[LFACE_VECTOR_SIZE],
              int former_face_id)
{
  struct face *face;

  eassert (cache != NULL);
  check_lface_attrs (attrs);

  if (former_face_id >= 0 && cache->used > former_face_id)
    {
      struct face *former_face = cache->faces_by_id[former_face_id];
      if (former_face)
        uncache_face (cache, former_face);
      free_realized_face (cache->f, former_face);
      SET_FRAME_GARBAGED (cache->f);
    }

  if (FRAME_WINDOW_P (cache->f))
    TODO; // face = realize_gui_face (cache, attrs);
  else if (FRAME_TERMCAP_P (cache->f) || FRAME_MSDOS_P (cache->f))
    face = realize_tty_face (cache, attrs);
  else if (FRAME_INITIAL_P (cache->f))
    {
      TODO; // face = make_realized_face (attrs);
    }
  else
    emacs_abort ();

  cache_face (cache, face, lface_hash (attrs));
  return face;
}

static bool
realize_default_face (struct frame *f)
{
  struct face_cache *c = FRAME_FACE_CACHE (f);
  Lisp_Object lface;
  Lisp_Object attrs[LFACE_VECTOR_SIZE];

  lface = lface_from_face_name (f, Qdefault, false);
  if (NILP (lface))
    {
      Lisp_Object frame;
      XSETFRAME (frame, f);
      lface = Finternal_make_lisp_face (Qdefault, frame);
    }

  if (!FRAME_WINDOW_P (f))
    {
      ASET (lface, LFACE_FAMILY_INDEX, build_string ("default"));
      ASET (lface, LFACE_FOUNDRY_INDEX, LFACE_FAMILY (lface));
      ASET (lface, LFACE_SWIDTH_INDEX, Qnormal);
      ASET (lface, LFACE_HEIGHT_INDEX, make_fixnum (1));
      if (UNSPECIFIEDP (LFACE_WEIGHT (lface)))
        ASET (lface, LFACE_WEIGHT_INDEX, Qnormal);
      if (UNSPECIFIEDP (LFACE_SLANT (lface)))
        ASET (lface, LFACE_SLANT_INDEX, Qnormal);
      if (UNSPECIFIEDP (LFACE_FONTSET (lface)))
        ASET (lface, LFACE_FONTSET_INDEX, Qnil);
    }

  if (UNSPECIFIEDP (LFACE_EXTEND (lface)))
    ASET (lface, LFACE_EXTEND_INDEX, Qnil);

  if (UNSPECIFIEDP (LFACE_UNDERLINE (lface)))
    ASET (lface, LFACE_UNDERLINE_INDEX, Qnil);

  if (UNSPECIFIEDP (LFACE_OVERLINE (lface)))
    ASET (lface, LFACE_OVERLINE_INDEX, Qnil);

  if (UNSPECIFIEDP (LFACE_STRIKE_THROUGH (lface)))
    ASET (lface, LFACE_STRIKE_THROUGH_INDEX, Qnil);

  if (UNSPECIFIEDP (LFACE_BOX (lface)))
    ASET (lface, LFACE_BOX_INDEX, Qnil);

  if (UNSPECIFIEDP (LFACE_INVERSE (lface)))
    ASET (lface, LFACE_INVERSE_INDEX, Qnil);

  if (UNSPECIFIEDP (LFACE_FOREGROUND (lface)))
    {
      Lisp_Object color = Fassq (Qforeground_color, f->param_alist);

      if (CONSP (color) && STRINGP (XCDR (color)))
        ASET (lface, LFACE_FOREGROUND_INDEX, XCDR (color));
      else if (FRAME_WINDOW_P (f))
        return false;
      else if (FRAME_INITIAL_P (f) || FRAME_TERMCAP_P (f) || FRAME_MSDOS_P (f))
        ASET (lface, LFACE_FOREGROUND_INDEX, build_string (unspecified_fg));
      else
        emacs_abort ();
    }

  if (UNSPECIFIEDP (LFACE_BACKGROUND (lface)))
    {
      Lisp_Object color = Fassq (Qbackground_color, f->param_alist);
      if (CONSP (color) && STRINGP (XCDR (color)))
        ASET (lface, LFACE_BACKGROUND_INDEX, XCDR (color));
      else if (FRAME_WINDOW_P (f))
        return false;
      else if (FRAME_INITIAL_P (f) || FRAME_TERMCAP_P (f) || FRAME_MSDOS_P (f))
        ASET (lface, LFACE_BACKGROUND_INDEX, build_string (unspecified_bg));
      else
        emacs_abort ();
    }

  if (UNSPECIFIEDP (LFACE_STIPPLE (lface)))
    ASET (lface, LFACE_STIPPLE_INDEX, Qnil);

  eassert (lface_fully_specified_p (XVECTOR (lface)->contents));
  check_lface (lface);
  memcpy (attrs, xvector_contents (lface), sizeof attrs);

  specpdl_ref count = SPECPDL_INDEX ();
  specbind (Qinhibit_redisplay, Qt);
  struct face *face = realize_face (c, attrs, DEFAULT_FACE_ID);
  unbind_to (count, Qnil);

  return true;
}

static void
realize_named_face (struct frame *f, Lisp_Object symbol, int id)
{
  struct face_cache *c = FRAME_FACE_CACHE (f);
  Lisp_Object lface = lface_from_face_name (f, symbol, false);
  Lisp_Object attrs[LFACE_VECTOR_SIZE];
  Lisp_Object symbol_attrs[LFACE_VECTOR_SIZE];

  get_lface_attributes_no_remap (f, Qdefault, attrs, true);
  check_lface_attrs (attrs);
  eassert (lface_fully_specified_p (attrs));

  if (NILP (lface))
    {
      Lisp_Object frame;
      XSETFRAME (frame, f);
      lface = Finternal_make_lisp_face (symbol, frame);
    }

  get_lface_attributes_no_remap (f, symbol, symbol_attrs, true);

  int i;
  for (i = 1; i < LFACE_VECTOR_SIZE; i++)
    if (EQ (symbol_attrs[i], Qreset))
      symbol_attrs[i] = attrs[i];
  merge_face_vectors (NULL, f, symbol_attrs, attrs, 0);

  realize_face (c, attrs, id);
}

static bool
realize_basic_faces (struct frame *f)
{
  bool success_p = false;

  block_input ();

  if (realize_default_face (f))
    {
      realize_named_face (f, Qmode_line_active, MODE_LINE_ACTIVE_FACE_ID);
      realize_named_face (f, Qmode_line_inactive, MODE_LINE_INACTIVE_FACE_ID);
      realize_named_face (f, Qtool_bar, TOOL_BAR_FACE_ID);
      realize_named_face (f, Qfringe, FRINGE_FACE_ID);
      realize_named_face (f, Qheader_line, HEADER_LINE_FACE_ID);
      realize_named_face (f, Qscroll_bar, SCROLL_BAR_FACE_ID);
      realize_named_face (f, Qborder, BORDER_FACE_ID);
      realize_named_face (f, Qcursor, CURSOR_FACE_ID);
      realize_named_face (f, Qmouse, MOUSE_FACE_ID);
      realize_named_face (f, Qmenu, MENU_FACE_ID);
      realize_named_face (f, Qvertical_border, VERTICAL_BORDER_FACE_ID);
      realize_named_face (f, Qwindow_divider, WINDOW_DIVIDER_FACE_ID);
      realize_named_face (f, Qwindow_divider_first_pixel,
                          WINDOW_DIVIDER_FIRST_PIXEL_FACE_ID);
      realize_named_face (f, Qwindow_divider_last_pixel,
                          WINDOW_DIVIDER_LAST_PIXEL_FACE_ID);
      realize_named_face (f, Qinternal_border, INTERNAL_BORDER_FACE_ID);
      realize_named_face (f, Qchild_frame_border, CHILD_FRAME_BORDER_FACE_ID);
      realize_named_face (f, Qtab_bar, TAB_BAR_FACE_ID);
      realize_named_face (f, Qtab_line, TAB_LINE_FACE_ID);

#if TODO_NELISP_LATER_AND
      if (FRAME_FACE_CACHE (f)->menu_face_changed_p)
        {
          FRAME_FACE_CACHE (f)->menu_face_changed_p = false;
        }
#endif

      success_p = true;
    }

  unblock_input ();
  return success_p;
}

void
init_xfaces (void)
{
  face_attr_sym[0] = Qface;
  face_attr_sym[LFACE_FOUNDRY_INDEX] = QCfoundry;
  face_attr_sym[LFACE_SWIDTH_INDEX] = QCwidth;
  face_attr_sym[LFACE_HEIGHT_INDEX] = QCheight;
  face_attr_sym[LFACE_WEIGHT_INDEX] = QCweight;
  face_attr_sym[LFACE_SLANT_INDEX] = QCslant;
  face_attr_sym[LFACE_UNDERLINE_INDEX] = QCunderline;
  face_attr_sym[LFACE_INVERSE_INDEX] = QCinverse_video;
  face_attr_sym[LFACE_FOREGROUND_INDEX] = QCforeground;
  face_attr_sym[LFACE_BACKGROUND_INDEX] = QCbackground;
  face_attr_sym[LFACE_STIPPLE_INDEX] = QCstipple;
  face_attr_sym[LFACE_OVERLINE_INDEX] = QCoverline;
  face_attr_sym[LFACE_STRIKE_THROUGH_INDEX] = QCstrike_through;
  face_attr_sym[LFACE_BOX_INDEX] = QCbox;
  face_attr_sym[LFACE_FONT_INDEX] = QCfont;
  face_attr_sym[LFACE_INHERIT_INDEX] = QCinherit;
  face_attr_sym[LFACE_FONTSET_INDEX] = QCfontset;
  face_attr_sym[LFACE_DISTANT_FOREGROUND_INDEX] = QCdistant_foreground;
  face_attr_sym[LFACE_EXTEND_INDEX] = QCextend;
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

  DEFVAR_LISP ("tty-defined-color-alist", Vtty_defined_color_alist,
   doc: /* An alist of defined terminal colors and their RGB values.
See the docstring of `tty-color-alist' for the details.  */);
  Vtty_defined_color_alist = Qnil;
}
