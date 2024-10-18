local specpdl=require'nelisp.specpdl'
local vars=require'nelisp.vars'

---@class nelisp.handler.msg_other
---@field id nelisp.handler.id
---@field val nelisp.obj
---@field is_lua nil
---@field jmp number
---@field nonlocal_exit 'SIGNAL'|'THROW'
---@class nelisp.handler.msg_lua: nelisp.handler.msg_other
---@field is_lua true
---@field backtrace string
---@field private val nil
---@field private jmp nil
---@field private nonlocal_exit nil
---@alias nelisp.handler.msg nelisp.handler.msg_other|nelisp.handler.msg_lua

---@class nelisp.handler
---@field jmp number
---@field tag_or_ch nelisp.obj?
---@field type 'CATCHER'|'CONDITION_CASE'|'CATCHER_ALL'|'CATCHER_LUA'
---@field lisp_eval_depth number
---@field pdlcount number

local M={}
---@type nelisp.handler[]
M.handlerlist={}
---@class nelisp.handler.id
local id={}


---@param tag_ch_cal nelisp.obj?
---@param handlertype 'CATCHER'|'CONDITION_CASE'|'CATCHER_ALL'|'CATCHER_LUA'
---@param fn function
---@return true|false
---@return any[]|string|nelisp.handler.msg
function M.with_handler(tag_ch_cal,handlertype,fn,...)
    table.insert(M.handlerlist,1,{
        tag_or_ch=tag_ch_cal,
        type=handlertype,
        lisp_eval_depth=vars.lisp_eval_depth,
        pdlcount=specpdl.index(),
        jmp=#M.handlerlist,
    } --[[@as nelisp.handler]])
    ---@type nelisp.handler.msg
    local msg,err
    local ret={select(2,xpcall(fn,function (m)
        msg=m
        err=true
        if type(msg)~='table' or msg.id~=id then
            local backtrace=debug.traceback(tostring(msg),3)
            ---@type nelisp.handler.msg_lua
            msg={id=id,backtrace=backtrace,is_lua=true}
        end
    end,...))}
    if not err then
        local e=table.remove(M.handlerlist,1)
        assert(e.jmp==#M.handlerlist)
        assert(vars.lisp_eval_depth==e.lisp_eval_depth)
        assert(specpdl.index()==e.pdlcount)
        return true,ret
    end
    local e=table.remove(M.handlerlist,1)
    specpdl.unbind_to(e.pdlcount,nil,true)
    vars.lisp_eval_depth=e.lisp_eval_depth
    if msg.is_lua then
        ---@cast msg nelisp.handler.msg_lua
        assert(e.type~='CATCHER_ALL','TODO')
        --TODO: maybe use a condition with a symbol instead
        if e.type=='CATCHER_LUA' then
            return false,'E5108: Error executing lua '..msg.backtrace
        else
            error(msg)
        end
    end
    ---@cast msg nelisp.handler.msg_other
    if msg.jmp==e.jmp then
        assert(e.type~='CATCHER_LUA','TODO')
        return false,msg
    else
        error(msg)
    end
end
---@param n number
---@param val nelisp.obj
---@param nonlocal_exit 'SIGNAL'|'THROW'
function M.longjmp(n,val,nonlocal_exit)
    assert(M.handlerlist[n+1])
    ---@type nelisp.handler.msg_other
    local msg={
        id=id,
        val=val,
        jmp=n,
        nonlocal_exit=nonlocal_exit,
    }
    error(msg)
end
---@param fun function
---@param handler fun(a:string)
function M.internal_catch_lua(fun,handler)
    local noerr,args=M.with_handler(nil,'CATCHER_LUA',fun)
    if noerr then
        return unpack(args --[[@as(any[])]])
    end
    return handler(args --[[@as string]])
end
function M.internal_catch(tag,fun,arg)
    local noerr,args=M.with_handler(tag,'CATCHER',fun,arg)
    if noerr then
        return unpack(args --[[@as(any[])]])
    end
    ---@cast args nelisp.handler.msg_other
    return args.val
end
function M.internal_condition_case(bfun,handlers,hfun)
    local noerr,args=M.with_handler(handlers,'CONDITION_CASE',bfun)
    if noerr then
        return unpack(args --[[@as(any[])]])
    end
    return hfun(args --[[@as nelisp.handler.msg_other]])
end
return M
