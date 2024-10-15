local lread=require'nelisp.lread'
local eval=require'nelisp.eval'
local specpdl=require'nelisp.specpdl'
local M={}
function M.init()
    require'nelisp.initer'
end
---@param str string
---@return nelisp.obj?
function M.eval(str)
    local noerr,ret=xpcall(function ()
        local ret
        for _,cons in ipairs(lread.full_read_lua_string(str)) do
            ret=eval.eval_sub(cons)
        end
        return ret
    end,function (msg)
            local err,ret=pcall(function ()
                specpdl.unbind_to(1,nil,true)
                vim.api.nvim_echo({{'E5108: Error executing lua '..debug.traceback(msg,4),'ErrorMsg'}},true,{})
            end)
            if not err then
                vim.api.nvim_err_writeln(ret --[[@as string]])
            end
        end)
    if noerr then
        return ret
    end
    error()
end
---@param path string
---@return nelisp.obj?
function M.eval_file(path)
    local content=table.concat(vim.fn.readfile(path),'\n')
    return M.eval(content)
end
return M
