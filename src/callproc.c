#include <unistd.h>

#include "lisp.h"

static bool
getenv_internal_1 (const char *var, ptrdiff_t varlen, char **value,
                   ptrdiff_t *valuelen, Lisp_Object env)
{
  for (; CONSP (env); env = XCDR (env))
    {
      Lisp_Object entry = XCAR (env);
      if (STRINGP (entry) && SBYTES (entry) >= varlen
          && !memcmp (SDATA (entry), var, varlen))
        {
          if (SBYTES (entry) > varlen && SREF (entry, varlen) == '=')
            {
              *value = SSDATA (entry) + (varlen + 1);
              *valuelen = SBYTES (entry) - (varlen + 1);
              return 1;
            }
          else if (SBYTES (entry) == varlen)
            {
              *value = NULL;
              return 1;
            }
        }
    }
  return 0;
}

static bool
getenv_internal (const char *var, ptrdiff_t varlen, char **value,
                 ptrdiff_t *valuelen, Lisp_Object frame)
{
  if (getenv_internal_1 (var, varlen, value, valuelen, Vprocess_environment))
    return *value ? 1 : 0;

  if (strcmp (var, "DISPLAY") == 0)
    {
      TODO;
    }
  return 0;
}

DEFUN ("getenv-internal", Fgetenv_internal, Sgetenv_internal, 1, 2, 0,
       doc: /* Get the value of environment variable VARIABLE.
VARIABLE should be a string.  Value is nil if VARIABLE is undefined in
the environment.  Otherwise, value is a string.

This function searches `process-environment' for VARIABLE.

If optional parameter ENV is a list, then search this list instead of
`process-environment', and return t when encountering a negative entry
\(an entry for a variable with no value).  */)
(Lisp_Object variable, Lisp_Object env)
{
  char *value;
  ptrdiff_t valuelen;

  CHECK_STRING (variable);
  if (CONSP (env))
    {
      if (getenv_internal_1 (SSDATA (variable), SBYTES (variable), &value,
                             &valuelen, env))
        return value ? make_string (value, valuelen) : Qt;
      else
        return Qnil;
    }
  else if (getenv_internal (SSDATA (variable), SBYTES (variable), &value,
                            &valuelen, env))
    return make_string (value, valuelen);
  else
    return Qnil;
}

void
set_initial_environment (void)
{
  char **envp;
#if TODO_NELISP_LATER_AND
  for (envp = environ; *envp; envp++)
#else
  for (envp = __environ; *envp; envp++)
#endif
    Vprocess_environment = Fcons (build_string (*envp), Vprocess_environment);
}

void
syms_of_callproc (void)
{
  DEFVAR_LISP ("data-directory", Vdata_directory,
        doc: /* Directory of machine-independent files that come with GNU Emacs.
These are files intended for Emacs to use while it runs.  */);

  DEFVAR_LISP ("process-environment", Vprocess_environment,
        doc: /* List of overridden environment variables for subprocesses to inherit.
Each element should be a string of the form ENVVARNAME=VALUE.

Entries in this list take precedence to those in the frame-local
environments.  Therefore, let-binding `process-environment' is an easy
way to temporarily change the value of an environment variable,
irrespective of where it comes from.  To use `process-environment' to
remove an environment variable, include only its name in the list,
without "=VALUE".

This variable is set to nil when Emacs starts.

If multiple entries define the same variable, the first one always
takes precedence.

Non-ASCII characters are encoded according to the initial value of
`locale-coding-system', i.e. the elements must normally be decoded for
use.

See `setenv' and `getenv'.  */);
  Vprocess_environment = Qnil;

  defsubr (&Sgetenv_internal);
}
