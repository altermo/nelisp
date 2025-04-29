#include "lisp.h"

#include "buffer.h"
#include "lua.h"

static Lisp_Object cached_system_name;

static void
init_and_cache_system_name (void)
{
  init_system_name ();
  cached_system_name = Vsystem_name;
}

DEFUN ("point-max", Fpoint_max, Spoint_max, 0, 0, 0,
       doc: /* Return the maximum permissible value of point in the current buffer.
This is (1+ (buffer-size)), unless narrowing (a buffer restriction)
is in effect, in which case it is less.  */)
(void)
{
  Lisp_Object temp;
  XSETFASTINT (temp, ZV);
  return temp;
}

DEFUN ("system-name", Fsystem_name, Ssystem_name, 0, 0, 0,
       doc: /* Return the host name of the machine you are running on, as a string.  */)
(void)
{
  if (EQ (Vsystem_name, cached_system_name))
    init_and_cache_system_name ();
  return Vsystem_name;
}

Lisp_Object
make_buffer_string_both (ptrdiff_t start, ptrdiff_t start_byte, ptrdiff_t end,
                         ptrdiff_t end_byte, bool props)
{
  Lisp_Object result;
  ptrdiff_t beg0, end0, size;

  // if (start_byte < GPT_BYTE && GPT_BYTE < end_byte)
  //   {
  //     beg0 = start_byte;
  //     end0 = GPT_BYTE;
  //     beg1 = GPT_BYTE + GAP_SIZE - BEG_BYTE;
  //     end1 = end_byte + GAP_SIZE - BEG_BYTE;
  //   }
  // else
  {
    beg0 = start_byte;
    end0 = end_byte;
    // beg1 = -1;
    // end1 = -1;
  }

#if TODO_NELISP_LATER_AND
  if (!NILP (BVAR (current_buffer, enable_multibyte_characters)))
    result = make_uninit_multibyte_string (end - start, end_byte - start_byte);
  else
#endif
    result = make_uninit_string (end - start);

  size = end0 - beg0;
  // memcpy (SDATA (result), BYTE_POS_ADDR (beg0), size);
  // if (beg1 != -1)
  //   memcpy (SDATA (result) + size, BEG_ADDR + beg1, end1 - beg1);
  buffer_memcpy (SDATA (result), beg0, size);

  if (props)
    {
#if TODO_NELISP_LATER_AND
      update_buffer_properties (start, end);

      tem
        = Fnext_property_change (make_fixnum (start), Qnil, make_fixnum (end));
      tem1 = Ftext_properties_at (make_fixnum (start), Qnil);

      if (XFIXNUM (tem) != end || !NILP (tem1))
        copy_intervals_to_string (result, current_buffer, start, end - start);
#endif
    }

  return result;
}

DEFUN ("buffer-string", Fbuffer_string, Sbuffer_string, 0, 0, 0,
       doc: /* Return the contents of the current buffer as a string.
If narrowing is in effect, this function returns only the visible part
of the buffer.

This function copies the text properties of that part of the buffer
into the result string; if you don’t want the text properties,
use `buffer-substring-no-properties' instead.  */)
(void) { return make_buffer_string_both (BEGV, BEGV_BYTE, ZV, ZV_BYTE, 1); }

DEFUN ("message", Fmessage, Smessage, 1, MANY, 0,
       doc: /* Display a message at the bottom of the screen.
The message also goes into the `*Messages*' buffer, if `message-log-max'
is non-nil.  (In keyboard macros, that's all it does.)
Return the message.

In batch mode, the message is printed to the standard error stream,
followed by a newline.

The first argument is a format control string, and the rest are data
to be formatted under control of the string.  Percent sign (%), grave
accent (\\=`) and apostrophe (\\=') are special in the format; see
`format-message' for details.  To display STRING without special
treatment, use (message "%s" STRING).

If the first argument is nil or the empty string, the function clears
any existing message; this lets the minibuffer contents show.  See
also `current-message'.

usage: (message FORMAT-STRING &rest ARGS)  */)
(ptrdiff_t nargs, Lisp_Object *args)
{
  UNUSED (nargs);
  TODO_NELISP_LATER;
  LUA (5)
  {
    lua_getglobal (L, "print");
    lua_pushlstring (L, (char *) SDATA (args[0]), SBYTES (args[0]));
    lua_pcall (L, 1, 0, 0);
  }
  return args[0];
}

void
syms_of_editfns (void)
{
  DEFVAR_LISP ("system-name", Vsystem_name,
          doc: /* The host name of the machine Emacs is running on.  */);
  Vsystem_name = cached_system_name = Qnil;

  defsubr (&Spoint_max);
  defsubr (&Ssystem_name);
  defsubr (&Sbuffer_string);
  defsubr (&Smessage);
}
