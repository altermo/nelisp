#include "lisp.h"

EMACS_INT command_loop_level;

static Lisp_Object
cmd_error (Lisp_Object data)
{
    TODO_NELISP_LATER;
    UNUSED(data);
    tcall_error=true;
    lua_pushliteral(global_lua_state,"nelisp error");
    return make_fixnum(0);
}

static Lisp_Object
command_loop_1 (void)
{
    while (1){
        mtx_lock(&main_mutex);
        cnd_signal(&main_cond);
        mtx_unlock(&main_mutex);
        cnd_wait(&thread_cond,&thread_mutex);

        eassert(main_func);
        main_func();
        main_func=NULL;
    }
}
Lisp_Object
command_loop_2 (Lisp_Object handlers)
{
    register Lisp_Object val;

    UNUSED(handlers);
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

void
init_keyboard (void)
{
    command_loop_level = -1;
}

void
syms_of_keyboard (void)
{
    defsubr (&Srecursive_edit);
}
