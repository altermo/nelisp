#include "lisp.h"
#include "lua.h"
#include "termhooks.h"

EMACS_INT command_loop_level;

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

  defsubr (&Srecursive_edit);

  DEFVAR_LISP ("meta-prefix-char", meta_prefix_char,
               doc: /* Meta-prefix character code.
Meta-foo as command input turns into this character followed by foo.  */);
  XSETINT (meta_prefix_char, 033);
}
