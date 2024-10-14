local vars=require'nelisp.vars'
local M={}

local F={}
F.Xprint={'!print',1,1,0,[[internal function]]}
function F.Xprint.f(x)
    require'nelisp._print'.p(x)
    return vars.Qt
end

function M.init_syms()
    vars.setsubr(F,'Xprint')
end
return M
