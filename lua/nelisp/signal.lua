local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'
local doprnt=require'nelisp.doprnt'
local str=require'nelisp.obj.str'

local function vformat_string(fmt,...)
    return doprnt.doprnt(fmt,{...},false)
end
local M={}
function M.xsignal(error_symbol,...)
    local data=lisp.list(...)
    vars.F.signal(error_symbol,data)
end
---@param fmt string
---@param ... string|number
function M.error(fmt,...)
    M.xsignal(vars.Qerror,str.make(vformat_string(fmt,...),'auto'))
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
