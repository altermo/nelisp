local vars=require'nelisp.vars'
local M={}
function M.init_syms()
    vars.defsym('Qemacs','emacs')
end
return M
