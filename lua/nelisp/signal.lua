local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'

local M={}
function M.xsignal(error_symbol,...)
    local data=lisp.list(...)
    error('TODO: err\n\n'..vim.inspect(error_symbol)..'\n\n'..vim.inspect(data)..'\n\n')
end
function M.error(fmt,...)
    error('TODO: err\n\n'..fmt..'\n\n'..vim.inspect({...})..'\n\n')
end
function M.wrong_type_argument(predicate,x)
    M.xsignal(vars.Qwrong_type_argument,predicate,x)
end
function M.signal_error(s,arg)
    error('TODO: err\n\n'..s..'\n\n'..vim.inspect(arg)..'\n\n')
end

return M
