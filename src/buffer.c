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

DEFUN ("kill-buffer", Fkill_buffer, Skill_buffer, 0, 1, "bKill buffer: ",
       doc: /* Kill the buffer specified by BUFFER-OR-NAME.
The argument may be a buffer or the name of an existing buffer.
Argument nil or omitted means kill the current buffer.  Return t if the
buffer is actually killed, nil otherwise.

The functions in `kill-buffer-query-functions' are called with the
buffer to be killed as the current buffer.  If any of them returns nil,
the buffer is not killed.  The hook `kill-buffer-hook' is run before the
buffer is actually killed.  The buffer being killed will be current
while the hook is running.  Functions called by any of these hooks are
supposed to not change the current buffer.  Neither hook is run for
internal or temporary buffers created by `get-buffer-create' or
`generate-new-buffer' with argument INHIBIT-BUFFER-HOOKS non-nil.

Any processes that have this buffer as the `process-buffer' are killed
with SIGHUP.  This function calls `replace-buffer-in-windows' for
cleaning up all windows currently displaying the buffer to be killed. */)
(Lisp_Object buffer_or_name)
{
  Lisp_Object buffer;
  struct buffer *b;
  Lisp_Object tem;
  struct Lisp_Marker *m;

  if (NILP (buffer_or_name))
    buffer = Fcurrent_buffer ();
  else
    buffer = Fget_buffer (buffer_or_name);
  if (NILP (buffer))
    nsberror (buffer_or_name);

  b = XBUFFER (buffer);

  if (!BUFFER_LIVE_P (b))
    return Qnil;

  TODO_NELISP_LATER;

  return nvim_buffer_kill (b) ? Qt : Qnil;
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

DEFUN ("generate-new-buffer-name", Fgenerate_new_buffer_name,
       Sgenerate_new_buffer_name, 1, 2, 0,
       doc: /* Return a string that is the name of no existing buffer based on NAME.
If there is no live buffer named NAME, then return NAME.
Otherwise modify name by appending `<NUMBER>', incrementing NUMBER
\(starting at 2) until an unused name is found, and then return that name.
Optional second argument IGNORE specifies a name that is okay to use (if
it is in the sequence to be tried) even if a buffer with that name exists.

If NAME begins with a space (i.e., a buffer that is not normally
visible to users), then if buffer NAME already exists a random number
is first appended to NAME, to speed up finding a non-existent buffer.  */)
(Lisp_Object name, Lisp_Object ignore)
{
  Lisp_Object genbase;

  CHECK_STRING (name);

  if ((!NILP (ignore) && !NILP (Fstring_equal (name, ignore)))
      || NILP (Fget_buffer (name)))
    return name;

  if (SREF (name, 0) != ' ') /* See bug#1229.  */
    genbase = name;
  else
    {
      enum
      {
        bug_52711 = true
      };
      char number[bug_52711 ? INT_BUFSIZE_BOUND (int) + 1 : sizeof "-999999"];
      EMACS_INT r = get_random ();
      eassume (0 <= r);
      int i = r % 1000000;
      AUTO_STRING_WITH_LEN (lnumber, number, sprintf (number, "-%d", i));
      genbase = concat2 (name, lnumber);
      if (NILP (Fget_buffer (genbase)))
        return genbase;
    }

  for (ptrdiff_t count = 2;; count++)
    {
      char number[INT_BUFSIZE_BOUND (ptrdiff_t) + sizeof "<>"];
      AUTO_STRING_WITH_LEN (lnumber, number,
                            sprintf (number, "<%" pD "d>", count));
      Lisp_Object gentemp = concat2 (genbase, lnumber);
      if (!NILP (Fstring_equal (gentemp, ignore))
          || NILP (Fget_buffer (gentemp)))
        return gentemp;
    }
}

DEFUN ("buffer-name", Fbuffer_name, Sbuffer_name, 0, 1, 0,
       doc: /* Return the name of BUFFER, as a string.
BUFFER defaults to the current buffer.
Return nil if BUFFER has been killed.  */)
(register Lisp_Object buffer) { return BVAR (decode_buffer (buffer), name); }

DEFUN ("buffer-file-name", Fbuffer_file_name, Sbuffer_file_name, 0, 1, 0,
       doc: /* Return name of file BUFFER is visiting, or nil if none.
No argument or nil as argument means use the current buffer.  */)
(register Lisp_Object buffer)
{
  return BVAR (decode_buffer (buffer), filename);
}

Lisp_Object
buffer_local_value (Lisp_Object variable, Lisp_Object buffer)
{
  register struct buffer *buf;
  register Lisp_Object result;
  struct Lisp_Symbol *sym;

  CHECK_SYMBOL (variable);
  CHECK_BUFFER (buffer);
  buf = XBUFFER (buffer);
  sym = XSYMBOL (variable);

start:
  switch (sym->u.s.redirect)
    {
    case SYMBOL_VARALIAS:
      sym = SYMBOL_ALIAS (sym);
      goto start;
    case SYMBOL_PLAINVAL:
      result = SYMBOL_VAL (sym);
      break;
    case SYMBOL_LOCALIZED:
      {
        struct Lisp_Buffer_Local_Value *blv = SYMBOL_BLV (sym);
        XSETSYMBOL (variable, sym);
        result = assq_no_quit (variable, BVAR (buf, local_var_alist));
        if (!NILP (result))
          {
            if (blv->fwd.fwdptr)
              {
                Lisp_Object current_alist_element = blv->valcell;

                TODO; // XSETCDR (current_alist_element,
                //          do_symval_forwarding (blv->fwd));
              }
            result = XCDR (result);
          }
        else
          result = Fdefault_value (variable);
        break;
      }
    case SYMBOL_FORWARDED:
      {
        lispfwd fwd = SYMBOL_FWD (sym);
        if (BUFFER_OBJFWDP (fwd))
          result = per_buffer_value (buf, XBUFFER_OBJFWD (fwd)->offset);
        else
          result = Fdefault_value (variable);
        break;
      }
    default:
      emacs_abort ();
    }

  return result;
}

DEFUN ("buffer-local-value", Fbuffer_local_value,
       Sbuffer_local_value, 2, 2, 0,
       doc: /* Return the value of VARIABLE in BUFFER.
If VARIABLE does not have a buffer-local binding in BUFFER, the value
is the default binding of the variable.  */)
(register Lisp_Object variable, register Lisp_Object buffer)
{
  register Lisp_Object result = buffer_local_value (variable, buffer);

  if (BASE_EQ (result, Qunbound))
    xsignal1 (Qvoid_variable, variable);

  return result;
}

DEFUN ("buffer-modified-p", Fbuffer_modified_p, Sbuffer_modified_p,
       0, 1, 0,
       doc: /* Return non-nil if BUFFER was modified since its file was last read or saved.
No argument or nil as argument means use current buffer as BUFFER.

If BUFFER was autosaved since it was last modified, this function
returns the symbol `autosaved'.  */)
(Lisp_Object buffer)
{
  struct buffer *buf = decode_buffer (buffer);
  TODO_NELISP_LATER;
  if (nvim_buffer_option_is_true (buf, "modified"))
    {
#if TODO_NELISP_LATER_AND
      if (BUF_AUTOSAVE_MODIFF (buf) == BUF_MODIFF (buf))
        return Qautosaved;
      else
#endif
        return Qt;
    }
  else
    return Qnil;
}

DEFUN ("buffer-enable-undo", Fbuffer_enable_undo, Sbuffer_enable_undo,
       0, 1, "",
       doc: /* Start keeping undo information for buffer BUFFER.
No argument or nil as argument means do this for the current buffer.  */)
(register Lisp_Object buffer)
{
  Lisp_Object real_buffer;

  if (NILP (buffer))
    XSETBUFFER (real_buffer, current_buffer);
  else
    {
      real_buffer = Fget_buffer (buffer);
      if (NILP (real_buffer))
        nsberror (buffer);
    }

  if (EQ (BVAR (XBUFFER (real_buffer), undo_list), Qt))
    bset_undo_list (XBUFFER (real_buffer), Qnil);

  return Qnil;
}

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

DEFUN ("set-buffer-modified-p", Fset_buffer_modified_p, Sset_buffer_modified_p,
       1, 1, 0,
       doc: /* Mark current buffer as modified or unmodified according to FLAG.
A non-nil FLAG means mark the buffer modified.
In addition, this function unconditionally forces redisplay of the
mode lines of the windows that display the current buffer, and also
locks or unlocks the file visited by the buffer, depending on whether
the function's argument is non-nil, but only if both `buffer-file-name'
and `buffer-file-truename' are non-nil.  */)
(Lisp_Object flag)
{
  Frestore_buffer_modified_p (flag);

  return Fforce_mode_line_update (Qnil);
}

DEFUN ("restore-buffer-modified-p", Frestore_buffer_modified_p,
       Srestore_buffer_modified_p, 1, 1, 0,
       doc: /* Like `set-buffer-modified-p', but doesn't redisplay buffer's mode line.
A nil FLAG means to mark the buffer as unmodified.  A non-nil FLAG
means mark the buffer as modified.  A special value of `autosaved'
will mark the buffer as modified and also as autosaved since it was
last modified.

This function also locks or unlocks the file visited by the buffer,
if both `buffer-file-truename' and `buffer-file-name' are non-nil.

It is not ensured that mode lines will be updated to show the modified
state of the current buffer.  Use with care.  */)
(Lisp_Object flag)
{
  TODO_NELISP_LATER;

  nvim_buffer_set_bool_option (current_buffer, "modified", !NILP (flag));

  return flag;
}

DEFUN ("set-buffer-multibyte", Fset_buffer_multibyte, Sset_buffer_multibyte,
       1, 1, 0,
       doc: /* Set the multibyte flag of the current buffer to FLAG.
If FLAG is t, this makes the buffer a multibyte buffer.
If FLAG is nil, this makes the buffer a single-byte buffer.
In these cases, the buffer contents remain unchanged as a sequence of
bytes but the contents viewed as characters do change.
If FLAG is `to', this makes the buffer a multibyte buffer by changing
all eight-bit bytes to eight-bit characters.
If the multibyte flag was really changed, undo information of the
current buffer is cleared.  */)
(Lisp_Object flag)
{
  if (NILP (flag) == NILP (BVAR (current_buffer, enable_multibyte_characters)))
    return flag;

  bset_enable_multibyte_characters (current_buffer, flag);

  TODO_NELISP_LATER;

  return flag;
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
  DEFSYM (Qchoice, "choice");
  DEFSYM (Qrange, "range");

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

  DEFVAR_PER_BUFFER ("default-directory", &MBVAR_ (current_buffer, directory),
        Qstringp,
        doc: /* Name of default directory of current buffer.
It should be an absolute directory name; on GNU and Unix systems,
these names start with "/" or "~" and end with "/".
To interactively change the default directory, use the command `cd'. */);

  DEFVAR_PER_BUFFER ("buffer-read-only", &MBVAR_ (current_buffer, read_only),
                     Qnil, doc : /* Non-nil if this buffer is read-only.  */);

  DEFVAR_PER_BUFFER ("buffer-undo-list", &MBVAR_ (current_buffer, undo_list), Qnil,
        doc: /* List of undo entries in current buffer.
Recent changes come first; older changes follow newer.

An entry (BEG . END) represents an insertion which begins at
position BEG and ends at position END.

An entry (TEXT . POSITION) represents the deletion of the string TEXT
from (abs POSITION).  If POSITION is positive, point was at the front
of the text being deleted; if negative, point was at the end.

An entry (t . TIMESTAMP), where TIMESTAMP is in the style of
`current-time', indicates that the buffer was previously unmodified;
TIMESTAMP is the visited file's modification time, as of that time.
If the modification time of the most recent save is different, this
entry is obsolete.

An entry (t . 0) means the buffer was previously unmodified but
its time stamp was unknown because it was not associated with a file.
An entry (t . -1) is similar, except that it means the buffer's visited
file did not exist.

An entry (nil PROPERTY VALUE BEG . END) indicates that a text property
was modified between BEG and END.  PROPERTY is the property name,
and VALUE is the old value.

An entry (apply FUN-NAME . ARGS) means undo the change with
\(apply FUN-NAME ARGS).

An entry (apply DELTA BEG END FUN-NAME . ARGS) supports selective undo
in the active region.  BEG and END is the range affected by this entry
and DELTA is the number of characters added or deleted in that range by
this change.

An entry (MARKER . DISTANCE) indicates that the marker MARKER
was adjusted in position by the offset DISTANCE (an integer).

An entry of the form POSITION indicates that point was at the buffer
location given by the integer.  Undoing an entry of this form places
point at POSITION.

Entries with value nil mark undo boundaries.  The undo command treats
the changes between two undo boundaries as a single step to be undone.

If the value of the variable is t, undo information is not recorded.  */);

  defsubr (&Sbuffer_list);
  defsubr (&Sget_buffer);
  defsubr (&Sget_buffer_create);
  defsubr (&Skill_buffer);
  defsubr (&Scurrent_buffer);
  defsubr (&Sset_buffer);
  defsubr (&Sgenerate_new_buffer_name);
  defsubr (&Sbuffer_name);
  defsubr (&Sbuffer_file_name);
  defsubr (&Sbuffer_local_value);
  defsubr (&Sforce_mode_line_update);
  defsubr (&Sset_buffer_modified_p);
  defsubr (&Srestore_buffer_modified_p);
  defsubr (&Sbuffer_modified_p);
  defsubr (&Sbuffer_enable_undo);
  defsubr (&Sset_buffer_multibyte);

  defsubr (&Smake_overlay);
  defsubr (&Sdelete_overlay);
  defsubr (&Soverlay_put);
}
