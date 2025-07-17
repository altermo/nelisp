#include "keymap.h"
#include "lisp.h"
#include "character.h"
#include "commands.h"
#include "keyboard.h"
#include "puresize.h"
#include "termhooks.h"

Lisp_Object current_global_map;

static Lisp_Object exclude_keys;

static Lisp_Object where_is_cache;
static Lisp_Object where_is_cache_keymaps;

static Lisp_Object unicode_case_table;

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

DEFUN ("keymapp", Fkeymapp, Skeymapp, 1, 1, 0,
       doc: /* Return t if OBJECT is a keymap.

A keymap is a list (keymap . ALIST),
or a symbol whose function definition is itself a keymap.
ALIST elements look like (CHAR . DEFN) or (SYMBOL . DEFN);
a vector of densely packed bindings for small character codes
is also allowed as an element.  */)
(Lisp_Object object) { return (KEYMAPP (object) ? Qt : Qnil); }

static Lisp_Object
keymap_parent (Lisp_Object keymap, bool autoload)
{
  keymap = get_keymap (keymap, 1, autoload);

  Lisp_Object list = XCDR (keymap);
  for (; CONSP (list); list = XCDR (list))
    {
      if (KEYMAPP (list))
        return list;
    }

  return get_keymap (list, 0, autoload);
}

DEFUN ("keymap-parent", Fkeymap_parent, Skeymap_parent, 1, 1, 0,
       doc: /* Return the parent keymap of KEYMAP.
If KEYMAP has no parent, return nil.  */)
(Lisp_Object keymap) { return keymap_parent (keymap, 1); }

static bool
keymap_memberp (Lisp_Object map, Lisp_Object maps)
{
  if (NILP (map))
    return 0;
  while (KEYMAPP (maps) && !EQ (map, maps))
    maps = keymap_parent (maps, 0);
  return (EQ (map, maps));
}

DEFUN ("set-keymap-parent", Fset_keymap_parent, Sset_keymap_parent, 2, 2, 0,
       doc: /* Modify KEYMAP to set its parent map to PARENT.
Return PARENT.  PARENT should be nil or another keymap.  */)
(Lisp_Object keymap, Lisp_Object parent)
{
  where_is_cache = Qnil;
  where_is_cache_keymaps = Qt;

  keymap = get_keymap (keymap, 1, 1);

  if (!NILP (parent))
    {
      parent = get_keymap (parent, 1, 0);

      if (keymap_memberp (keymap, parent))
        error ("Cyclic keymap inheritance");
    }

  Lisp_Object prev = keymap;
  while (1)
    {
      Lisp_Object list = XCDR (prev);
      if (!CONSP (list) || KEYMAPP (list))
        {
          CHECK_IMPURE (prev, XCONS (prev));
          XSETCDR (prev, parent);
          return parent;
        }
      prev = list;
    }
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

static void
map_keymap_item (map_keymap_function_t fun, Lisp_Object args, Lisp_Object key,
                 Lisp_Object val, void *data)
{
  if (EQ (val, Qt))
    val = Qnil;
  (*fun) (key, val, args, data);
}

union map_keymap
{
  struct
  {
    map_keymap_function_t fun;
    Lisp_Object args;
    void *data;
  } s;
  GCALIGNED_UNION_MEMBER
};

static void
map_keymap_char_table_item (Lisp_Object args, Lisp_Object key, Lisp_Object val)
{
  if (!NILP (val))
    {
      if (CONSP (key))
        key = Fcons (XCAR (key), XCDR (key));
      union map_keymap *md = XFIXNUMPTR (args);
      map_keymap_item (md->s.fun, md->s.args, key, val, md->s.data);
    }
}

static Lisp_Object
map_keymap_internal (Lisp_Object map, map_keymap_function_t fun,
                     Lisp_Object args, void *data)
{
  Lisp_Object tail
    = (CONSP (map) && EQ (Qkeymap, XCAR (map))) ? XCDR (map) : map;

  for (; CONSP (tail) && !EQ (Qkeymap, XCAR (tail)); tail = XCDR (tail))
    {
      Lisp_Object binding = XCAR (tail);

      if (KEYMAPP (binding))
        break;
      else if (CONSP (binding))
        map_keymap_item (fun, args, XCAR (binding), XCDR (binding), data);
      else if (VECTORP (binding))
        {
          int len = ASIZE (binding);
          int c;
          for (c = 0; c < len; c++)
            {
              Lisp_Object character;
              XSETFASTINT (character, c);
              map_keymap_item (fun, args, character, AREF (binding, c), data);
            }
        }
      else if (CHAR_TABLE_P (binding))
        {
          union map_keymap mapdata = { { fun, args, data } };
          map_char_table (map_keymap_char_table_item, Qnil, binding,
                          make_pointer_integer (&mapdata));
        }
    }

  return tail;
}

static void
map_keymap_call (Lisp_Object key, Lisp_Object val, Lisp_Object fun, void *dummy)
{
  call2 (fun, key, val);
}

void
map_keymap (Lisp_Object map, map_keymap_function_t fun, Lisp_Object args,
            void *data, bool autoload)
{
  map = get_keymap (map, 1, autoload);
  while (CONSP (map))
    {
      if (KEYMAPP (XCAR (map)))
        {
          map_keymap (XCAR (map), fun, args, data, autoload);
          map = XCDR (map);
        }
      else
        map = map_keymap_internal (map, fun, args, data);
      if (!CONSP (map))
        map = get_keymap (map, 0, autoload);
    }
}

DEFUN ("map-keymap", Fmap_keymap, Smap_keymap, 2, 3, 0,
       doc: /* Call FUNCTION once for each event binding in KEYMAP.
FUNCTION is called with two arguments: the event that is bound, and
the definition it is bound to.  The event may be a character range.

If KEYMAP has a parent, the parent's bindings are included as well.
This works recursively: if the parent has itself a parent, then the
grandparent's bindings are also included and so on.

For more information, see Info node `(elisp) Keymaps'.

usage: (map-keymap FUNCTION KEYMAP)  */)
(Lisp_Object function, Lisp_Object keymap, Lisp_Object sort_first)
{
  if (!NILP (sort_first))
    return call2 (Qmap_keymap_sorted, function, keymap);

  map_keymap (keymap, map_keymap_call, function, NULL, 1);
  return Qnil;
}

static Lisp_Object
get_keyelt (Lisp_Object object, bool autoload)
{
  while (1)
    {
      if (!(CONSP (object)))
        return object;

      else if (EQ (XCAR (object), Qmenu_item))
        {
          if (CONSP (XCDR (object)))
            {
              Lisp_Object tem;

              object = XCDR (XCDR (object));
              tem = object;
              if (CONSP (object))
                object = XCAR (object);

              for (; CONSP (tem) && CONSP (XCDR (tem)); tem = XCDR (tem))
                if (EQ (XCAR (tem), QCfilter) && autoload)
                  {
                    Lisp_Object filter;
                    filter = XCAR (XCDR (tem));
                    filter = list2 (filter, list2 (Qquote, object));
                    TODO; // object = menu_item_eval_property (filter);
                    break;
                  }
            }
          else
            return object;
        }

      else if (STRINGP (XCAR (object)))
        object = XCDR (object);

      else
        return object;
    }
}

static Lisp_Object
access_keymap_1 (Lisp_Object map, Lisp_Object idx, bool t_ok, bool noinherit,
                 bool autoload)
{
  idx = EVENT_HEAD (idx);

  if (SYMBOLP (idx))
    idx = reorder_modifiers (idx);
  else if (FIXNUMP (idx))
    XSETFASTINT (idx, XFIXNUM (idx) & (CHAR_META | (CHAR_META - 1)));

  if (FIXNUMP (idx) && XFIXNAT (idx) & meta_modifier)
    {
      Lisp_Object event_meta_binding, event_meta_map;
      if (XFIXNUM (meta_prefix_char) & CHAR_META)
        meta_prefix_char = make_fixnum (27);
      event_meta_binding
        = access_keymap_1 (map, meta_prefix_char, t_ok, noinherit, autoload);
      event_meta_map = get_keymap (event_meta_binding, 0, autoload);
      if (CONSP (event_meta_map))
        {
          map = event_meta_map;
          idx = make_fixnum (XFIXNAT (idx) & ~meta_modifier);
        }
      else if (t_ok)
        idx = Qt;
      else
        return NILP (event_meta_binding) ? Qnil : Qunbound;
    }

  {
    Lisp_Object tail;
    Lisp_Object t_binding = Qunbound;
    UNUSED (t_binding);
    Lisp_Object retval = Qunbound;
    Lisp_Object retval_tail = Qnil;

    for (tail = (CONSP (map) && EQ (Qkeymap, XCAR (map))) ? XCDR (map) : map;
         (CONSP (tail)
          || (tail = get_keymap (tail, 0, autoload), CONSP (tail)));
         tail = XCDR (tail))
      {
        Lisp_Object val = Qunbound;
        Lisp_Object binding = XCAR (tail);
        Lisp_Object submap = get_keymap (binding, 0, autoload);

        if (EQ (binding, Qkeymap))
          {
            if (noinherit || NILP (retval))
              break;
            else if (!BASE_EQ (retval, Qunbound))
              {
                Lisp_Object parent_entry;
                eassert (KEYMAPP (retval));
                parent_entry
                  = get_keymap (access_keymap_1 (tail, idx, t_ok, 0, autoload),
                                0, autoload);
                if (KEYMAPP (parent_entry))
                  {
                    if (CONSP (retval_tail))
                      XSETCDR (retval_tail, parent_entry);
                    else
                      {
                        retval_tail = Fcons (retval, parent_entry);
                        retval = Fcons (Qkeymap, retval_tail);
                      }
                  }
                break;
              }
          }
        else if (CONSP (submap))
          {
            val = access_keymap_1 (submap, idx, t_ok, noinherit, autoload);
          }
        else if (CONSP (binding))
          {
            Lisp_Object key = XCAR (binding);

            if (EQ (key, idx))
              val = XCDR (binding);
            else if (t_ok && EQ (key, Qt))
              {
                t_binding = XCDR (binding);
                t_ok = 0;
              }
          }
        else if (VECTORP (binding))
          {
            if (FIXNUMP (idx) && XFIXNAT (idx) < ASIZE (binding))
              val = AREF (binding, XFIXNAT (idx));
          }
        else if (CHAR_TABLE_P (binding))
          {
            if (FIXNUMP (idx) && (XFIXNAT (idx) & CHAR_MODIFIER_MASK) == 0)
              {
                val = Faref (binding, idx);
                if (NILP (val))
                  val = Qunbound;
              }
          }

        if (!BASE_EQ (val, Qunbound))
          {
            if (EQ (val, Qt))
              val = Qnil;

            val = get_keyelt (val, autoload);

            if (!KEYMAPP (val))
              {
                if (NILP (retval) || BASE_EQ (retval, Qunbound))
                  retval = val;
                if (!NILP (val))
                  break;
              }
            else if (NILP (retval) || BASE_EQ (retval, Qunbound))
              retval = val;
            else if (CONSP (retval_tail))
              {
                XSETCDR (retval_tail, list1 (val));
                retval_tail = XCDR (retval_tail);
              }
            else
              {
                retval_tail = list1 (val);
                retval = Fcons (Qkeymap, Fcons (retval, retval_tail));
              }
          }
        maybe_quit ();
      }

    return BASE_EQ (Qunbound, retval) ? get_keyelt (t_binding, autoload)
                                      : retval;
  }
}
Lisp_Object
access_keymap (Lisp_Object map, Lisp_Object idx, bool t_ok, bool noinherit,
               bool autoload)
{
  Lisp_Object val = access_keymap_1 (map, idx, t_ok, noinherit, autoload);
  return BASE_EQ (val, Qunbound) ? Qnil : val;
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
    idx = reorder_modifiers (idx);
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

void
initial_define_lispy_key (Lisp_Object keymap, const char *keyname,
                          const char *defname)
{
  store_in_keymap (keymap, intern_c_string (keyname), intern_c_string (defname),
                   false);
}

static Lisp_Object
define_as_prefix (Lisp_Object keymap, Lisp_Object c)
{
  Lisp_Object cmd = Fmake_sparse_keymap (Qnil);
  store_in_keymap (keymap, c, cmd, false);

  return cmd;
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

static void
silly_event_symbol_error (Lisp_Object c)
{
  Lisp_Object parsed = parse_modifiers (c);
  int modifiers = XFIXNAT (XCAR (XCDR (parsed)));
  Lisp_Object base = XCAR (parsed);
  Lisp_Object name = Fsymbol_name (base);
  Lisp_Object assoc = Fassoc (name, exclude_keys, Qnil);

  if (!NILP (assoc))
    {
      TODO;
      UNUSED (modifiers);
    }
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
      Lisp_Object tmp = make_nil_vector (ASIZE (def));
      ptrdiff_t i = ASIZE (def);
      while (--i >= 0)
        {
          Lisp_Object defi = AREF (def, i);
          if (CONSP (defi) && lucid_event_type_list_p (defi))
            defi = Fevent_convert_list (defi);
          ASET (tmp, i, defi);
        }
      def = tmp;
    }

  key = possibly_translate_key_sequence (key, &length);

  ptrdiff_t idx = 0;
  while (1)
    {
      Lisp_Object c = Faref (key, make_fixnum (idx));

      if (CONSP (c))
        {
          if (lucid_event_type_list_p (c))
            c = Fevent_convert_list (c);
          else if (CHARACTERP (XCAR (c)))
            CHECK_CHARACTER_CDR (c);
        }

      if (SYMBOLP (c))
        silly_event_symbol_error (c);

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

      Lisp_Object cmd = access_keymap (keymap, c, 0, 1, 1);

      if (NILP (cmd))
        cmd = define_as_prefix (keymap, c);

      keymap = get_keymap (cmd, 0, 1);
      if (!CONSP (keymap))
        {
          const char *trailing_esc = ((EQ (c, meta_prefix_char) && metized)
                                        ? (idx == 0 ? "ESC" : " ESC")
                                        : "");
          UNUSED (trailing_esc);
          TODO;
        }
    }
}

static Lisp_Object
lookup_key_1 (Lisp_Object keymap, Lisp_Object key, Lisp_Object accept_default)
{
  bool t_ok = !NILP (accept_default);

  if (!CONSP (keymap) && !NILP (keymap))
    keymap = get_keymap (keymap, true, true);

  ptrdiff_t length = CHECK_VECTOR_OR_STRING (key);
  if (length == 0)
    return keymap;

  key = possibly_translate_key_sequence (key, &length);

  ptrdiff_t idx = 0;
  while (1)
    {
      Lisp_Object c = Faref (key, make_fixnum (idx++));

      if (CONSP (c) && lucid_event_type_list_p (c))
        TODO; // c = Fevent_convert_list (c);

      if (STRINGP (key) && XFIXNUM (c) & 0x80 && !STRING_MULTIBYTE (key))
        XSETINT (c, (XFIXNUM (c) | meta_modifier) & ~0x80);

      if (!FIXNUMP (c) && !SYMBOLP (c) && !CONSP (c) && !STRINGP (c))
        TODO; // message_with_string ("Key sequence contains invalid event %s",
              // c, 1);

      Lisp_Object cmd = access_keymap (keymap, c, t_ok, 0, 1);
      if (idx == length)
        return cmd;

      keymap = get_keymap (cmd, 0, 1);
      if (!CONSP (keymap))
        return make_fixnum (idx);

      maybe_quit ();
    }
}

DEFUN ("lookup-key", Flookup_key, Slookup_key, 2, 3, 0,
       doc: /* Look up key sequence KEY in KEYMAP.  Return the definition.
This is a legacy function; see `keymap-lookup' for the recommended
function to use instead.

A value of nil means undefined.  See doc of `define-key'
for kinds of definitions.

A number as value means KEY is "too long";
that is, characters or symbols in it except for the last one
fail to be a valid sequence of prefix characters in KEYMAP.
The number is how many characters at the front of KEY
it takes to reach a non-prefix key.
KEYMAP can also be a list of keymaps.

Normally, `lookup-key' ignores bindings for t, which act as default
bindings, used when nothing else in the keymap applies; this makes it
usable as a general function for probing keymaps.  However, if the
third optional argument ACCEPT-DEFAULT is non-nil, `lookup-key' will
recognize the default bindings, just as `read-key-sequence' does.  */)
(Lisp_Object keymap, Lisp_Object key, Lisp_Object accept_default)
{
  Lisp_Object found = lookup_key_1 (keymap, key, accept_default);
  if (!NILP (found) && !NUMBERP (found))
    return found;

  if (!VECTORP (key) || !(ASIZE (key) > 0) || !EQ (AREF (key, 0), Qmenu_bar))
    return found;

  if (NILP (unicode_case_table))
    {
      unicode_case_table = uniprop_table (Qlowercase);

      if (NILP (unicode_case_table))
        return found;
      staticpro (&unicode_case_table);
    }

  ptrdiff_t key_len = ASIZE (key);
  Lisp_Object new_key = make_vector (key_len, Qnil);

  Lisp_Object tables[2] = { unicode_case_table, Fcurrent_case_table () };
  for (int tbl_num = 0; tbl_num < 2; tbl_num++)
    {
      for (int i = 0; i < key_len; i++)
        {
          Lisp_Object item = AREF (key, i);
          if (!SYMBOLP (item))
            ASET (new_key, i, item);
          else
            {
              Lisp_Object key_item = Fsymbol_name (item);
              Lisp_Object new_item;
              if (!STRING_MULTIBYTE (key_item))
                new_item = Fdowncase (key_item);
              else
                {
                  USE_SAFE_ALLOCA;
                  ptrdiff_t size = SCHARS (key_item), n;
                  if (ckd_mul (&n, size, MAX_MULTIBYTE_LENGTH))
                    n = PTRDIFF_MAX;
                  unsigned char *dst = SAFE_ALLOCA (n);
                  unsigned char *p = dst;
                  ptrdiff_t j_char = 0, j_byte = 0;

                  while (j_char < size)
                    {
                      int ch = fetch_string_char_advance (key_item, &j_char,
                                                          &j_byte);
                      Lisp_Object ch_conv
                        = CHAR_TABLE_REF (tables[tbl_num], ch);
                      if (!NILP (ch_conv))
                        CHAR_STRING (XFIXNUM (ch_conv), p);
                      else
                        CHAR_STRING (ch, p);
                      p = dst + j_byte;
                    }
                  new_item
                    = make_multibyte_string ((char *) dst, SCHARS (key_item),
                                             SBYTES (key_item));
                  SAFE_FREE ();
                }
              ASET (new_key, i, Fintern (new_item, Qnil));
            }
        }

      found = lookup_key_1 (keymap, new_key, accept_default);
      if (!NILP (found) && !NUMBERP (found))
        break;

      for (int i = 0; i < key_len; i++)
        {
          if (!SYMBOLP (AREF (new_key, i)))
            continue;

          Lisp_Object lc_key = Fsymbol_name (AREF (new_key, i));

          if (!strstr (SSDATA (lc_key), " "))
            continue;

          USE_SAFE_ALLOCA;
          ptrdiff_t size = SCHARS (lc_key), n;
          if (ckd_mul (&n, size, MAX_MULTIBYTE_LENGTH))
            n = PTRDIFF_MAX;
          unsigned char *dst = SAFE_ALLOCA (n);

          memcpy (dst, SSDATA (lc_key), SBYTES (lc_key));
          for (int i = 0; i < SBYTES (lc_key); ++i)
            {
              if (dst[i] == ' ')
                dst[i] = '-';
            }
          Lisp_Object new_it
            = make_multibyte_string ((char *) dst, SCHARS (lc_key),
                                     SBYTES (lc_key));
          ASET (new_key, i, Fintern (new_it, Qnil));
          SAFE_FREE ();
        }

      found = lookup_key_1 (keymap, new_key, accept_default);
      if (!NILP (found) && !NUMBERP (found))
        break;
    }

  return found;
}

DEFUN ("use-global-map", Fuse_global_map, Suse_global_map, 1, 1, 0,
       doc: /* Select KEYMAP as the global keymap.  */)
(Lisp_Object keymap)
{
  keymap = get_keymap (keymap, 1, 1);
  current_global_map = keymap;

  return Qnil;
}

DEFUN ("current-global-map", Fcurrent_global_map, Scurrent_global_map, 0, 0, 0,
       doc: /* Return the current global keymap.  */)
(void) { return current_global_map; }

DEFUN ("key-description", Fkey_description, Skey_description, 1, 2, 0,
       doc: /* Return a pretty description of key-sequence KEYS.
Optional arg PREFIX is the sequence of keys leading up to KEYS.
For example, [?\\C-x ?l] is converted into the string \"C-x l\".

For an approximate inverse of this, see `kbd'.  */)
(Lisp_Object keys, Lisp_Object prefix)
{
  ptrdiff_t len = 0;
  Lisp_Object *args;
  EMACS_INT nkeys = XFIXNUM (Flength (keys));
  EMACS_INT nprefix = XFIXNUM (Flength (prefix));
  Lisp_Object sep = build_string (" ");
  bool add_meta = false;
  USE_SAFE_ALLOCA;

  ptrdiff_t size4;
  if (ckd_mul (&size4, nkeys + nprefix, 4))
    memory_full (SIZE_MAX);
  SAFE_ALLOCA_LISP (args, size4);

  Lisp_Object lists[2] = { prefix, keys };
  ptrdiff_t listlens[2] = { nprefix, nkeys };
  for (int li = 0; li < ARRAYELTS (lists); li++)
    {
      Lisp_Object list = lists[li];
      ptrdiff_t listlen = listlens[li], i_byte = 0;

      if (!(NILP (list) || STRINGP (list) || VECTORP (list) || CONSP (list)))
        wrong_type_argument (Qarrayp, list);

      for (ptrdiff_t i = 0; i < listlen;)
        {
          Lisp_Object key;
          if (STRINGP (list))
            {
              int c = fetch_string_char_advance (list, &i, &i_byte);
              if (!STRING_MULTIBYTE (list) && (c & 0200))
                c ^= 0200 | meta_modifier;
              key = make_fixnum (c);
            }
          else if (VECTORP (list))
            {
              key = AREF (list, i);
              i++;
            }
          else
            {
              key = XCAR (list);
              list = XCDR (list);
              i++;
            }

          if (add_meta)
            {
              if (!FIXNUMP (key) || EQ (key, meta_prefix_char)
                  || (XFIXNUM (key) & meta_modifier))
                {
                  args[len++]
                    = Fsingle_key_description (meta_prefix_char, Qnil);
                  args[len++] = sep;
                  if (EQ (key, meta_prefix_char))
                    continue;
                }
              else
                key = make_fixnum (XFIXNUM (key) | meta_modifier);
              add_meta = false;
            }
          else if (EQ (key, meta_prefix_char))
            {
              add_meta = true;
              continue;
            }
          args[len++] = Fsingle_key_description (key, Qnil);
          args[len++] = sep;
        }
    }

  Lisp_Object result;
  if (add_meta)
    {
      args[len] = Fsingle_key_description (meta_prefix_char, Qnil);
      result = Fconcat (len + 1, args);
    }
  else if (len == 0)
    result = empty_unibyte_string;
  else
    result = Fconcat (len - 1, args);
  SAFE_FREE ();
  return result;
}

char *
push_key_description (EMACS_INT ch, char *p)
{
  int c, c2;
  bool tab_as_ci;

  c = ch & (meta_modifier | ~-meta_modifier);
  c2 = c
       & ~(alt_modifier | ctrl_modifier | hyper_modifier | meta_modifier
           | shift_modifier | super_modifier);

  if (!CHARACTERP (make_fixnum (c2)))
    {
      p += sprintf (p, "[%d]", c);
      return p;
    }

  tab_as_ci = (c2 == '\t' && (c & meta_modifier));

  if (c & alt_modifier)
    {
      *p++ = 'A';
      *p++ = '-';
      c -= alt_modifier;
    }
  if ((c & ctrl_modifier) != 0
      || (c2 < ' ' && c2 != 27 && c2 != '\t' && c2 != Ctl ('M')) || tab_as_ci)
    {
      *p++ = 'C';
      *p++ = '-';
      c &= ~ctrl_modifier;
    }
  if (c & hyper_modifier)
    {
      *p++ = 'H';
      *p++ = '-';
      c -= hyper_modifier;
    }
  if (c & meta_modifier)
    {
      *p++ = 'M';
      *p++ = '-';
      c -= meta_modifier;
    }
  if (c & shift_modifier)
    {
      *p++ = 'S';
      *p++ = '-';
      c -= shift_modifier;
    }
  if (c & super_modifier)
    {
      *p++ = 's';
      *p++ = '-';
      c -= super_modifier;
    }
  if (c < 040)
    {
      if (c == 033)
        {
          *p++ = 'E';
          *p++ = 'S';
          *p++ = 'C';
        }
      else if (tab_as_ci)
        {
          *p++ = 'i';
        }
      else if (c == '\t')
        {
          *p++ = 'T';
          *p++ = 'A';
          *p++ = 'B';
        }
      else if (c == Ctl ('M'))
        {
          *p++ = 'R';
          *p++ = 'E';
          *p++ = 'T';
        }
      else
        {
          if (c > 0 && c <= Ctl ('Z'))
            *p++ = c + 0140;
          else
            *p++ = c + 0100;
        }
    }
  else if (c == 0177)
    {
      *p++ = 'D';
      *p++ = 'E';
      *p++ = 'L';
    }
  else if (c == ' ')
    {
      *p++ = 'S';
      *p++ = 'P';
      *p++ = 'C';
    }
  else if (c < 128)
    *p++ = c;
  else
    {
      p += CHAR_STRING (c, (unsigned char *) p);
    }

  return p;
}

DEFUN ("single-key-description", Fsingle_key_description,
       Ssingle_key_description, 1, 2, 0,
       doc: /* Return a pretty description of a character event KEY.
Control characters turn into C-whatever, etc.
Optional argument NO-ANGLES non-nil means don't put angle brackets
around function keys and event symbols.

See `text-char-description' for describing character codes.  */)
(Lisp_Object key, Lisp_Object no_angles)
{
  USE_SAFE_ALLOCA;

  if (CONSP (key) && lucid_event_type_list_p (key))
    key = Fevent_convert_list (key);

  if (CONSP (key) && FIXNUMP (XCAR (key)) && FIXNUMP (XCDR (key)))
    {
      AUTO_STRING (dot_dot, "..");
      return concat3 (Fsingle_key_description (XCAR (key), no_angles), dot_dot,
                      Fsingle_key_description (XCDR (key), no_angles));
    }

  key = EVENT_HEAD (key);

  if (FIXNUMP (key))
    {
      char tem[KEY_DESCRIPTION_SIZE];
      char *p = push_key_description (XFIXNUM (key), tem);
      *p = 0;
      return make_specified_string (tem, -1, p - tem, 1);
    }
  else if (SYMBOLP (key))
    {
      if (NILP (no_angles))
        {
          Lisp_Object namestr = SYMBOL_NAME (key);
          const char *sym = SSDATA (namestr);
          ptrdiff_t len = SBYTES (namestr);
          int i = 0;
          while (i < len - 3 && sym[i + 1] == '-' && strchr ("CMSsHA", sym[i]))
            i += 2;
          char *buffer = SAFE_ALLOCA (len + 3);
          memcpy (buffer, sym, i);
          buffer[i] = '<';
          memcpy (buffer + i + 1, sym + i, len - i);
          buffer[len + 1] = '>';
          buffer[len + 2] = '\0';
          Lisp_Object result = build_string (buffer);
          SAFE_FREE ();
          return result;
        }
      else
        return Fsymbol_name (key);
    }
  else if (STRINGP (key))
    return Fcopy_sequence (key);
  else
    error ("KEY must be an integer, cons, symbol, or string");
}

void
syms_of_keymap (void)
{
  DEFSYM (Qkeymapp, "keymapp");
  DEFSYM (Qmenu_item, "menu-item");
  DEFSYM (Qmap_keymap_sorted, "map-keymap-sorted");

  DEFSYM (Qkeymap, "keymap");
  DEFSYM (Qmode_line, "mode-line");
  Fput (Qkeymap, Qchar_table_extra_slots, make_fixnum (0));

  where_is_cache_keymaps = Qt;
  where_is_cache = Qnil;
  staticpro (&where_is_cache);
  staticpro (&where_is_cache_keymaps);

  exclude_keys = pure_list (pure_cons (build_pure_c_string ("DEL"),
                                       build_pure_c_string ("\\d")),
                            pure_cons (build_pure_c_string ("TAB"),
                                       build_pure_c_string ("\\t")),
                            pure_cons (build_pure_c_string ("RET"),
                                       build_pure_c_string ("\\r")),
                            pure_cons (build_pure_c_string ("ESC"),
                                       build_pure_c_string ("\\e")),
                            pure_cons (build_pure_c_string ("SPC"),
                                       build_pure_c_string (" ")));
  staticpro (&exclude_keys);

  DEFVAR_LISP ("minibuffer-local-map", Vminibuffer_local_map,
	       doc: /* Default keymap to use when reading from the minibuffer.  */);
  Vminibuffer_local_map = Fmake_sparse_keymap (Qnil);

  DEFVAR_LISP ("minor-mode-map-alist", Vminor_mode_map_alist,
        doc: /* Alist of keymaps to use for minor modes.
Each element looks like (VARIABLE . KEYMAP); KEYMAP is used to read
key sequences and look up bindings if VARIABLE's value is non-nil.
If two active keymaps bind the same key, the keymap appearing earlier
in the list takes precedence.  */);
  Vminor_mode_map_alist = Qnil;

  DEFSYM (Qmenu_bar, "menu-bar");

  defsubr (&Skeymapp);
  defsubr (&Skeymap_parent);
  defsubr (&Sset_keymap_parent);
  defsubr (&Smake_keymap);
  defsubr (&Smake_sparse_keymap);
  defsubr (&Smap_keymap);
  defsubr (&Sdefine_key);
  defsubr (&Slookup_key);
  defsubr (&Suse_global_map);
  defsubr (&Scurrent_global_map);
  defsubr (&Skey_description);
  defsubr (&Ssingle_key_description);
}
