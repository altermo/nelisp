local specpdl=require'nelisp.specpdl'
local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'
local signal=require'nelisp.signal'

---@class nelisp.handler.msg_other
---@field id nelisp.handler.id
---@field val nelisp.obj
---@field is_lua nil
---@field handler_id number
---@field nonlocal_exit 'SIGNAL'|'THROW'
---@class nelisp.handler.msg_lua: nelisp.handler.msg_other
---@field is_lua true
---@field backtrace string
---@field val nil
---@field handler_id nil
---@field nonlocal_exit nil
---@alias nelisp.handler.msg nelisp.handler.msg_lua|nelisp.handler.msg_other

---@class nelisp.handler
---@field id number
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
        id=#M.handlerlist,
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
        assert(e.id==#M.handlerlist)
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
            return false,msg.backtrace
        else
            error(msg)
        end
    end
    ---@cast msg nelisp.handler.msg_other
    if msg.handler_id==e.id then
        assert(e.type~='CATCHER_LUA','TODO')
        return false,msg
    else
        error(msg)
    end
end
---@param handler_id number
---@param val nelisp.obj
---@param nonlocal_exit 'SIGNAL'|'THROW'
function M.unwind_to_catch(handler_id,val,nonlocal_exit)
    assert(M.handlerlist[handler_id+1])
    ---@type nelisp.handler.msg_other
    local msg={
        id=id,
        val=val,
        handler_id=handler_id,
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
    return hfun(args.val)
end
function M.internal_lisp_condition_case(var,bodyform,handlers)
    local tail=handlers
    local success_handler=vars.Qnil
    local clauses={}
    while lisp.consp(tail) do
        local tem=lisp.xcar(tail)
        if not (lisp.nilp(tem)
            or (lisp.consp(tem) and
            (lisp.symbolp(lisp.xcar(tem)) or lisp.consp(lisp.xcar(tem))))) then
            signal.error('Invalid condition handler: %s',lisp.sdata(vars.F.prin1_to_string(tem,vars.Qt,vars.Qnil)))
        end
        if lisp.consp(tem) and lisp.eq(lisp.xcar(tem),vars.QCsuccess) then
            success_handler=lisp.xcdr(tem)
        else
            table.insert(clauses,1,tem)
        end
        tail=lisp.xcdr(tail)
    end
    local result
    local function f(idx)
        local clause=clauses[idx]
        if not clause then
            result=require'nelisp.eval'.eval_sub(bodyform)
            return
        end
        local condition=lisp.consp(clause) and lisp.xcar(clause) or var.Qnil
        if not lisp.consp(condition) then
            condition=lisp.list(condition)
        end
        local noerr,ret=M.with_handler(condition,'CONDITION_CASE',f,idx+1)
        if noerr then
            return ret
        end
        ---@cast ret nelisp.handler.msg_other
        local val=ret.val
        assert(lisp.consp(clause))
        local handler_body=lisp.xcdr(clause)
        if lisp.nilp(var) then
            return vars.F.progn(handler_body)
        end
        local handler_var=var
        if not lisp.nilp(vars.V.internal_interpreter_environment) then
            val=vars.F.cons(vars.F.cons(var,val),vars.V.internal_interpreter_environment)
            handler_var=vars.Qinternal_interpreter_environment
        end
        local count=specpdl.index()
        specpdl.bind(handler_var,val)
        return specpdl.unbind_to(count,vars.F.progn(handler_body))
    end
    local ret=f(1)
    if result==nil then
        return ret
    end
    if not lisp.nilp(success_handler) then
        error('TODO')
    end
    return result
end
return M
