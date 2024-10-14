local lread=require'nelisp.lread'
local eval=require'nelisp.eval'
local M={}
function M.init()
    require'nelisp.initer'
end
---@param str string
---@return nelisp.obj?
function M.eval(str)
    local ret
    for _,cons in ipairs(lread.full_read_lua_string(str)) do
        ret=eval.eval_sub(cons)
    end
    return ret
end
---@param path string
---@return nelisp.obj?
function M.eval_file(path)
    local content=table.concat(vim.fn.readfile(path),'\n')
    return M.eval(content)
end
return M
