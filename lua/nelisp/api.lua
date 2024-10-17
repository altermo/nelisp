local lread=require'nelisp.lread'
local eval=require'nelisp.eval'
local handler=require'nelisp.handler'
local M={}
function M.init()
    require'nelisp.initer'
end
---@param str string
---@return nelisp.obj?
function M.eval(str)
    local noerr,ret=handler.with_handler(nil,'CATCHER_LUA',function ()
        local ret
        for _,cons in ipairs(lread.full_read_lua_string(str)) do
            ret=eval.eval_sub(cons)
        end
        return ret
    end)
    if noerr then
        return ret --[[@as nelisp.obj]]
    end
    vim.api.nvim_echo({{ret,'ErrorMsg'}},true,{})
    error()
end
---@param path string
---@return nelisp.obj?
function M.eval_file(path)
    local content=table.concat(vim.fn.readfile(path),'\n')
    return M.eval(content)
end
return M
