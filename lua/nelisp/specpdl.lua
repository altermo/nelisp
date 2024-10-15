local lisp=require'nelisp.lisp'
local symbol=require'nelisp.obj.symbol'

---@class nelisp.specpdl.index: number

local specpdl={}

local M={}

---@enum nelisp.specpdl.type
M.type={
    backtrace=1,
    let=2,
    let_default=3,
}

---@return nelisp.specpdl.index
function M.index()
    return #specpdl+1 --[[@as nelisp.specpdl.index]]
end
---@generic T
---@param index nelisp.specpdl.index
---@param val T
---@return T
function M.unbind_to(index,val)
    assert(index~=M.index(),'DEV: index not changed, unbind_to may be unnecessary')
    while M.index()>index do
        local entry=table.remove(specpdl)
        if entry.type==M.type.let and lisp.symbolp(entry.symbol) and symbol.get_redirect(entry.symbol)==symbol.redirect.plain then
            if symbol.get_tapped_wire(entry.symbol)==symbol.trapped_wire.untrapped_write then
                symbol.set_var(entry.symbol,entry.old_value)
            else
                error('TODO')
            end
        elseif entry.type==M.type.let or entry.type==M.type.let_default then
            error('TODO')
        elseif entry.type==M.type.backtrace then
        else
            error('TODO')
        end
    end
    return val
end
---@param func nelisp.obj
---@param args nelisp.obj[]
---@param nargs number
---@overload fun(func:nelisp.obj,args:nelisp.obj,nargs:'UNEVALLED')
function M.record_in_backtrace(func,args,nargs)
    local index=M.index()
    table.insert(specpdl,{
        type=M.type.backtrace,
        debug_on_exit=false,
        func=func,
        args=args,
        nargs=nargs,
    })
    return index
end
---@param index nelisp.specpdl.index
function M.backtrace_debug_on_exit(index)
    local entry=specpdl[index]
    assert(entry.type==M.type.backtrace)
    return entry.debug_on_exit
end
---@param index nelisp.specpdl.index
function M.set_backtrace_args(index,args)
    local entry=specpdl[index]
    assert(entry.type==M.type.backtrace)
    entry.args=args
end
---@param sym nelisp.obj
---@param val nelisp.obj
function M.bind(sym,val)
    lisp.check_symbol(sym)
    ---@cast sym nelisp.symbol
    if symbol.get_redirect(sym)==symbol.redirect.plain then
        table.insert(specpdl,{
            type=M.type.let,
            symbol=sym,
            old_value=symbol.get_var(sym)
        })
        if symbol.get_tapped_wire(sym)==symbol.trapped_wire.untrapped_write then
            symbol.set_var(sym,val)
        else
            error('TODO')
        end
    else
        error('TODO')
    end
end
---@return fun():table
function M.riter()
    return coroutine.wrap(function ()
        for i=#specpdl,1,-1 do
            coroutine.yield(specpdl[i])
        end

    end)
end
return M
