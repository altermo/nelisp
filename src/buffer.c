#include "buffer.h"
#include "lisp.h"
#include "frame.h"
#include "nvim.h"

struct buffer buffer_defaults;

static void
CHECK_OVERLAY (Lisp_Object x)
{
  CHECK_TYPE (OVERLAYP (x), Qoverlayp, x);
}

EMACS_INT
fix_position (Lisp_Object pos)
{
  if (FIXNUMP (pos))
    return XFIXNUM (pos);
  if (MARKERP (pos))
    TODO; // return marker_position (pos);
  CHECK_TYPE (BIGNUMP (pos), Qinteger_or_marker_p, pos);
  return !NILP (Fnatnump (pos)) ? MOST_POSITIVE_FIXNUM : MOST_NEGATIVE_FIXNUM;
}

void
nsberror (Lisp_Object spec)
{
  if (STRINGP (spec))
    error ("No buffer named %s", SDATA (spec));
  error ("Invalid buffer argument");
}

DEFUN ("buffer-list", Fbuffer_list, Sbuffer_list, 0, 1, 0,
       doc: /* Return a list of all live buffers.
If the optional arg FRAME is a frame, return the buffer list in the
proper order for that frame: the buffers shown in FRAME come first,
followed by the rest of the buffers.  */)
(Lisp_Object frame)
{
  Lisp_Object general;
  general = nvim_buffer_list ();

  if (FRAMEP (frame))
    {
      Lisp_Object framelist, prevlist, tail;

      framelist = Fcopy_sequence (nvim_frame_buffer_list (XFRAME (frame)));
      prevlist = Fnreverse (
        Fcopy_sequence (nvim_frame_buried_buffer_list (XFRAME (frame))));

      tail = framelist;
      while (CONSP (tail))
        {
          general = Fdelq (XCAR (tail), general);
          tail = XCDR (tail);
        }
      tail = prevlist;
      while (CONSP (tail))
        {
          general = Fdelq (XCAR (tail), general);
          tail = XCDR (tail);
        }

      return CALLN (Fnconc, framelist, general, prevlist);
    }
  else
    return general;
}

DEFUN ("get-buffer", Fget_buffer, Sget_buffer, 1, 1, 0,
       doc: /* Return the buffer named BUFFER-OR-NAME.
BUFFER-OR-NAME must be either a string or a buffer.  If BUFFER-OR-NAME
is a string and there is no buffer with that name, return nil.  If
BUFFER-OR-NAME is a buffer, return it as given.  */)
(register Lisp_Object buffer_or_name)
{
  if (BUFFERP (buffer_or_name))
    return buffer_or_name;
  CHECK_STRING (buffer_or_name);

  return nvim_name_to_bufobj (buffer_or_name);
}

DEFUN ("get-buffer-create", Fget_buffer_create, Sget_buffer_create, 1, 2, 0,
       doc: /* Return the buffer specified by BUFFER-OR-NAME, creating a new one if needed.
If BUFFER-OR-NAME is a string and a live buffer with that name exists,
return that buffer.  If no such buffer exists, create a new buffer with
that name and return it.

If BUFFER-OR-NAME starts with a space, the new buffer does not keep undo
information.  If optional argument INHIBIT-BUFFER-HOOKS is non-nil, the
new buffer does not run the hooks `kill-buffer-hook',
`kill-buffer-query-functions', and `buffer-list-update-hook'.  This
avoids slowing down internal or temporary buffers that are never
presented to users or passed on to other applications.

If BUFFER-OR-NAME is a buffer instead of a string, return it as given,
even if it is dead.  The return value is never nil.  */)
(register Lisp_Object buffer_or_name, Lisp_Object inhibit_buffer_hooks)
{
  register Lisp_Object buffer;

  buffer = Fget_buffer (buffer_or_name);
  if (!NILP (buffer))
    return buffer;

  if (SCHARS (buffer_or_name) == 0)
    error ("Empty string for buffer name is not allowed");
  buffer = nvim_create_buf (buffer_or_name, inhibit_buffer_hooks);
  return buffer;
}

DEFUN ("current-buffer", Fcurrent_buffer, Scurrent_buffer, 0, 0, 0,
       doc: /* Return the current buffer as a Lisp object.  */)
(void)
{
  register Lisp_Object buf;
  XSETBUFFER (buf, current_buffer);
  return buf;
}

void
set_buffer_internal (register struct buffer *b)
{
  TODO_NELISP_LATER;
  nvim_set_buffer (b);
}

DEFUN ("set-buffer", Fset_buffer, Sset_buffer, 1, 1, 0,
       doc: /* Make buffer BUFFER-OR-NAME current for editing operations.
BUFFER-OR-NAME may be a buffer or the name of an existing buffer.
See also `with-current-buffer' when you want to make a buffer current
temporarily.  This function does not display the buffer, so its effect
ends when the current command terminates.  Use `switch-to-buffer' or
`pop-to-buffer' to switch buffers permanently.
The return value is the buffer made current.  */)
(register Lisp_Object buffer_or_name)
{
  register Lisp_Object buffer;
  buffer = Fget_buffer (buffer_or_name);
  if (NILP (buffer))
    nsberror (buffer_or_name);
  if (!BUFFER_LIVE_P (XBUFFER (buffer)))
    error ("Selecting deleted buffer");
  set_buffer_internal (XBUFFER (buffer));
  return buffer;
}

void
set_buffer_if_live (Lisp_Object buffer)
{
  if (BUFFER_LIVE_P (XBUFFER (buffer)))
    set_buffer_internal (XBUFFER (buffer));
}

DEFUN ("buffer-name", Fbuffer_name, Sbuffer_name, 0, 1, 0,
       doc: /* Return the name of BUFFER, as a string.
BUFFER defaults to the current buffer.
Return nil if BUFFER has been killed.  */)
(register Lisp_Object buffer) { return BVAR (decode_buffer (buffer), name); }

DEFUN ("force-mode-line-update", Fforce_mode_line_update,
       Sforce_mode_line_update, 0, 1, 0,
       doc: /* Force redisplay of the current buffer's mode line and header line.
With optional non-nil ALL, force redisplay of all mode lines, tab lines and
header lines.  This function also forces recomputation of the
menu bar menus and the frame title.  */)
(Lisp_Object all)
{
  TODO_NELISP_LATER;
  return all;
}

DEFUN ("make-overlay", Fmake_overlay, Smake_overlay, 2, 5, 0,
       doc: /* Create a new overlay with range BEG to END in BUFFER and return it.
If omitted, BUFFER defaults to the current buffer.
BEG and END may be integers or markers.
The fourth arg FRONT-ADVANCE, if non-nil, makes the marker
for the front of the overlay advance when text is inserted there
\(which means the text *is not* included in the overlay).
The fifth arg REAR-ADVANCE, if non-nil, makes the marker
for the rear of the overlay advance when text is inserted there
\(which means the text *is* included in the overlay).  */)
(Lisp_Object beg, Lisp_Object end, Lisp_Object buffer,
 Lisp_Object front_advance, Lisp_Object rear_advance)
{
  Lisp_Object ov;
  struct buffer *b;

  if (NILP (buffer))
    XSETBUFFER (buffer, current_buffer);
  else
    CHECK_BUFFER (buffer);

  b = XBUFFER (buffer);
  if (!BUFFER_LIVE_P (b))
    error ("Attempt to create overlay in a dead buffer");

  if (MARKERP (beg) && !(TODO, false))
    signal_error ("Marker points into wrong buffer", beg);
  if (MARKERP (end) && !(TODO, false))
    signal_error ("Marker points into wrong buffer", end);

  CHECK_FIXNUM_COERCE_MARKER (beg);
  CHECK_FIXNUM_COERCE_MARKER (end);

  if (XFIXNUM (beg) > XFIXNUM (end))
    {
      Lisp_Object temp;
      temp = beg;
      beg = end;
      end = temp;
    }

  ptrdiff_t obeg = clip_to_bounds (BUF_BEG (b), XFIXNUM (beg), BUF_Z (b));
  ptrdiff_t oend = clip_to_bounds (obeg, XFIXNUM (end), BUF_Z (b));
  ov = build_overlay (!NILP (front_advance), !NILP (rear_advance), Qnil);
#if TODO_NELISP_LATER_AND
  add_buffer_overlay (b, XOVERLAY (ov), obeg, oend);
#endif

  return ov;
}

DEFUN ("delete-overlay", Fdelete_overlay, Sdelete_overlay, 1, 1, 0,
       doc: /* Delete the overlay OVERLAY from its buffer.  */)
(Lisp_Object overlay)
{
  struct buffer *b;
  specpdl_ref count = SPECPDL_INDEX ();

  CHECK_OVERLAY (overlay);

  TODO_NELISP_LATER;

  return unbind_to (count, Qnil);
}

DEFUN ("overlay-put", Foverlay_put, Soverlay_put, 3, 3, 0,
       doc: /* Set one property of overlay OVERLAY: give property PROP value VALUE.
VALUE will be returned.*/)
(Lisp_Object overlay, Lisp_Object prop, Lisp_Object value)
{
  Lisp_Object tail;
  struct buffer *b;
  bool changed;
  UNUSED (changed);

  CHECK_OVERLAY (overlay);

#if TODO_NELISP_LATER_AND
  b = OVERLAY_BUFFER (overlay);
#endif

  for (tail = XOVERLAY (overlay)->plist; CONSP (tail) && CONSP (XCDR (tail));
       tail = XCDR (XCDR (tail)))
    if (EQ (XCAR (tail), prop))
      {
        changed = !EQ (XCAR (XCDR (tail)), value);
        XSETCAR (XCDR (tail), value);
        goto found;
      }
  changed = !NILP (value);
  set_overlay_plist (overlay,
                     Fcons (prop, Fcons (value, XOVERLAY (overlay)->plist)));
found:
#if TODO_NELISP_LATER_AND
  if (b)
    {
      if (changed)
        modify_overlay (b, OVERLAY_START (overlay), OVERLAY_END (overlay));
      if (EQ (prop, Qevaporate) && !NILP (value)
          && (OVERLAY_START (overlay) == OVERLAY_END (overlay)))
        Fdelete_overlay (overlay);
    }
#endif

  return value;
}

#define DEFVAR_PER_BUFFER(lname, vname, predicate, doc)     \
  do                                                        \
    {                                                       \
      static struct Lisp_Buffer_Objfwd bo_fwd;              \
      defvar_per_buffer (&bo_fwd, lname, vname, predicate); \
    }                                                       \
  while (0)

static void
defvar_per_buffer (struct Lisp_Buffer_Objfwd *bo_fwd, const char *namestring,
                   Lisp_Object *address, Lisp_Object predicate)
{
  struct Lisp_Symbol *sym;
  int offset;

  sym = XSYMBOL (intern (namestring));
  offset = (char *) address - (char *) current_buffer;

  bo_fwd->type = Lisp_Fwd_Buffer_Obj;
  bo_fwd->offset = offset;
  bo_fwd->predicate = predicate;
  sym->u.s.declared_special = true;
  sym->u.s.redirect = SYMBOL_FORWARDED;
  SET_SYMBOL_FWD (sym, bo_fwd);
}

void
init_buffer (void)
{
  AUTO_STRING (scratch, "*scratch*");
  Fget_buffer_create (scratch, Qnil);
}

void
syms_of_buffer (void)
{
  DEFSYM (Qoverlayp, "overlayp");

  DEFVAR_LISP ("case-fold-search", Vcase_fold_search,
	       doc: /* Non-nil if searches and matches should ignore case.  */);
  Vcase_fold_search = Qt;
  DEFSYM (Qcase_fold_search, "case-fold-search");
  Fmake_variable_buffer_local (Qcase_fold_search);

  DEFVAR_PER_BUFFER ("enable-multibyte-characters",
        &BVAR_ (current_buffer, enable_multibyte_characters),
        Qnil,
        doc: /* Non-nil means the buffer contents are regarded as multi-byte characters.
Otherwise they are regarded as unibyte.  This affects the display,
file I/O and the behavior of various editing commands.

This variable is buffer-local but you cannot set it directly;
use the function `set-buffer-multibyte' to change a buffer's representation.
To prevent any attempts to set it or make it buffer-local, Emacs will
signal an error in those cases.
See also Info node `(elisp)Text Representations'.  */);
  make_symbol_constant (intern_c_string ("enable-multibyte-characters"));

  defsubr (&Sbuffer_list);
  defsubr (&Sget_buffer);
  defsubr (&Sget_buffer_create);
  defsubr (&Scurrent_buffer);
  defsubr (&Sset_buffer);
  defsubr (&Sbuffer_name);
  defsubr (&Sforce_mode_line_update);

  defsubr (&Smake_overlay);
  defsubr (&Sdelete_overlay);
  defsubr (&Soverlay_put);
}
