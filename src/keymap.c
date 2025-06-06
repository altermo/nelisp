#include "keymap.h"
#include "lisp.h"
#include "character.h"
#include "keyboard.h"
#include "puresize.h"
#include "termhooks.h"

Lisp_Object current_global_map;

static Lisp_Object exclude_keys;

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

void
syms_of_keymap (void)
{
  DEFSYM (Qkeymapp, "keymapp");
  DEFSYM (Qmenu_item, "menu-item");
  DEFSYM (Qmap_keymap_sorted, "map-keymap-sorted");

  DEFSYM (Qkeymap, "keymap");
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

  defsubr (&Skeymapp);
  defsubr (&Skeymap_parent);
  defsubr (&Sset_keymap_parent);
  defsubr (&Smake_keymap);
  defsubr (&Smake_sparse_keymap);
  defsubr (&Smap_keymap);
  defsubr (&Sdefine_key);
  defsubr (&Suse_global_map);
  defsubr (&Scurrent_global_map);
}
