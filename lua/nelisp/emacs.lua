local vars=require'nelisp.vars'
local M={}
function M.init_syms()
    vars.defsym('Qrisky_local_variable','risky-local-variable')

    vars.defvar_lisp('dump_mode','dump-mode','Non-nil when Emacs is dumping itself.')
    vars.V.dump_mode=vars.Qnil
    vars.defvar_lisp('command_line_args','command-line-args',[[Args passed by shell to Emacs, as a list of strings.
Many arguments are deleted from the list as they are processed.]])
    vars.V.command_line_args=vars.Qnil
    if not _G.nelisp_later then
        error('TODO: command-line-args how should it be initialized?')
    end
end
return M
