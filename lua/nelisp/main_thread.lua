local handler=require'nelisp.handler'
local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local print_=require'nelisp.print'
local M={}
local recursive_depth=0
local function co_init()
    if M.co then return end
    recursive_depth=0
    M.co=coroutine.create(function ()
        while true do
            local err
            handler.internal_catch_lua(M.recursive_edit,function (msg)
                M.co=nil
                assert(#handler.handlerlist==0,'TODO')
                err=msg
            end)
            if err then return '\r'..err end
        end
    end)
    coroutine.resume(M.co)
end
local function command_loop_1()
    local fn=coroutine.yield(recursive_depth>1)
    while true do
        fn()
        fn=coroutine.yield(false)
    end
end
local function cmd_error(d)
    if not _G.nelisp_later then
        error('TODO')
    end
    local sig=lisp.xcar(d)
    if lisp.eq(sig,vars.Qvoid_function) then
        local cdr=lisp.xcdr(d)
        if lisp.consp(cdr) then
            error('TODO: MAYBE need to implement: '..lisp.sdata(lisp.symbol_name(lisp.xcar(cdr))))
        end
    end
    local readcharfun=print_.make_printcharfun()
    print_.print_obj(d,true,readcharfun)
    error('\n\nError (nelisp):\n'..readcharfun.out()..'\n')
end
local function command_loop_2(handlers)
    local val
    while true do
        val=handler.internal_condition_case(command_loop_1,handlers,cmd_error)
        if lisp.nilp(val) then
            break
        end
    end
end
function M.recursive_edit()
    local top_level=recursive_depth==0
    recursive_depth=recursive_depth+1
    if top_level then
        if not _G.nelisp_later then
            error('TODO: call the function in top-level')
        end
        while true do
            handler.internal_catch(vars.Qtop_level,command_loop_2,vars.Qerror)
            recursive_depth=1
        end
    end
    local val=handler.internal_catch(vars.Qexit,command_loop_2,vars.Qerror)
    recursive_depth=recursive_depth-1
    return val
end
---@return boolean|'error'
function M.call(fn)
    co_init()
    if M.co==coroutine.running() then
        return fn()
    end
    local jit_on
    if not _G.nelisp_later then
        error('TODO: remove jit.on and jit.off')
    elseif _G.nelisp_optimize_jit then
        jit_on=jit.status()
        jit.off() --luajit makes the code slower (why?)
    end
    local ret={select(2,coroutine.resume(M.co,fn))}
    if _G.nelisp_optimize_jit and jit_on then
        jit.on()
    end
    return unpack(ret)
end
return M
