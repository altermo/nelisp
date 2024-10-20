local lread=require'nelisp.lread'
local eval=require'nelisp.eval'
local main_thread=require'nelisp.main_thread'
local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'
local str=require'nelisp.obj.str'
local M={}
function M.init()
    require'nelisp.initer'
end
---@class nelisp.eval.prommise
---@field [1] 'Prommise, see :help ...' --TODO
---@field [2] true?
---@field [3] nelisp.obj?

---@param form string|nelisp.obj
---@return nelisp.obj|nelisp.eval.prommise?
function M.eval(form)
    if not _G.nelisp_later then
        error('TODO: if it errors after it has prommised something, then the prommise is never resolved')
        --What should happen is `prommise[2]='error'`
        --Example `(progn (recursive-edit) (throw 'exit t))`
    end
    local done,prommise,ret
    local status=main_thread.call(function ()
        if type(form)=='string' then
            for _,cons in ipairs(lread.full_read_lua_string(form)) do
                ret=eval.eval_sub(cons)
            end
        elseif lisp.consp(form) then
            ret=eval.eval_sub(form)
        elseif lisp.nilp(form) then
            ret=nil
        else
            error('Unreachable')
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
    return M.eval(lisp.list(vars.Qload,str.make(path,'auto')))
end
return M
