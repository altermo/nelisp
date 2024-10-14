local vars=require'nelisp.vars'
local M={}
function M.init_syms()
    vars.defsym('Qfunction_documentation','function-documentation')
end
return M
