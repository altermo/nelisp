#include "lisp.h"
#include "character.h"
#include "keyboard.h"
#include "puresize.h"
#include "termhooks.h"

static Lisp_Object where_is_cache;
static Lisp_Object where_is_cache_keymaps;

Lisp_Object
get_keymap (Lisp_Object object, bool error_if_not_keymap, bool autoload)
{
autoload_retry:
  if (NILP (object))
    goto end;
  if (CONSP (object) && EQ (XCAR (object), Qkeymap))
    return object;

  Lisp_Object tem = indirect_function (object);
  if (CONSP (tem))
    {
      if (EQ (XCAR (tem), Qkeymap))
        return tem;

      if ((autoload || !error_if_not_keymap) && EQ (XCAR (tem), Qautoload)
          && SYMBOLP (object))
        {
          Lisp_Object tail;

          tail = Fnth (make_fixnum (4), tem);
          if (EQ (tail, Qkeymap))
            {
              if (autoload)
                {
                  TODO; // Fautoload_do_load (tem, object, Qnil);
                  goto autoload_retry;
                }
              else
                return object;
            }
        }
    }

end:
  if (error_if_not_keymap)
    wrong_type_argument (Qkeymapp, object);
  return Qnil;
}

DEFUN ("make-keymap", Fmake_keymap, Smake_keymap, 0, 1, 0,
       doc: /* Construct and return a new keymap, of the form (keymap CHARTABLE . ALIST).
CHARTABLE is a char-table that holds the bindings for all characters
without modifiers.  All entries in it are initially nil, meaning
"command undefined".  ALIST is an assoc-list which holds bindings for
function keys, mouse events, and any other things that appear in the
input stream.  Initially, ALIST is nil.

The optional arg STRING supplies a menu name for the keymap
in case you use it as a menu with `x-popup-menu'.  */)
(Lisp_Object string)
{
  Lisp_Object tail = !NILP (string) ? list1 (string) : Qnil;
  return Fcons (Qkeymap, Fcons (Fmake_char_table (Qkeymap, Qnil), tail));
}

DEFUN ("make-sparse-keymap", Fmake_sparse_keymap, Smake_sparse_keymap, 0, 1, 0,
       doc: /* Construct and return a new sparse keymap.
Its car is `keymap' and its cdr is an alist of (CHAR . DEFINITION),
which binds the character CHAR to DEFINITION, or (SYMBOL . DEFINITION),
which binds the function key or mouse event SYMBOL to DEFINITION.
Initially the alist is nil.

The optional arg STRING supplies a menu name for the keymap
in case you use it as a menu with `x-popup-menu'.  */)
(Lisp_Object string)
{
  if (!NILP (string))
    {
      if (!NILP (Vpurify_flag))
        string = Fpurecopy (string);
      return list2 (Qkeymap, string);
    }
  return list1 (Qkeymap);
}

static Lisp_Object
store_in_keymap (Lisp_Object keymap, register Lisp_Object idx, Lisp_Object def,
                 bool remove)
{
  where_is_cache = Qnil;
  where_is_cache_keymaps = Qt;

  if (EQ (idx, Qkeymap))
    error ("`keymap' is reserved for embedded parent maps");

  if (CONSP (def) && PURE_P (XCONS (def))
      && (EQ (XCAR (def), Qmenu_item) || STRINGP (XCAR (def))))
    def = Fcons (XCAR (def), XCDR (def));

  if (!CONSP (keymap) || !EQ (XCAR (keymap), Qkeymap))
    error ("attempt to define a key in a non-keymap");

  if (CONSP (idx) && CHARACTERP (XCAR (idx)))
    CHECK_CHARACTER_CDR (idx);
  else
    idx = EVENT_HEAD (idx);

  if (SYMBOLP (idx))
    TODO; // idx = reorder_modifiers (idx);
  else if (FIXNUMP (idx))
    XSETFASTINT (idx, XFIXNUM (idx) & (CHAR_META | (CHAR_META - 1)));

  {
    Lisp_Object tail;

    Lisp_Object insertion_point = keymap;
    for (tail = XCDR (keymap); CONSP (tail); tail = XCDR (tail))
      {
        Lisp_Object elt = XCAR (tail);
        if (VECTORP (elt))
          {
            if (FIXNATP (idx) && XFIXNAT (idx) < ASIZE (elt))
              {
                CHECK_IMPURE (elt, XVECTOR (elt));
                ASET (elt, XFIXNAT (idx), def);
                return def;
              }
            else if (CONSP (idx) && CHARACTERP (XCAR (idx)))
              {
                int from = XFIXNAT (XCAR (idx));
                int to = XFIXNAT (XCDR (idx));

                if (to >= ASIZE (elt))
                  to = ASIZE (elt) - 1;
                for (; from <= to; from++)
                  ASET (elt, from, def);
                if (to == XFIXNAT (XCDR (idx)))
                  return def;
              }
            insertion_point = tail;
          }
        else if (CHAR_TABLE_P (elt))
          {
            Lisp_Object sdef = def;
            if (remove)
              sdef = Qnil;
            else if (NILP (sdef))
              sdef = Qt;

            if (FIXNATP (idx) && !(XFIXNAT (idx) & CHAR_MODIFIER_MASK))
              {
                Faset (elt, idx, sdef);
                return def;
              }
            else if (CONSP (idx) && CHARACTERP (XCAR (idx)))
              {
                Fset_char_table_range (elt, idx, sdef);
                return def;
              }
            insertion_point = tail;
          }
        else if (CONSP (elt))
          {
            if (EQ (Qkeymap, XCAR (elt)))
              {
                tail = insertion_point = elt;
              }
            else if (EQ (idx, XCAR (elt)))
              {
                CHECK_IMPURE (elt, XCONS (elt));
                if (remove)
                  insertion_point = Fdelq (elt, insertion_point);
                else
                  XSETCDR (elt, def);
                return def;
              }
            else if (CONSP (idx) && CHARACTERP (XCAR (idx))
                     && CHARACTERP (XCAR (elt)))
              {
                int from = XFIXNAT (XCAR (idx));
                int to = XFIXNAT (XCDR (idx));

                if (from <= XFIXNAT (XCAR (elt)) && to >= XFIXNAT (XCAR (elt)))
                  {
                    if (remove)
                      insertion_point = Fdelq (elt, insertion_point);
                    else
                      XSETCDR (elt, def);
                    if (from == to)
                      return def;
                  }
              }
          }
        else if (EQ (elt, Qkeymap))
          goto keymap_end;

        maybe_quit ();
      }

  keymap_end:
    if (!remove)
      {
        Lisp_Object elt;

        if (CONSP (idx) && CHARACTERP (XCAR (idx)))
          {
            elt = Fmake_char_table (Qkeymap, Qnil);
            Fset_char_table_range (elt, idx, NILP (def) ? Qt : def);
          }
        else
          elt = Fcons (idx, def);
        CHECK_IMPURE (insertion_point, XCONS (insertion_point));
        XSETCDR (insertion_point, Fcons (elt, XCDR (insertion_point)));
      }
  }

  return def;
}

static Lisp_Object
possibly_translate_key_sequence (Lisp_Object key, ptrdiff_t *length)
{
  if (VECTORP (key) && ASIZE (key) == 1 && STRINGP (AREF (key, 0)))
    {
      UNUSED (length);
      TODO;
    }

  return key;
}

DEFUN ("define-key", Fdefine_key, Sdefine_key, 3, 4, 0,
       doc: /* In KEYMAP, define key sequence KEY as DEF.
This is a legacy function; see `keymap-set' for the recommended
function to use instead.

KEYMAP is a keymap.

KEY is a string or a vector of symbols and characters, representing a
sequence of keystrokes and events.  Non-ASCII characters with codes
above 127 (such as ISO Latin-1) can be represented by vectors.
Two types of vector have special meanings:
 [remap COMMAND] remaps any key binding for COMMAND.
 [t] creates a default definition, which applies to any event with no
    other definition in KEYMAP.

DEF is anything that can be a key's definition:
 nil (means key is undefined in this keymap),
 a command (a Lisp function suitable for interactive calling),
 a string (treated as a keyboard macro),
 a keymap (to define a prefix key),
 a symbol (when the key is looked up, the symbol will stand for its
    function definition, which should at that time be one of the above,
    or another symbol whose function definition is used, etc.),
 a cons (STRING . DEFN), meaning that DEFN is the definition
    (DEFN should be a valid definition in its own right) and
    STRING is the menu item name (which is used only if the containing
    keymap has been created with a menu name, see `make-keymap'),
 or a cons (MAP . CHAR), meaning use definition of CHAR in keymap MAP,
 or an extended menu item definition.
 (See info node `(elisp)Extended Menu Items'.)

If REMOVE is non-nil, the definition will be removed.  This is almost
the same as setting the definition to nil, but makes a difference if
the KEYMAP has a parent, and KEY is shadowing the same binding in the
parent.  With REMOVE, subsequent lookups will return the binding in
the parent, and with a nil DEF, the lookups will return nil.

If KEYMAP is a sparse keymap with a binding for KEY, the existing
binding is altered.  If there is no binding for KEY, the new pair
binding KEY to DEF is added at the front of KEYMAP.  */)
(Lisp_Object keymap, Lisp_Object key, Lisp_Object def, Lisp_Object remove)
{
  bool metized = false;

  keymap = get_keymap (keymap, 1, 1);

  ptrdiff_t length = CHECK_VECTOR_OR_STRING (key);
  if (length == 0)
    return Qnil;

  int meta_bit = (VECTORP (key) || (STRINGP (key) && STRING_MULTIBYTE (key))
                    ? meta_modifier
                    : 0x80);

  if (VECTORP (def) && ASIZE (def) > 0 && CONSP (AREF (def, 0)))
    {
      TODO;
    }

  key = possibly_translate_key_sequence (key, &length);

  ptrdiff_t idx = 0;
  while (1)
    {
      Lisp_Object c = Faref (key, make_fixnum (idx));

      if (CONSP (c))
        {
          TODO;
        }

      if (SYMBOLP (c))
        TODO; // silly_event_symbol_error (c);

      if (FIXNUMP (c) && (XFIXNUM (c) & meta_bit) && !metized)
        {
          c = meta_prefix_char;
          metized = true;
        }
      else
        {
          if (FIXNUMP (c))
            XSETINT (c, XFIXNUM (c) & ~meta_bit);

          metized = false;
          idx++;
        }

      if (!FIXNUMP (c) && !SYMBOLP (c)
          && (!CONSP (c) || (FIXNUMP (XCAR (c)) && idx != length)))
        TODO; // message_with_string ("Key sequence contains invalid event %s",
              // c, 1);

      UNUSED (remove);
      UNUSED (length);
      if (idx == length)
        return store_in_keymap (keymap, c, def, !NILP (remove));

      TODO;
    }
}

void
syms_of_keymap (void)
{
  DEFSYM (Qkeymapp, "keymapp");
  DEFSYM (Qmenu_item, "menu-item");

  DEFSYM (Qkeymap, "keymap");
  Fput (Qkeymap, Qchar_table_extra_slots, make_fixnum (0));

  where_is_cache_keymaps = Qt;
  where_is_cache = Qnil;
  staticpro (&where_is_cache);
  staticpro (&where_is_cache_keymaps);

  defsubr (&Smake_keymap);
  defsubr (&Smake_sparse_keymap);
  defsubr (&Sdefine_key);
}
