local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'

---@class nelisp.specpdl.index: number

---@class nelisp.specpdl.entry
---@field type nelisp.specpdl.type

---@class nelisp.specpdl.backtrace_entry: nelisp.specpdl.entry
---@field type nelisp.specpdl.type.backtrace
---@field debug_on_exit boolean
---@field func nelisp.obj
---@field args nelisp.obj[]
---@field nargs number|'UNEVALLED'

---@class nelisp.specpdl.let_entry: nelisp.specpdl.entry
---@field type nelisp.specpdl.type.let
---@field symbol nelisp.obj
---@field old_value nelisp.obj?

---@class nelisp.specpdl.unwind_entry: nelisp.specpdl.entry
---@field type nelisp.specpdl.type.unwind
---@field func function
---@field lisp_eval_depth number

---@alias nelisp.specpdl.all_entries nelisp.specpdl.backtrace_entry|nelisp.specpdl.let_entry|nelisp.specpdl.unwind_entry

---@type (nelisp.specpdl.all_entries)[]
local specpdl={}

local M={}

---@enum nelisp.specpdl.type
M.type={
    backtrace=1,
    unwind=2,
    let=100,
    let_default=101,
}

---@return nelisp.specpdl.index
function M.index()
    return #specpdl+1 --[[@as nelisp.specpdl.index]]
end
---@generic T
---@param index nelisp.specpdl.index
---@param val T
---@return T
function M.unbind_to(index,val,assert_ignore)
    if not assert_ignore then
        assert(index~=M.index(),'DEV: index not changed, unbind_to may be unnecessary')
    end
    while M.index()>index do
        ---@type nelisp.specpdl.all_entries
        local entry=table.remove(specpdl)
        if entry.type==M.type.let and lisp.symbolp(entry.symbol) and
            (entry.symbol --[[@as nelisp._symbol]]).redirect==lisp.symbol_redirect.plainval then
            if (entry.symbol --[[@as nelisp._symbol]]).trapped_write==lisp.symbol_trapped_write.untrapped then
                lisp.set_symbol_val(entry.symbol --[[@as nelisp._symbol]],entry.old_value)
            else
                error('TODO')
            end
        elseif entry.type==M.type.let or entry.type==M.type.let_default then
            error('TODO')
        elseif entry.type==M.type.backtrace then
        elseif entry.type==M.type.unwind then
            vars.lisp_eval_depth=entry.lisp_eval_depth
            entry.func()
        else
            error('TODO')
        end
    end
    return val
end
---@param func nelisp.obj
---@param args nelisp.obj[]
---@param nargs number|'UNEVALLED'
function M.record_in_backtrace(func,args,nargs)
    local index=M.index()
    table.insert(specpdl,{
        type=M.type.backtrace --[[@as nelisp.specpdl.type.backtrace]],
        debug_on_exit=false,
        func=func,
        args=args,
        nargs=nargs,
    } --[[@as nelisp.specpdl.backtrace_entry]])
    return index
end
function M.record_unwind_protect(func)
    table.insert(specpdl,{
        type=M.type.unwind --[[@as nelisp.specpdl.type.unwind]],
        func=func,
        lisp_eval_depth=vars.lisp_eval_depth,
    } --[[@as nelisp.specpdl.unwind_entry]])
end
---@param index nelisp.specpdl.index
function M.backtrace_debug_on_exit(index)
    local entry=specpdl[index]
    assert(entry.type==M.type.backtrace)
    return entry.debug_on_exit
end
---@param args nelisp.obj[]
---@param index nelisp.specpdl.index
function M.set_backtrace_args(index,args)
    local entry=specpdl[index]
    assert(entry.type==M.type.backtrace)
    entry.args=args
    entry.nargs=#args
end
---@param sym nelisp.obj
---@param val nelisp.obj
function M.bind(sym,val)
    lisp.check_symbol(sym)
    local s=sym --[[@as nelisp._symbol]]
    if s.redirect==lisp.symbol_redirect.plainval then
        table.insert(specpdl,{
            type=M.type.let --[[@as nelisp.specpdl.type.let]],
            symbol=sym,
            old_value=lisp.symbol_val(s)
        } --[[@as nelisp.specpdl.let_entry]])
        if s.trapped_write==lisp.symbol_trapped_write.untrapped then
            lisp.set_symbol_val(s,val)
        else
            error('TODO')
        end
    else
        error('TODO')
    end
end
---@return fun():nelisp.specpdl.all_entries
function M.riter()
    return coroutine.wrap(function ()
        for i=#specpdl,1,-1 do
            coroutine.yield(specpdl[i])
        end
    end)
end
---@return nelisp.specpdl.backtrace_entry?
function M.backtrace_next()
    for i in M.riter() do
        if i.type==M.type.backtrace then
            return i --[[@as nelisp.specpdl.backtrace_entry]]
        end
    end
    return nil
end
return M
