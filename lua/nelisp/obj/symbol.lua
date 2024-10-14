local types=require'nelisp.obj.types'
local cons=require'nelisp.obj.cons'

---@class nelisp.symbol: nelisp.obj
---@field [1] nelisp.types.symbol
---@field [2] nelisp.str --name
---@field [3]? nelisp.symbol.trapped_wire --trapped_wire
---@field [4]? nelisp.symbol.redirect --redirect
---@field [5]? nelisp.symbol.interned --interned
---@field [6]? boolean --declared_special
---@field [7]? any --value of symbol (dependent on "redirect")
---@field [8]? nelisp.symbol|nelisp.subr|nelisp.cons --function
---@field [9]? nelisp.list --plist
---@field [10]? nelisp.symbol --next symbol in obarray

local M={}

---@enum nelisp.symbol.redirect
M.redirect={
    plain=nil,
    varalias =1,
    localized=2,
    --forwarded=3 --I don't think I will need this, I can just look upp the global variables
}
---@enum nelisp.symbol.interned
M.interned={
    interned_in_initial_obarray=nil,
    uninterned=1,
    interned=2,
}
---@enum nelisp.symbol.trapped_wire
M.trapped_wire={
    untrapped_write=nil,
    nowrite=1,
    trapped_write=2
}

---@param name nelisp.str
---@return nelisp.symbol
function M.make_uninterned(name)
    ---@type nelisp.symbol
    return {
        types.symbol --[[@as nelisp.types.symbol]],
        name,
        [5]=M.interned.uninterned,
    }
end
---@param name nelisp.str
---@return nelisp.symbol
function M.make_interned(name)
    ---@type nelisp.symbol
    return {
        types.symbol --[[@as nelisp.types.symbol]],
        name,
        [5]=M.interned.interned,
    }
end
---@param name nelisp.str
---@return nelisp.symbol
function M.make_interned_in_initial_obarray(name)
    ---@type nelisp.symbol
    return {
        types.symbol --[[@as nelisp.types.symbol]],
        name,
        [5]=M.interned.interned_in_initial_obarray,
    }
end
---@param sym nelisp.symbol
---@return nelisp.obj?
function M.get_var(sym)
    assert(sym[4]==nil,'TODO')
    return sym[7]
end
---@param sym nelisp.symbol
---@param val nelisp.obj
function M.set_var(sym,val)
    assert(val~=require'nelisp.vars'.Qunbound)
    assert(sym[4]==M.redirect.plain)
    sym[7]=val
end
---@param sym nelisp.symbol
---@return nelisp.symbol.trapped_wire
function M.get_tapped_wire(sym)
    return sym[3]
end
---@param sym nelisp.symbol
---@return nelisp.symbol.redirect
function M.get_redirect(sym)
    return sym[4]
end
---@param sym nelisp.symbol
---@return nelisp.str
function M.get_name(sym)
    return sym[2]
end
---@param sym nelisp.symbol
---@return nelisp.symbol?
function M.get_next(sym)
    return sym[10]
end
---@param sym nelisp.symbol
---@param next nelisp.symbol
function M.set_next(sym,next)
    sym[10]=next
end
---@param sym nelisp.symbol
function M.set_constant(sym)
    sym[3]=M.trapped_wire.nowrite
end
---@param sym nelisp.symbol
function M.set_special(sym)
    sym[6]=true
end
---@param sym nelisp.symbol
function M.get_special(sym)
    return sym[6]
end
---@param sym nelisp.symbol
---@return nelisp.symbol|nelisp.subr|nelisp.cons
function M.get_func(sym)
    return sym[8] or require'nelisp.vars'.Qnil
end
---@param sym nelisp.symbol
---@param func nelisp.subr|nelisp.symbol|nelisp.cons
function M.set_func(sym,func)
    sym[8]=func
end
---@param sym nelisp.symbol
---@param prop nelisp.symbol
---@param val nelisp.obj
function M.put(sym,prop,val)
    local Qnil=require'nelisp.vars'.Qnil
    if not sym[9] then
        sym[9]=Qnil
    end
    local prev=Qnil
    local tail=sym[9] --[[@as nelisp.obj]]
    local has_visited={}
    local lisp=require'nelisp.lisp'
    while lisp.consp(tail) do
        ---@cast tail nelisp.cons
        if not lisp.consp(cons.cdr(tail)) then
            break
        end
        if lisp.eq(cons.car(tail),val) then
            cons.setcar(cons.cdr(tail) --[[@as nelisp.cons]],prop)
            return val
        end
        prev=tail
        tail=cons.cdr(cons.cdr(tail) --[[@as nelisp.cons]])
        if has_visited[tail] then
            error('TODO')
        end
        has_visited[tail]=true
    end
    if not lisp.nilp(tail) then
        error('TODO')
    end
    if lisp.nilp(prev) then
        sym[9]=cons.make(prop,cons.make(val,Qnil))
    else
        cons.setcdr(cons.cdr(prev) --[[@as nelisp.cons]],cons.make(prop,Qnil))
    end
end
return M