local specpdl=require'nelisp.specpdl'
local vars=require'nelisp.vars'

local M={}
local handlerlist={}
local id={}

---@param tag_ch_cal nelisp.obj?
---@param handlertype 'CATCHER'|'CONDITION_CASE'|'CATCHER_ALL'|'CATCHER_LUA'
---@param fn function
---@return true|false,any
function M.with_handler(tag_ch_cal,handlertype,fn,...)
    local msg,err
    table.insert(handlerlist,1,{
        tag_or_ch=tag_ch_cal,
        type=handlertype,
        lisp_eval_depth=vars.lisp_eval_depth,
        pdlcount=specpdl.index(),
    })
    local n=#handlerlist
    local ret={xpcall(fn,function (m)
        msg=m
        err=true
        if type(msg)~='table' or msg.id~=id then
            local backtrace=debug.traceback(tostring(msg),3)
            msg={id=id,backtrace=backtrace,n=-1,is_lua=true}
        end
    end,...)}
    assert(n==#handlerlist)
    if not err then
        local e=table.remove(handlerlist,1)
        assert(vars.lisp_eval_depth==e.lisp_eval_depth)
        assert(specpdl.index()==e.pdlcount)
        return unpack(ret)
    end
    local e=table.remove(handlerlist,1)
    specpdl.unbind_to(e.pdlcount,nil,true)
    vars.lisp_eval_depth=e.lisp_eval_depth
    if msg.is_lua then
        assert(e.type~='CATCHER_ALL','TODO')
        if e.type=='CATCHER_LUA' then
            return false,'E5108: Error executing lua '..msg.backtrace
        else
            error(msg)
        end
    elseif msg.n==n then
        return false,msg.val
    else
        error(msg)
    end
end
return M
