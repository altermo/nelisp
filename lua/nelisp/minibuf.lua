local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'

local M={}
function M.init()
    vars.V.minibuffer_prompt_properties=lisp.list(vars.Qread_only,vars.Qt)
end
function M.init_syms()
    vars.defvar_lisp('minibuffer_prompt_properties','minibuffer-prompt-properties',[[Text properties that are added to minibuffer prompts.
These are in addition to the basic `field' property, and stickiness
properties.]])

    vars.defsym('Qread_only','read-only')
end
return M
