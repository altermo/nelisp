local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'

local M={}
function M.xsignal(error_symbol,...)
    local data=lisp.list(...)
    vars.F.signal(error_symbol,data)
end
function M.error(fmt,...)
    error('TODO: err\n\n'..fmt..'\n\n'..vim.inspect({...})..'\n\n')
end
function M.wrong_type_argument(predicate,x)
    M.xsignal(vars.Qwrong_type_argument,predicate,x)
end
function M.args_out_of_range(a1,a2,a3)
    if a3 then
        M.xsignal(vars.Qargs_out_of_range,a1,a2,a3)
    else
        M.xsignal(vars.Qargs_out_of_range,a1,a2)
    end
end
function M.signal_error(s,arg)
    error('TODO: err\n\n'..s..'\n\n'..vim.inspect(arg)..'\n\n')
end

return M
