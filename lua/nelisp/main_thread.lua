local handler=require'nelisp.handler'
local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local M={}
local recursive_depth=0
local function co_init()
    if M.co then return end
    recursive_depth=0
    M.co=coroutine.create(function ()
        while true do
            handler.internal_catch_lua(M.recursive_edit,function (msg)
                M.co=nil
                assert(#handler.handlerlist==0,'TODO')
                vim.api.nvim_echo({{msg,'ErrorMsg'}},true,{})
                error()
            end)
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
local function cmd_error()
    error('TODO')
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
function M.call(fn)
    co_init()
    if M.co==coroutine.running() then
        return fn()
    end
    return select(2,coroutine.resume(M.co,fn))
end
return M
