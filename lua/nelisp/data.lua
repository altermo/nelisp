local lisp=require'nelisp.lisp'
local symbol=require'nelisp.obj.symbol'
local vars=require'nelisp.vars'
local cons=require'nelisp.obj.cons'

local M={}
---@param sym nelisp.obj
---@param newval nelisp.obj
---@param where nelisp.obj
---@param bindflag 'SET'|'BIND'|'UNBIND'|'THREAD_SWITCH'
function M.set_internal(sym,newval,where,bindflag)
    local voide=(newval==vars.Qunbound)
    lisp.check_symbol(sym)
    ---@cast sym nelisp.symbol
    local tapped_wire=symbol.get_tapped_wire(sym)
    if tapped_wire==symbol.trapped_wire.nowrite then
        error('TODO')
    elseif tapped_wire==symbol.trapped_wire.trapped_write then
        error('TODO')
    end
    local redirect=symbol.get_redirect(sym)
    if redirect==symbol.redirect.plain then
        symbol.set_var(sym,newval) --TODO: newval may be Qunbound
    else
        error('TODO')
    end
end

local F={}
F.symbol_value={'symbol-value',1,1,0,[[Return SYMBOL's value.  Error if that is void.
Note that if `lexical-binding' is in effect, this returns the
global value outside of any lexical scope.]]}
---@param sym nelisp.obj
---@return nelisp.obj
function F.symbol_value.f(sym)
    lisp.check_symbol(sym)
    ---@cast sym nelisp.symbol
    local val=symbol.get_var(sym)
    if val then
        return val
    end
    error('TODO: err: '..sym[2][2])
end
F.bare_symbol={'bare-symbol',1,1,0,[[Extract, if need be, the bare symbol from SYM, a symbol.]]}
function F.bare_symbol.f(sym)
    if lisp.symbolp(sym) then
        return sym
    end
    error('TODO')
end
F.set={'set',2,2,0,[[Set SYMBOL's value to NEWVAL, and return NEWVAL.]]}
---@param sym nelisp.obj
---@param newval nelisp.obj
function F.set.f(sym,newval)
    M.set_internal(sym,newval,vars.Qnil,'SET')
end
F.car={'car',1,1,0,[[Return the car of LIST.  If LIST is nil, return nil.
Error if LIST is not nil and not a cons cell.  See also `car-safe'.

See Info node `(elisp)Cons Cells' for a discussion of related basic
Lisp concepts such as car, cdr, cons cell and list.]]}
function F.car.f(list)
    if lisp.consp(list) then
        return cons.car(list)
    elseif lisp.nilp(list) then
        return list
    else
        error('TODO: err')
    end
end
F.car_safe={'car-safe',1,1,0,[[Return the car of OBJECT if it is a cons cell, or else nil.]]}
function F.car_safe.f(obj)
    return lisp.consp(obj) and cons.car(obj) or vars.Qnil
end
F.cdr={'cdr',1,1,0,[[Return the cdr of LIST.  If LIST is nil, return nil.
Error if LIST is not nil and not a cons cell.  See also `cdr-safe'.

See Info node `(elisp)Cons Cells' for a discussion of related basic
Lisp concepts such as cdr, car, cons cell and list.]]}
function F.cdr.f(list)
    if lisp.consp(list) then
        return cons.cdr(list)
    elseif lisp.nilp(list) then
        return list
    else
        error('TODO: err')
    end
end
F.setcdr={'setcdr',2,2,0,[[Set the cdr of CELL to be NEWCDR.  Returns NEWCDR.]]}
function F.setcdr.f(cell,newcdr)
    lisp.check_cons(cell)
    cons.setcdr(cell,newcdr)
    return newcdr
end
F.eq={'eq',2,2,0,[[Return t if the two args are the same Lisp object.]]}
function F.eq.f(a,b)
    if lisp.eq(a,b) then
        return vars.Qt
    end
    return vars.Qnil
end
F.fset={'fset',2,2,0,[[Set SYMBOL's function definition to DEFINITION, and return DEFINITION.]]}
function F.fset.f(sym,definition)
    lisp.check_symbol(sym)
    if lisp.nilp(sym) and not lisp.nilp(definition) then
        error('TODO: err')
    end
    symbol.set_func(sym,definition)
    return definition
end
F.defalias={'defalias',2,3,0,[[Set SYMBOL's function definition to DEFINITION.
Associates the function with the current load file, if any.
The optional third argument DOCSTRING specifies the documentation string
for SYMBOL; if it is omitted or nil, SYMBOL uses the documentation string
determined by DEFINITION.

Internally, this normally uses `fset', but if SYMBOL has a
`defalias-fset-function' property, the associated value is used instead.

The return value is undefined.]]}
function F.defalias.f(sym,definition,docstring)
    lisp.check_symbol(sym)
    if not _G.nelisp_later then
        error('TODO')
    end
    vars.F.fset(sym,definition)
    if not lisp.nilp(docstring) then
        vars.F.put(sym,vars.Qfunction_documentation,docstring)
    end
    return sym
end
---@param bindflag 'SET'|'BIND'|'UNBIND'|'THREAD_SWITCH'
function M.set_default_internal(sym,val,bindflag)
    lisp.check_symbol(sym)
    local trapped_wire=symbol.get_tapped_wire(sym)
    if trapped_wire==symbol.trapped_wire.trapped_write then
        error('TODO')
    elseif trapped_wire==symbol.trapped_wire.nowrite then
        error('TODO')
    end
    local redirect=symbol.get_redirect(sym)
    if redirect==symbol.redirect.plain then
        M.set_internal(sym,val,vars.Qnil,bindflag)
        return
    else
        error('TODO')
    end
end
F.set_default={'set-default',2,2,0,[[Set SYMBOL's default value to VALUE.  SYMBOL and VALUE are evaluated.
The default value is seen in buffers that do not have their own values
for this variable.]]}
F.set_default.f=function (sym,val)
    M.set_default_internal(sym,val,'SET')
    return val
end
local function default_value(sym)
    lisp.check_symbol(sym)
    local redirect=symbol.get_redirect(sym)
    if redirect==symbol.redirect.plain then
        return symbol.get_var(sym)
    else
        error('TODO')
    end
end
F.default_boundp={'default-boundp',1,1,0,[[Return t if SYMBOL has a non-void default value.
A variable may have a buffer-local value.  This function says whether
the variable has a non-void value outside of the current buffer
context.  Also see `default-value'.]]}
function F.default_boundp.f(sym)
    local value=default_value(sym)
    return value==nil and vars.Qnil or vars.Qt
end
F.stringp={'stringp',1,1,0,[[Return t if OBJECT is a string.]]}
function F.stringp.f(a) return lisp.stringp(a) and vars.Qt or vars.Qnil end
F.null={'null',1,1,0,[[Return t if OBJECT is nil, and return nil otherwise.]]}
function F.null.f(a) return lisp.nilp(a) and vars.Qt or vars.Qnil end
F.numberp={'numberp',1,1,0,[[Return t if OBJECT is a number (floating point or integer).]]}
function F.numberp.f(a) return lisp.numberp(a) and vars.Qt or vars.Qnil end
F.listp={'listp',1,1,0,[[Return t if OBJECT is a list, that is, a cons cell or nil.
Otherwise, return nil.]]}
function F.listp.f(a) return (lisp.consp(a) or lisp.nilp(a)) and vars.Qt or vars.Qnil end
F.symbolp={'symbolp',1,1,0,[[Return t if OBJECT is a symbol.]]}
function F.symbolp.f(a) return lisp.symbolp(a) and vars.Qt or vars.Qnil end

function M.init_syms()
    vars.setsubr(F,'symbol_value')
    vars.setsubr(F,'bare_symbol')
    vars.setsubr(F,'car')
    vars.setsubr(F,'car_safe')
    vars.setsubr(F,'cdr')
    vars.setsubr(F,'setcdr')

    vars.setsubr(F,'fset')
    vars.setsubr(F,'set')
    vars.setsubr(F,'set_default')

    vars.setsubr(F,'eq')
    vars.setsubr(F,'defalias')

    vars.setsubr(F,'default_boundp')
    vars.setsubr(F,'stringp')
    vars.setsubr(F,'null')
    vars.setsubr(F,'numberp')
    vars.setsubr(F,'listp')
    vars.setsubr(F,'symbolp')

    vars.defsym('Qquote','quote')
    vars.defsym('Qlambda','lambda')

    vars.defsym('Qlistp','listp')
    vars.defsym('Qsymbolp','symbolp')
    vars.defsym('Qintegerp','integerp')
    vars.defsym('Qstringp','stringp')
    vars.defsym('Qconsp','consp')
end
return M
