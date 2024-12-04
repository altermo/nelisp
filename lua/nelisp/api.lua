local lread=require'nelisp.lread'
local eval=require'nelisp.eval'
local main_thread=require'nelisp.main_thread'
local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'
local alloc=require'nelisp.alloc'
local specpdl=require'nelisp.specpdl'
local M={}
function M.init()
    require'nelisp.initer'
end
---@class nelisp.eval.promise
---@field [1] 'promise'
---@field [2] true|'error'|nil
---@field [3] nelisp.obj|nil

---@param form string|nelisp.obj
---@param callback fun(ret:nelisp.obj?)?
---@return nelisp.obj|nelisp.eval.promise?
function M.eval(form,callback)
    ---@type nelisp.eval.promise?
    local promise
    local done
    local ret
    local noerr,errmsg=main_thread.call(function ()
        local count=specpdl.index()
        local is_error=true
        specpdl.record_unwind_protect(function ()
            if is_error and promise then
                promise[2]='error'
            elseif promise then
                promise[2]=true
                promise[3]=ret
            elseif is_error then
            else
                done=true
            end
        end)
        if type(form)=='string' then
            for _,cons in ipairs(lread.full_read_lua_string(form)) do
                ret=eval.eval_sub(cons)
            end
            is_error=false
        else
            ret=eval.eval_sub(form)
            is_error=false
        end
        if callback then
            callback(ret)
        end
        specpdl.unbind_to(count,nil)
    end)
    if not noerr then
        error(errmsg,0)
    elseif done then
        return ret
    else
        promise={'promise'}
        return promise
    end
end
---@param path string
---@return nelisp.obj|nelisp.eval.promise?
function M.load(path)
    return M.eval(lisp.list(vars.Qload,alloc.make_string(path)))
end
return M
