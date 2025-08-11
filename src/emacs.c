#include "lisp.h"

#ifdef HAVE_SETLOCALE
# include <locale.h>
#endif

bool running_asynch_code;
bool build_details;
bool noninteractive;

static const char emacs_version[] = PACKAGE_VERSION;

static void
synchronize_locale (int category, Lisp_Object *plocale,
                    Lisp_Object desired_locale)
{
  if (!EQ (*plocale, desired_locale))
    {
      *plocale = desired_locale;
      char const *locale_string
        = STRINGP (desired_locale) ? SSDATA (desired_locale) : "";
      setlocale (category, locale_string);
    }
}

static Lisp_Object Vprevious_system_time_locale;

void
synchronize_system_time_locale (void)
{
  synchronize_locale (LC_TIME, &Vprevious_system_time_locale,
                      Vsystem_time_locale);
}

void
syms_of_emacs (void)
{
  DEFSYM (Qfile_name_handler_alist, "file-name-handler-alist");
  DEFSYM (Qrisky_local_variable, "risky-local-variable");
  DEFSYM (Qkill_emacs, "kill-emacs");
  DEFSYM (Qkill_emacs_hook, "kill-emacs-hook");
  DEFSYM (Qrun_hook_query_error_with_timeout,
    "run-hook-query-error-with-timeout");
  DEFSYM (Qfile_truename, "file-truename");
  DEFSYM (Qcommand_line_processed, "command-line-processed");
  DEFSYM (Qsafe_magic, "safe-magic");

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

  DEFVAR_LISP ("after-init-time", Vafter_init_time,
        doc: /* Value of `current-time' after loading the init files.
This is nil during initialization.  */);
  Vafter_init_time = Qnil;

  DEFVAR_LISP ("system-messages-locale", Vsystem_messages_locale,
        doc: /* System locale for messages.  */);
  Vsystem_messages_locale = Qnil;

  DEFVAR_LISP ("system-time-locale", Vsystem_time_locale,
        doc: /* System locale for time.  */);
  Vsystem_time_locale = Qnil;
  Vprevious_system_time_locale = Qnil;
  staticpro (&Vprevious_system_time_locale);

  DEFVAR_BOOL ("inhibit-x-resources", inhibit_x_resources,
        doc: /* If non-nil, X resources, Windows Registry settings, and NS defaults are not used.  */);
  inhibit_x_resources = 0;

  DEFVAR_LISP ("emacs-version", Vemacs_version,
	       doc: /* Version numbers of this version of Emacs.
This has the form: MAJOR.MINOR[.MICRO], where MAJOR/MINOR/MICRO are integers.
MICRO is only present in unreleased development versions,
and is not especially meaningful.  Prior to Emacs 26.1, an extra final
component .BUILD is present.  This is now stored separately in
`emacs-build-number'.  */);
  Vemacs_version = build_string (emacs_version);

  DEFVAR_BOOL ("noninteractive", noninteractive1,
               doc: /* Non-nil means Emacs is running without interactive terminal.  */);
}
