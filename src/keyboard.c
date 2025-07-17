#include "lisp.h"
#include "commands.h"
#include "keymap.h"
#include "lua.h"
#include "termhooks.h"

EMACS_INT command_loop_level;

int
make_ctrl_char (int c)
{
  int upper = c & ~0177;

  if (!ASCII_CHAR_P (c))
    return c |= ctrl_modifier;

  c &= 0177;

  if (c >= 0100 && c < 0140)
    {
      int oc = c;
      c &= ~0140;
      if (oc >= 'A' && oc <= 'Z')
        c |= shift_modifier;
    }

  else if (c >= 'a' && c <= 'z')
    c &= ~0140;

  else if (c >= ' ')
    c |= ctrl_modifier;

  c |= (upper & ~ctrl_modifier);

  return c;
}

int
parse_solitary_modifier (Lisp_Object symbol)
{
  Lisp_Object name;

  if (!SYMBOLP (symbol))
    return 0;

  name = SYMBOL_NAME (symbol);

  switch (SREF (name, 0))
    {
#define SINGLE_LETTER_MOD(BIT) \
  if (SBYTES (name) == 1)      \
    return BIT;

#define MULTI_LETTER_MOD(BIT, NAME, LEN)                         \
  if (LEN == SBYTES (name) && !memcmp (SDATA (name), NAME, LEN)) \
    return BIT;

    case 'A':
      SINGLE_LETTER_MOD (alt_modifier);
      break;

    case 'a':
      MULTI_LETTER_MOD (alt_modifier, "alt", 3);
      break;

    case 'C':
      SINGLE_LETTER_MOD (ctrl_modifier);
      break;

    case 'c':
      MULTI_LETTER_MOD (ctrl_modifier, "ctrl", 4);
      MULTI_LETTER_MOD (ctrl_modifier, "control", 7);
      MULTI_LETTER_MOD (click_modifier, "click", 5);
      break;

    case 'H':
      SINGLE_LETTER_MOD (hyper_modifier);
      break;

    case 'h':
      MULTI_LETTER_MOD (hyper_modifier, "hyper", 5);
      break;

    case 'M':
      SINGLE_LETTER_MOD (meta_modifier);
      break;

    case 'm':
      MULTI_LETTER_MOD (meta_modifier, "meta", 4);
      break;

    case 'S':
      SINGLE_LETTER_MOD (shift_modifier);
      break;

    case 's':
      MULTI_LETTER_MOD (shift_modifier, "shift", 5);
      MULTI_LETTER_MOD (super_modifier, "super", 5);
      SINGLE_LETTER_MOD (super_modifier);
      break;

    case 'd':
      MULTI_LETTER_MOD (drag_modifier, "drag", 4);
      MULTI_LETTER_MOD (down_modifier, "down", 4);
      MULTI_LETTER_MOD (double_modifier, "double", 6);
      break;

    case 't':
      MULTI_LETTER_MOD (triple_modifier, "triple", 6);
      break;

    case 'u':
      MULTI_LETTER_MOD (up_modifier, "up", 2);
      break;

#undef SINGLE_LETTER_MOD
#undef MULTI_LETTER_MOD
    }

  return 0;
}

bool
lucid_event_type_list_p (Lisp_Object object)
{
  if (!CONSP (object))
    return false;

  if (EQ (XCAR (object), Qhelp_echo) || EQ (XCAR (object), Qvertical_line)
      || EQ (XCAR (object), Qmode_line) || EQ (XCAR (object), Qtab_line)
      || EQ (XCAR (object), Qheader_line))
    return false;

  Lisp_Object tail = object;
  FOR_EACH_TAIL_SAFE (object)
    {
      Lisp_Object elt = XCAR (object);
      if (!(FIXNUMP (elt) || SYMBOLP (elt)))
        return false;
      tail = XCDR (object);
    }

  return NILP (tail);
}

static Lisp_Object
apply_modifiers_uncached (int modifiers, char *base, int base_len,
                          int base_len_byte)
{
  char new_mods[sizeof "A-C-H-M-S-s-up-down-drag-double-triple-"];
  int mod_len;

  {
    char *p = new_mods;

    if (modifiers & alt_modifier)
      {
        *p++ = 'A';
        *p++ = '-';
      }
    if (modifiers & ctrl_modifier)
      {
        *p++ = 'C';
        *p++ = '-';
      }
    if (modifiers & hyper_modifier)
      {
        *p++ = 'H';
        *p++ = '-';
      }
    if (modifiers & meta_modifier)
      {
        *p++ = 'M';
        *p++ = '-';
      }
    if (modifiers & shift_modifier)
      {
        *p++ = 'S';
        *p++ = '-';
      }
    if (modifiers & super_modifier)
      {
        *p++ = 's';
        *p++ = '-';
      }
    if (modifiers & double_modifier)
      p = stpcpy (p, "double-");
    if (modifiers & triple_modifier)
      p = stpcpy (p, "triple-");
    if (modifiers & up_modifier)
      p = stpcpy (p, "up-");
    if (modifiers & down_modifier)
      p = stpcpy (p, "down-");
    if (modifiers & drag_modifier)
      p = stpcpy (p, "drag-");

    *p = '\0';

    mod_len = p - new_mods;
  }

  {
    Lisp_Object new_name;

    new_name = make_uninit_multibyte_string (mod_len + base_len,
                                             mod_len + base_len_byte);
    memcpy (SDATA (new_name), new_mods, mod_len);
    memcpy (SDATA (new_name) + mod_len, base, base_len_byte);

    return Fintern (new_name, Qnil);
  }
}

static Lisp_Object
apply_modifiers (int modifiers, Lisp_Object base)
{
  Lisp_Object cache, idx, entry, new_symbol;

  modifiers &= INTMASK;

  if (FIXNUMP (base))
    return make_fixnum (XFIXNUM (base) | modifiers);

  cache = Fget (base, Qmodifier_cache);
  XSETFASTINT (idx, (modifiers & ~click_modifier));
  entry = assq_no_quit (idx, cache);

  if (CONSP (entry))
    new_symbol = XCDR (entry);
  else
    {
      new_symbol
        = apply_modifiers_uncached (modifiers, SSDATA (SYMBOL_NAME (base)),
                                    SCHARS (SYMBOL_NAME (base)),
                                    SBYTES (SYMBOL_NAME (base)));

      entry = Fcons (idx, new_symbol);
      Fput (base, Qmodifier_cache, Fcons (entry, cache));
    }

  if (NILP (Fget (new_symbol, Qevent_kind)))
    {
      Lisp_Object kind;

      kind = Fget (base, Qevent_kind);
      if (!NILP (kind))
        Fput (new_symbol, Qevent_kind, kind);
    }

  return new_symbol;
}

DEFUN ("event-convert-list", Fevent_convert_list, Sevent_convert_list, 1, 1, 0,
       doc: /* Convert the event description list EVENT-DESC to an event type.
EVENT-DESC should contain one base event type (a character or symbol)
and zero or more modifier names (control, meta, hyper, super, shift, alt,
drag, down, double or triple).  The base must be last.

The return value is an event type (a character or symbol) which has
essentially the same base event type and all the specified modifiers.
(Some compatibility base types, like symbols that represent a
character, are not returned verbatim.)  */)
(Lisp_Object event_desc)
{
  Lisp_Object base = Qnil;
  int modifiers = 0;

  FOR_EACH_TAIL_SAFE (event_desc)
    {
      Lisp_Object elt = XCAR (event_desc);
      int this = 0;

      if (SYMBOLP (elt) && CONSP (XCDR (event_desc)))
        this = parse_solitary_modifier (elt);

      if (this != 0)
        modifiers |= this;
      else if (!NILP (base))
        error ("Two bases given in one event");
      else
        base = elt;
    }

  if (SYMBOLP (base) && SCHARS (SYMBOL_NAME (base)) == 1)
    XSETINT (base, SREF (SYMBOL_NAME (base), 0));

  if (FIXNUMP (base))
    {
      if ((modifiers & shift_modifier) != 0
          && (XFIXNUM (base) >= 'a' && XFIXNUM (base) <= 'z'))
        {
          XSETINT (base, XFIXNUM (base) - ('a' - 'A'));
          modifiers &= ~shift_modifier;
        }

      if (modifiers & ctrl_modifier)
        return make_fixnum ((modifiers & ~ctrl_modifier)
                            | make_ctrl_char (XFIXNUM (base)));
      else
        return make_fixnum (modifiers | XFIXNUM (base));
    }
  else if (SYMBOLP (base))
    return apply_modifiers (modifiers, base);
  else
    error ("Invalid base event");
}

static Lisp_Object
cmd_error (Lisp_Object data)
{
  TODO_NELISP_LATER;
  tcall_error = true;
  Lisp_Object err_symbol = Fcar (data);
  Lisp_Object message = Fget (err_symbol, Qerror_message);
  Lisp_Object tail = Fcdr_safe (data);
  char *errmsg = "nil";
  if (STRINGP (message))
    errmsg = (char *) SDATA (message);
  char *extra = "";
  if (SYMBOLP (tail))
    extra = (char *) SDATA (SYMBOL_NAME (tail));
  if (SYMBOLP (Fcar_safe (tail)))
    extra = (char *) SDATA (SYMBOL_NAME (Fcar_safe (tail)));
  LUAC (5, 1) { lua_pushfstring (L, "(nelisp): %s (%s)", errmsg, extra); }
  return make_fixnum (0);
}

static Lisp_Object
command_loop_1 (void)
{
  while (1)
    {
      mtx_lock (&main_mutex);
      cnd_signal (&main_cond);
      mtx_unlock (&main_mutex);
      cnd_wait (&thread_cond, &thread_mutex);

      eassert (main_func);
      main_func ();
      main_func = NULL;
    }

  __builtin_unreachable ();
}
Lisp_Object
command_loop_2 (Lisp_Object handlers)
{
  register Lisp_Object val;

  UNUSED (handlers);
  do
    val = internal_condition_case (command_loop_1, handlers, cmd_error);
  while (!NILP (val));

  return Qnil;
}
Lisp_Object
command_loop (void)
{
#if TODO_NELISP_LATER_AND
  if (command_loop_level > 0 || minibuf_level > 0)
    {
      Lisp_Object val;
      val = internal_catch (Qexit, command_loop_2, Qerror);
      executing_kbd_macro = Qnil;
      return val;
    }
  else
#endif
    while (1)
      {
        // internal_catch (Qtop_level, top_level_1, Qnil);
        internal_catch (Qtop_level, command_loop_2, Qerror);
        // executing_kbd_macro = Qnil;
        //
        // if (noninteractive)
        //     Fkill_emacs (Qt, Qnil);
      }
}

Lisp_Object
recursive_edit_1 (void)
{
  specpdl_ref count = SPECPDL_INDEX ();
  Lisp_Object val;

  if (command_loop_level > 0)
    {
      TODO;
    }

#if TODO_NELISP_LATER_AND
  specbind (Qinhibit_redisplay, Qnil);
  redisplaying_p = 0;
  specbind (Qundo_auto__undoably_changed_buffers, Qnil);
#endif

  val = command_loop ();
  UNUSED (val);
#if TODO_NELISP_LATER_AND
  if (EQ (val, Qt))
    quit ();
  if (STRINGP (val))
    xsignal1 (Qerror, val);

  if (FUNCTIONP (val))
    call0 (val);
#endif

  return unbind_to (count, Qnil);
}

DEFUN ("recursive-edit", Frecursive_edit, Srecursive_edit, 0, 0, "",
       doc: /* Invoke the editor command loop recursively.
To get out of the recursive edit, a command can throw to `exit' -- for
instance (throw \\='exit nil).

The following values (last argument to `throw') can be used when
throwing to \\='exit:

- t causes `recursive-edit' to quit, so that control returns to the
  command loop one level up.

- A string causes `recursive-edit' to signal an error, printing that
  string as the error message.

- A function causes `recursive-edit' to call that function with no
  arguments, and then return normally.

- Any other value causes `recursive-edit' to return normally to the
  function that called it.

This function is called by the editor initialization to begin editing.  */)
(void)
{
  specpdl_ref count = SPECPDL_INDEX ();
#if TODO_NELISP_LATER_AND
  Lisp_Object buffer;

  if (input_blocked_p ())
    return Qnil;

  if (command_loop_level >= 0
      && current_buffer != XBUFFER (XWINDOW (selected_window)->contents))
    buffer = Fcurrent_buffer ();
  else
    buffer = Qnil;
#endif

  command_loop_level++;
#if TODO_NELISP_LATER_AND
  update_mode_lines = 17;
  record_unwind_protect (recursive_edit_unwind, buffer);

  if (command_loop_level > 0)
    temporarily_switch_to_single_kboard (SELECTED_FRAME ());
#endif

  recursive_edit_1 ();
  return unbind_to (count, Qnil);
}

static int
parse_modifiers_uncached (Lisp_Object symbol, ptrdiff_t *modifier_end)
{
  Lisp_Object name;
  ptrdiff_t i;
  int modifiers;

  CHECK_SYMBOL (symbol);

  modifiers = 0;
  name = SYMBOL_NAME (symbol);

  for (i = 0; i < SBYTES (name) - 1;)
    {
      ptrdiff_t this_mod_end = 0;
      int this_mod = 0;

      switch (SREF (name, i))
        {
#define SINGLE_LETTER_MOD(BIT) (this_mod_end = i + 1, this_mod = BIT)

        case 'A':
          SINGLE_LETTER_MOD (alt_modifier);
          break;

        case 'C':
          SINGLE_LETTER_MOD (ctrl_modifier);
          break;

        case 'H':
          SINGLE_LETTER_MOD (hyper_modifier);
          break;

        case 'M':
          SINGLE_LETTER_MOD (meta_modifier);
          break;

        case 'S':
          SINGLE_LETTER_MOD (shift_modifier);
          break;

        case 's':
          SINGLE_LETTER_MOD (super_modifier);
          break;

#undef SINGLE_LETTER_MOD

#define MULTI_LETTER_MOD(BIT, NAME, LEN)                                     \
  if (i + LEN + 1 <= SBYTES (name) && !memcmp (SDATA (name) + i, NAME, LEN)) \
    {                                                                        \
      this_mod_end = i + LEN;                                                \
      this_mod = BIT;                                                        \
    }

        case 'd':
          MULTI_LETTER_MOD (drag_modifier, "drag", 4);
          MULTI_LETTER_MOD (down_modifier, "down", 4);
          MULTI_LETTER_MOD (double_modifier, "double", 6);
          break;

        case 't':
          MULTI_LETTER_MOD (triple_modifier, "triple", 6);
          break;

        case 'u':
          MULTI_LETTER_MOD (up_modifier, "up", 2);
          break;
#undef MULTI_LETTER_MOD
        }

      if (this_mod_end == 0)
        break;

      if (this_mod_end >= SBYTES (name) || SREF (name, this_mod_end) != '-')
        break;

      modifiers |= this_mod;
      i = this_mod_end + 1;
    }

  if (!(modifiers
        & (down_modifier | drag_modifier | double_modifier | triple_modifier))
      && i + 7 == SBYTES (name) && memcmp (SDATA (name) + i, "mouse-", 6) == 0
      && ('0' <= SREF (name, i + 6) && SREF (name, i + 6) <= '9'))
    modifiers |= click_modifier;

  if (!(modifiers & (double_modifier | triple_modifier))
      && i + 6 < SBYTES (name) && memcmp (SDATA (name) + i, "wheel-", 6) == 0)
    modifiers |= click_modifier;

  if (modifier_end)
    *modifier_end = i;

  return modifiers;
}

static const char *const modifier_names[]
  = { "up", "down", "drag", "click", "double", "triple", 0,         0,     0, 0,
      0,    0,      0,      0,       0,        0,        0,         0,     0, 0,
      0,    0,      "alt",  "super", "hyper",  "shift",  "control", "meta" };
#define NUM_MOD_NAMES ARRAYELTS (modifier_names)

static Lisp_Object modifier_symbols;

static Lisp_Object
lispy_modifier_list (int modifiers)
{
  Lisp_Object modifier_list;
  unsigned long i;

  modifier_list = Qnil;
  for (i = 0; (1 << i) <= modifiers && i < NUM_MOD_NAMES; i++)
    if (modifiers & (1 << i))
      modifier_list = Fcons (AREF (modifier_symbols, i), modifier_list);

  return modifier_list;
}

#define KEY_TO_CHAR(k) (XFIXNUM (k) & ((1 << CHARACTERBITS) - 1))

Lisp_Object
parse_modifiers (Lisp_Object symbol)
{
  Lisp_Object elements;

  if (FIXNUMP (symbol))
    return list2i (KEY_TO_CHAR (symbol), XFIXNUM (symbol) & CHAR_MODIFIER_MASK);
  else if (!SYMBOLP (symbol))
    return Qnil;

  elements = Fget (symbol, Qevent_symbol_element_mask);
  if (CONSP (elements))
    return elements;
  else
    {
      ptrdiff_t end;
      int modifiers = parse_modifiers_uncached (symbol, &end);
      Lisp_Object unmodified;
      Lisp_Object mask;

      unmodified = Fintern (make_string (SSDATA (SYMBOL_NAME (symbol)) + end,
                                         SBYTES (SYMBOL_NAME (symbol)) - end),
                            Qnil);

      if (modifiers & ~INTMASK)
        emacs_abort ();
      XSETFASTINT (mask, modifiers);
      elements = list2 (unmodified, mask);

      Fput (symbol, Qevent_symbol_element_mask, elements);
      Fput (symbol, Qevent_symbol_elements,
            Fcons (unmodified, lispy_modifier_list (modifiers)));

      return elements;
    }
}

Lisp_Object
reorder_modifiers (Lisp_Object symbol)
{
  Lisp_Object parsed;

  parsed = parse_modifiers (symbol);
  return apply_modifiers (XFIXNAT (XCAR (XCDR (parsed))), XCAR (parsed));
}

void
init_keyboard (void)
{
  command_loop_level = -1;
}

void
syms_of_keyboard (void)
{
  DEFSYM (Qhelp_echo, "help-echo");

  DEFSYM (Qvertical_line, "vertical-line");

  DEFSYM (QCfilter, ":filter");

  DEFSYM (Qevent_kind, "event-kind");
  DEFSYM (Qevent_symbol_elements, "event-symbol-elements");
  DEFSYM (Qevent_symbol_element_mask, "event-symbol-element-mask");

  DEFSYM (Qmodifier_cache, "modifier-cache");

  {
    int i;
    int len = ARRAYELTS (modifier_names);

    modifier_symbols = make_nil_vector (len);
    for (i = 0; i < len; i++)
      if (modifier_names[i])
        ASET (modifier_symbols, i, intern_c_string (modifier_names[i]));
    staticpro (&modifier_symbols);
  }

  defsubr (&Sevent_convert_list);
  defsubr (&Srecursive_edit);

  DEFVAR_LISP ("meta-prefix-char", meta_prefix_char,
               doc: /* Meta-prefix character code.
Meta-foo as command input turns into this character followed by foo.  */);
  XSETINT (meta_prefix_char, 033);

  DEFVAR_LISP ("function-key-map", Vfunction_key_map,
               doc: /* The parent keymap of all `local-function-key-map' instances.
Function key definitions that apply to all terminal devices should go
here.  If a mapping is defined in both the current
`local-function-key-map' binding and this variable, then the local
definition will take precedence.  */);
  Vfunction_key_map = Fmake_sparse_keymap (Qnil);

  DEFVAR_LISP ("key-translation-map", Vkey_translation_map,
               doc: /* Keymap of key translations that can override keymaps.
This keymap works like `input-decode-map', but comes after `function-key-map'.
Another difference is that it is global rather than terminal-local.  */);
  Vkey_translation_map = Fmake_sparse_keymap (Qnil);

  DEFVAR_LISP ("special-event-map", Vspecial_event_map,
        doc: /* Keymap defining bindings for special events to execute at low level.  */);
  Vspecial_event_map = list1 (Qkeymap);

  DEFVAR_LISP ("help-char", Vhelp_char,
        doc: /* Character to recognize as meaning Help.
When it is read, do `(eval help-form)', and display result if it's a string.
If the value of `help-form' is nil, this char can be read normally.  */);
  XSETINT (Vhelp_char, Ctl ('H'));

  DEFVAR_LISP ("command-error-function", Vcommand_error_function,
        doc: /* Function to output error messages.
Called with three arguments:
- the error data, a list of the form (SIGNALED-CONDITION . SIGNAL-DATA)
  such as what `condition-case' would bind its variable to,
- the context (a string which normally goes at the start of the message),
- the Lisp function within which the error was signaled.

For instance, to make error messages stand out more in the echo area,
you could say something like:

    (setq command-error-function
          (lambda (data _ _)
            (message "%s" (propertize (error-message-string data)
                                      \\='face \\='error))))

Also see `set-message-function' (which controls how non-error messages
are displayed).  */);
  Vcommand_error_function = Qcommand_error_default_function;

  DEFSYM (Qcommand_error_default_function, "command-error-default-function");
}

void
keys_of_keyboard (void)
{
  initial_define_lispy_key (Vspecial_event_map, "delete-frame",
                            "handle-delete-frame");
  initial_define_lispy_key (Vspecial_event_map, "ns-put-working-text",
                            "ns-put-working-text");
  initial_define_lispy_key (Vspecial_event_map, "ns-unput-working-text",
                            "ns-unput-working-text");
  initial_define_lispy_key (Vspecial_event_map, "iconify-frame", "ignore");
  initial_define_lispy_key (Vspecial_event_map, "make-frame-visible", "ignore");
  initial_define_lispy_key (Vspecial_event_map, "save-session",
                            "handle-save-session");
#ifdef THREADS_ENABLED
  initial_define_lispy_key (Vspecial_event_map, "thread-event",
                            "thread-handle-event");
#endif

#ifdef USE_FILE_NOTIFY
  initial_define_lispy_key (Vspecial_event_map, "file-notify",
                            "file-notify-handle-event");
#endif
  initial_define_lispy_key (Vspecial_event_map, "config-changed-event",
                            "ignore");

  initial_define_lispy_key (Vspecial_event_map, "focus-in", "handle-focus-in");
  initial_define_lispy_key (Vspecial_event_map, "focus-out",
                            "handle-focus-out");
  initial_define_lispy_key (Vspecial_event_map, "move-frame",
                            "handle-move-frame");
}
