local vars=require'nelisp.vars'
local lread=require'nelisp.lread'
local alloc=require'nelisp.alloc'
local M={}
function M.init()
    if not _G.nelisp_later then
        error('TODO: command-line-args how should it be initialized?')
        error('TODO: system-type should also be initialized')
    end

    vars.V.system_type=lread.intern_c_string('gnu/linux')

    vars.V.emacs_version=alloc.make_string('29.4')
end
function M.init_syms()
    vars.defsym('Qrisky_local_variable','risky-local-variable')

    vars.defvar_lisp('dump_mode','dump-mode','Non-nil when Emacs is dumping itself.')
    vars.V.dump_mode=vars.Qnil
    vars.defvar_lisp('command_line_args','command-line-args',[[Args passed by shell to Emacs, as a list of strings.
Many arguments are deleted from the list as they are processed.]])
    vars.V.command_line_args=vars.Qnil

    vars.defvar_lisp('system_type','system-type',[[The value is a symbol indicating the type of operating system you are using.
Special values:
  `gnu'          compiled for a GNU Hurd system.
  `gnu/linux'    compiled for a GNU/Linux system.
  `gnu/kfreebsd' compiled for a GNU system with a FreeBSD kernel.
  `darwin'       compiled for Darwin (GNU-Darwin, macOS, ...).
  `ms-dos'       compiled as an MS-DOS application.
  `windows-nt'   compiled as a native W32 application.
  `cygwin'       compiled using the Cygwin library.
  `haiku'        compiled for a Haiku system.
Anything else (in Emacs 26, the possibilities are: aix, berkeley-unix,
hpux, usg-unix-v) indicates some sort of Unix system.]])

    vars.defvar_lisp('emacs_version','emacs-version',[[Version numbers of this version of Emacs.
This has the form: MAJOR.MINOR[.MICRO], where MAJOR/MINOR/MICRO are integers.
MICRO is only present in unreleased development versions,
and is not especially meaningful.  Prior to Emacs 26.1, an extra final
component .BUILD is present.  This is now stored separately in
`emacs-build-number'.]])
end
return M
