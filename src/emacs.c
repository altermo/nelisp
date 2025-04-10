#include "lisp.h"

void
syms_of_emacs (void)
{
  DEFSYM (Qrisky_local_variable, "risky-local-variable");

  DEFVAR_LISP ("command-line-args", Vcommand_line_args,
                 doc: /* Args passed by shell to Emacs, as a list of strings.
Many arguments are deleted from the list as they are processed.  */);

  DEFVAR_LISP ("dump-mode", Vdump_mode,
                 doc: /* Non-nil when Emacs is dumping itself.  */);

  DEFVAR_LISP ("system-type", Vsystem_type,
	       doc: /* The value is a symbol indicating the type of operating system you are using.
Special values:
  `gnu'          compiled for a GNU Hurd system.
  `gnu/linux'    compiled for a GNU/Linux system.
  `gnu/kfreebsd' compiled for a GNU system with a FreeBSD kernel.
  `darwin'       compiled for Darwin (GNU-Darwin, macOS, ...).
  `ms-dos'       compiled as an MS-DOS application.
  `windows-nt'   compiled as a native W32 application.
  `cygwin'       compiled using the Cygwin library.
  `haiku'        compiled for a Haiku system.
  `android'      compiled for Android.
Anything else (in Emacs 26, the possibilities are: aix, berkeley-unix,
hpux, usg-unix-v) indicates some sort of Unix system.  */);
  Vsystem_type = intern_c_string (SYSTEM_TYPE);
}
