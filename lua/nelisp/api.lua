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
    return handler.internal_catch_lua(function ()
        local ret
        for _,cons in ipairs(lread.full_read_lua_string(str)) do
            ret=eval.eval_sub(cons)
        end
        return ret
    end,function (msg)
        assert(#handler.handlerlist==0,'TODO')
        vim.api.nvim_echo({{msg,'ErrorMsg'}},true,{})
        error()
    end)
end
---@param path string
---@return nelisp.obj?
function M.eval_file(path)
    local content=table.concat(vim.fn.readfile(path),'\n')
    return M.eval(content)
end
return M
