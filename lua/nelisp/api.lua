local lread=require'nelisp.lread'
local eval=require'nelisp.eval'
local main_thread=require'nelisp.main_thread'
local M={}
function M.init()
    require'nelisp.initer'
end
---@class nelisp.eval.prommise
---@field [1] 'Prommise, see :help ...' --TODO
---@field [2] true?
---@field [3] nelisp.obj?

---@param str string
---@return nelisp.obj|nelisp.eval.prommise?
function M.eval(str)
    if not _G.nelisp_later then
        error('TODO: if it errors after it has prommised something, then the prommise is never resolved')
        --What should happen is `prommise[2]='error'`
        --Example `(progn (recursive-edit) (throw 'exit t))`
    end
    local done,prommise,ret
    local status=main_thread.call(function ()
        for _,cons in ipairs(lread.full_read_lua_string(str)) do
            ret=eval.eval_sub(cons)
        end
        done=true
        if prommise then
            prommise[2]=true
            prommise[3]=ret
        end
    end)
    if done then
        return ret
    elseif status then
        ---@type nelisp.eval.prommise
        prommise={'Prommise, see :help ...'}
        return prommise
    else
        return nil
    end
end
---@param path string
---@return nelisp.obj|nelisp.eval.prommise?
function M.eval_file(path)
    local content=table.concat(vim.fn.readfile(path),'\n')
    return M.eval(content)
end
return M
