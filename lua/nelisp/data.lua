local lisp=require'nelisp.lisp'
local symbol=require'nelisp.obj.symbol'
local vars=require'nelisp.vars'
local signal=require'nelisp.signal'
local fixnum=require'nelisp.obj.fixnum'
local str=require'nelisp.obj.str'
local char_table=require'nelisp.obj.char_table'
local vec=require'nelisp.obj.vec'
local lread=require'nelisp.lread'
local overflow=require'nelisp.overflow'
local cons=require'nelisp.obj.cons'

local M={}
---@param sym nelisp.obj
---@param newval nelisp.obj
---@param where nelisp.obj
---@param bindflag 'SET'|'BIND'|'UNBIND'|'THREAD_SWITCH'
function M.set_internal(sym,newval,where,bindflag)
    lisp.check_symbol(sym)
    ---@cast sym nelisp.symbol
    local tapped_wire=symbol.get_trapped_wire(sym)
    if tapped_wire==symbol.trapped_wire.nowrite then
        error('TODO')
    elseif tapped_wire==symbol.trapped_wire.trapped_write then
        error('TODO')
    end
    local redirect=symbol.get_redirect(sym)
    if redirect==symbol.redirect.plain then
        symbol.set_var(sym,newval)
    else
        error('TODO')
    end
end
---@return nelisp.obj?
function M.find_symbol_value(sym)
    lisp.check_symbol(sym)
    local redirect=symbol.get_redirect(sym)
    if redirect==symbol.redirect.plain then
        return symbol.get_var(sym)
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
    signal.xsignal(vars.Qvoid_variable,sym)
    error('unreachable')
end
---@return nelisp.obj?
local function default_value(sym)
    lisp.check_symbol(sym)
    local redirect=symbol.get_redirect(sym)
    if redirect==symbol.redirect.plain then
        return symbol.get_var(sym)
    else
        error('TODO')
    end
end
F.default_value={'default-value',1,1,0,[[Return SYMBOL's default value.
This is the value that is seen in buffers that do not have their own values
for this variable.  The default value is meaningful for variables with
local bindings in certain buffers.]]}
function F.default_value.f(sym)
    local val=default_value(sym)
    if val then return val end
    signal.xsignal(vars.Qvoid_variable,sym)
end
F.symbol_function={'symbol-function',1,1,0,[[Return SYMBOL's function definition, or nil if that is void.]]}
function F.symbol_function.f(sym)
    lisp.check_symbol(sym)
    return symbol.get_func(sym)
end
F.symbol_name={'symbol-name',1,1,0,[[Return SYMBOL's name, a string.

Warning: never alter the string returned by `symbol-name'.
Doing that might make Emacs dysfunctional, and might even crash Emacs.]]}
function F.symbol_name.f(sym)
    lisp.check_symbol(sym)
    return symbol.get_name(sym)
end
F.bare_symbol={'bare-symbol',1,1,0,[[Extract, if need be, the bare symbol from SYM, a symbol.]]}
function F.bare_symbol.f(sym)
    if lisp.symbolp(sym) then
        return sym
    end
    error('TODO')
end
function M.indirect_function(obj)
    local has_visited={}
    ---@type nelisp.obj
    local hare=obj
    while true do
        if not lisp.symbolp(hare) or lisp.nilp(hare) then
            return hare
        end
        hare=symbol.get_func(hare --[[@as nelisp.symbol]])
        if has_visited[hare] then
            signal.xsignal(vars.Qcyclic_function_indirection,obj)
        end
        has_visited[hare]=true
    end
end
F.indirect_function={'indirect-function',1,2,0,[[Return the function at the end of OBJECT's function chain.
If OBJECT is not a symbol, just return it.  Otherwise, follow all
function indirections to find the final function binding and return it.
Signal a cyclic-function-indirection error if there is a loop in the
function chain of symbols.]]}
function F.indirect_function.f(obj,_)
    local result=obj
    if lisp.symbolp(result) and not lisp.nilp(result) then
        result=symbol.get_func(result)
        if lisp.symbolp(result) then
            result=M.indirect_function(result)
        end
    end
    return result
end
F.aref={'aref',2,2,0,[[Return the element of ARRAY at index IDX.
ARRAY may be a vector, a string, a char-table, a bool-vector, a record,
or a byte-code object.  IDX starts at 0.]]}
function F.aref.f(array,idx)
    lisp.check_fixnum(idx)
    local idxval=fixnum.tonumber(idx)
    if lisp.stringp(array) then
        if idxval<0 or idxval>=lisp.schars(array) then
            signal.args_out_of_range(array,idx)
        elseif not str.is_multibyte(array) then
            return fixnum.make(str.index0(array,idxval))
        end
        error('TODO')
    elseif lisp.vectorp(array) then
        if idxval<0 or idxval>=vec.length(array) then
            signal.args_out_of_range(array,idx)
        end
        return vec.index0(array,idxval)
    elseif lisp.chartablep(array) then
        lisp.check_chartable(array)
        return char_table.get(array,idxval)
    else
        error('TODO')
    end
end
F.aset={'aset',3,3,0,[[Store into the element of ARRAY at index IDX the value NEWELT.
Return NEWELT.  ARRAY may be a vector, a string, a char-table or a
bool-vector.  IDX starts at 0.]]}
function F.aset.f(array,idx,newval)
    lisp.check_fixnum(idx)
    local idxval=fixnum.tonumber(idx)
    if not lisp.recodrp(array) then
        lisp.check_array(array,vars.Qarrayp)
    end
    if lisp.chartablep(array) then
        lisp.check_chartable(array)
        char_table.set(array,idxval,newval)
    elseif lisp.vectorp(array) then
        lisp.check_vector(array)
        if idxval<0 or idxval>=vec.length(array) then
            error('INDEX out of range')
        end
        vec.set_index0(array,idxval,newval)
    else
        error('TODO')
    end
    return newval
end
F.set={'set',2,2,0,[[Set SYMBOL's value to NEWVAL, and return NEWVAL.]]}
---@param sym nelisp.obj
---@param newval nelisp.obj
function F.set.f(sym,newval)
    M.set_internal(sym,newval,vars.Qnil,'SET')
    return newval
end
F.car={'car',1,1,0,[[Return the car of LIST.  If LIST is nil, return nil.
Error if LIST is not nil and not a cons cell.  See also `car-safe'.

See Info node `(elisp)Cons Cells' for a discussion of related basic
Lisp concepts such as car, cdr, cons cell and list.]]}
function F.car.f(list)
    if lisp.consp(list) then
        return lisp.xcar(list)
    elseif lisp.nilp(list) then
        return list
    else
        signal.wrong_type_argument(vars.Qlistp,list)
    end
end
F.car_safe={'car-safe',1,1,0,[[Return the car of OBJECT if it is a cons cell, or else nil.]]}
function F.car_safe.f(obj)
    return lisp.consp(obj) and lisp.xcar(obj) or vars.Qnil
end
F.cdr={'cdr',1,1,0,[[Return the cdr of LIST.  If LIST is nil, return nil.
Error if LIST is not nil and not a cons cell.  See also `cdr-safe'.

See Info node `(elisp)Cons Cells' for a discussion of related basic
Lisp concepts such as cdr, car, cons cell and list.]]}
function F.cdr.f(list)
    if lisp.consp(list) then
        return lisp.xcdr(list)
    elseif lisp.nilp(list) then
        return list
    else
        signal.wrong_type_argument(vars.Qlistp,list)
    end
end
F.setcdr={'setcdr',2,2,0,[[Set the cdr of CELL to be NEWCDR.  Returns NEWCDR.]]}
function F.setcdr.f(cell,newcdr)
    lisp.check_cons(cell)
    lisp.setcdr(cell,newcdr)
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
        signal.xsignal(vars.Qsetting_constant,sym)
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
F.make_variable_buffer_local={'make-variable-buffer-local',1,1,'vMake Variable Buffer Local: ',[[Make VARIABLE become buffer-local whenever it is set.
At any time, the value for the current buffer is in effect,
unless the variable has never been set in this buffer,
in which case the default value is in effect.
Note that binding the variable with `let', or setting it while
a `let'-style binding made in this buffer is in effect,
does not make the variable buffer-local.  Return VARIABLE.

This globally affects all uses of this variable, so it belongs together with
the variable declaration, rather than with its uses (if you just want to make
a variable local to the current buffer for one particular use, use
`make-local-variable').  Buffer-local bindings are normally cleared
while setting up a new major mode, unless they have a `permanent-local'
property.

The function `default-value' gets the default value and `set-default' sets it.

See also `defvar-local'.]]}
function F.make_variable_buffer_local.f(var)
    lisp.check_symbol(var)
    local redirect=symbol.get_redirect(var)
    local default
    if redirect==symbol.redirect.plain then
        default=symbol.get_var(var) or vars.Qnil
    else
        error('TODO')
    end
    symbol.set_redirect(var,symbol.redirect.localized)
    symbol.set_blv(var,{default=default,local_if_set=true})
    return var
end
---@param bindflag 'SET'|'BIND'|'UNBIND'|'THREAD_SWITCH'
function M.set_default_internal(sym,val,bindflag)
    lisp.check_symbol(sym)
    local trapped_wire=symbol.get_trapped_wire(sym)
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
---@param code '+'|'-'|'or'
---@param args (number|nelisp.float|nelisp.bignum|nelisp.fixnum)[]
local function arith_driver(code,args)
    if not _G.nelisp_later then
        error('TODO: args may contain markers')
    end
    if code=='+' then
        ---@type number|nil
        local acc=0
        for _,v in ipairs(args) do
            if type(v)=='number' then
                acc=overflow.add(acc,v)
            elseif lisp.bignump(v) then
                error('TODO')
            elseif lisp.floatp(v) then
                error('TODO')
            elseif lisp.fixnump(v) then
                acc=overflow.add(acc,fixnum.tonumber(v --[[@as nelisp.fixnum]]))
            else
                error('unreachable')
            end
        end
        if acc==nil then
            error('TODO')
        end
        return fixnum.make(acc)
    elseif code=='-' then
        ---@type number|nil
        local acc=0
        for _,v in ipairs(args) do
            if type(v)=='number' then
                acc=overflow.sub(acc,v)
            elseif lisp.bignump(v) then
                error('TODO')
            elseif lisp.floatp(v) then
                error('TODO')
            elseif lisp.fixnump(v) then
                acc=overflow.sub(acc,fixnum.tonumber(v --[[@as nelisp.fixnum]]))
            else
                error('unreachable')
            end
        end
        if acc==nil then
            error('TODO')
        end
        return fixnum.make(acc)
    elseif code=='or' then
        if not _G.nelisp_later then
            error('TODO: bit can only do numbers up to 32 bit, fixnum is 52 bit')
            error('TODO: a number being negative is treated as it having infinite ones at the left side')
        end
        local acc=0
        for _,v in ipairs(args) do
            if type(v)=='number' then
                acc=bit.bor(acc,v)
            elseif lisp.bignump(v) then
                error('TODO')
            elseif lisp.floatp(v) then
                error('TODO')
            elseif lisp.fixnump(v) then
                acc=bit.bor(acc,fixnum.tonumber(v --[[@as nelisp.fixnum]]))
            else
                error('unreachable')
            end
        end
        return acc
    else
        error('TODO')
    end
end
---@param code '='|'<='|'/='
---@return boolean
local function arithcompare(a,b,code)
    if not _G.nelisp_later then
        error('TODO: args may contain markers')
    end
    local lt,gt,eq
    if lisp.fixnump(a) then
        if lisp.fixnump(b) then
            local i1=fixnum.tonumber(a --[[@as nelisp.fixnum]])
            local i2=fixnum.tonumber(b --[[@as nelisp.fixnum]])
            lt=i1<i2
            gt=i1>i2
            eq=i1==i2
        else
            error('TODO')
        end
    else
        error('TODO')
    end

    if code=='=' then
        return eq
    elseif code=='/=' then
        return not eq
    elseif code=='<=' then
        return lt or eq
    elseif code=='<' then
        return lt
    elseif code=='>=' then
        return gt or eq
    elseif code=='>' then
        return gt
    else
        error('TODO')
    end
end
---@param code '='|'<='
---@param args (number|nelisp.float|nelisp.bignum|nelisp.fixnum)[]
local function arithcompare_driver(code,args)
    for i=1,#args-1 do
        if not arithcompare(args[i],args[i+1],code) then
            return vars.Qnil
        end
    end
    return vars.Qt
end
F.add1={'1+',1,1,0,[[Return NUMBER plus one.  NUMBER may be a number or a marker.
Markers are converted to integers.]]}
function F.add1.f(num)
    num=lisp.check_number_coerce_marker(num)
    if lisp.fixnump(num) then
        return arith_driver('+',{num,1})
    else
        error('TODO')
    end
end
F.sub1={'1-',1,1,0,[[Return NUMBER minus one.  NUMBER may be a number or a marker.
Markers are converted to integers.]]}
function F.sub1.f(num)
    num=lisp.check_number_coerce_marker(num)
    if lisp.fixnump(num) then
        return arith_driver('-',{num,1})
    else
        error('TODO')
    end
end
F.lss={'<',1,-2,0,[[Return t if each arg (a number or marker), is less than the next arg.
usage: (< NUMBER-OR-MARKER &rest NUMBERS-OR-MARKERS)]]}
function F.lss.f(args)
    if #args==2 and lisp.fixnump(args[1]) and lisp.fixnump(args[2]) then
        return fixnum.tonumber(args[1])<fixnum.tonumber(args[2]) and vars.Qt or vars.Qnil
    end
    error('TODO')
end
F.leq={'<=',1,-2,0,[[Return t if each arg (a number or marker) is less than or equal to the next.
usage: (<= NUMBER-OR-MARKER &rest NUMBERS-OR-MARKERS)]]}
function F.leq.f(args)
    if #args==2 and lisp.fixnump(args[1]) and lisp.fixnump(args[2]) then
        return fixnum.tonumber(args[1])<=fixnum.tonumber(args[2]) and vars.Qt or vars.Qnil
    end
    return arithcompare_driver('<=',args)
end
F.gtr={'>',1,-2,0,[[Return t if each arg (a number or marker) is greater than the next arg.
usage: (> NUMBER-OR-MARKER &rest NUMBERS-OR-MARKERS)]]}
function F.gtr.f(args)
    if #args==2 and lisp.fixnump(args[1]) and lisp.fixnump(args[2]) then
        return fixnum.tonumber(args[1])>fixnum.tonumber(args[2]) and vars.Qt or vars.Qnil
    end
    error('TODO')
end
F.geq={'>=',1,-2,0,[[Return t if each arg (a number or marker) is greater than or equal to the next.
usage: (>= NUMBER-OR-MARKER &rest NUMBERS-OR-MARKERS)]]}
function F.geq.f(args)
    if #args==2 and lisp.fixnump(args[1]) and lisp.fixnump(args[2]) then
        return fixnum.tonumber(args[1])>=fixnum.tonumber(args[2]) and vars.Qt or vars.Qnil
    end
    error('TODO')
end
F.logior={'logior',0,-2,0,[[Return bitwise-or of all the arguments.
Arguments may be integers, or markers converted to integers.
usage: (logior &rest INTS-OR-MARKERS)]]}
function F.logior.f(args)
    if #args==0 then
        return fixnum.zero
    end
    local a=lisp.check_number_coerce_marker(args[1])
    return #args==1 and a or arith_driver('or',args)
end
F.eqlsign={'=',1,-2,0,[[Return t if args, all numbers or markers, are equal.
usage: (= NUMBER-OR-MARKER &rest NUMBERS-OR-MARKERS)]]}
function F.eqlsign.f(args)
    return arithcompare_driver('=',args)
end
F.neq={'/=',2,2,0,[[Return t if first arg is not equal to second arg.  Both must be numbers or markers.]]}
function F.neq.f(a,b)
    return arithcompare(a,b,'/=') and vars.Qt or vars.Qnil
end
F.local_variable_if_set_p={'local-variable-if-set-p',1,2,0,[[Non-nil if VARIABLE is local in buffer BUFFER when set there.
BUFFER defaults to the current buffer.

More precisely, return non-nil if either VARIABLE already has a local
value in BUFFER, or if VARIABLE is automatically buffer-local (see
`make-variable-buffer-local').]]}
function F.local_variable_if_set_p.f(variable,buffer)
    lisp.check_symbol(variable)
    local redirect=symbol.get_redirect(variable)
    if redirect==symbol.redirect.plain then
        return vars.Qnil
    end
    error('TODO')
end
F.string_to_number={'string-to-number',1,2,0,[[Parse STRING as a decimal number and return the number.
Ignore leading spaces and tabs, and all trailing chars.  Return 0 if
STRING cannot be parsed as an integer or floating point number.

If BASE, interpret STRING as a number in that base.  If BASE isn't
present, base 10 is used.  BASE must be between 2 and 16 (inclusive).
If the base used is not 10, STRING is always parsed as an integer.]]}
function F.string_to_number.f(s,base)
    lisp.check_string(s)
    local b
    if lisp.nilp(base) then
        b=10
    else
        lisp.check_fixnum(base)
        if not (fixnum.tonumber(base)>=2 and fixnum.tonumber(base)<=16) then
            signal.xsignal(vars.Qargs_out_of_range,base)
        end
        b=fixnum.tonumber(base)
    end
    local p=lisp.sdata(s):gsub('^[ \t]+','')
    local val=lread.string_to_number(p,b)
    return val==nil and fixnum.zero or val
end
F.default_boundp={'default-boundp',1,1,0,[[Return t if SYMBOL has a non-void default value.
A variable may have a buffer-local value.  This function says whether
the variable has a non-void value outside of the current buffer
context.  Also see `default-value'.]]}
function F.default_boundp.f(sym)
    local value=default_value(sym)
    return value==nil and vars.Qnil or vars.Qt
end
F.boundp={'boundp',1,1,0,[[Return t if SYMBOL's value is not void.
Note that if `lexical-binding' is in effect, this refers to the
global value outside of any lexical scope.]]}
function F.boundp.f(sym)
    lisp.check_symbol(sym)
    local redirect=symbol.get_redirect(sym)
    if redirect==symbol.redirect.plain then
        return symbol.get_var(sym)==nil and vars.Qnil or vars.Qt
    else
        error('TODO')
    end
end
F.fboundp={'fboundp',1,1,0,[[Return t if SYMBOL's function definition is not void.]]}
function F.fboundp.f(sym)
    lisp.check_symbol(sym)
    return lisp.nilp(symbol.get_func(sym)) and vars.Qnil or vars.Qt
end
F.keywordp={'keywordp',1,1,0,[[Return t if OBJECT is a keyword.
This means that it is a symbol with a print name beginning with `:'
interned in the initial obarray.]]}
function F.keywordp.f(a)
    if lisp.symbolp(a) and
        str.index1_neg(symbol.get_name(a),1)==(require'nelisp.bytes')[':'] and
        symbol.get_interned(a)==symbol.interned.interned_in_initial_obarray
        then
        return vars.Qt
    end
    return vars.Qnil
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
F.floatp={'floatp',1,1,0,[[Return t if OBJECT is a floating point number.]]}
function F.floatp.f(a) return lisp.floatp(a) and vars.Qt or vars.Qnil end
F.vectorp={'vectorp',1,1,0,[[Return t if OBJECT is a vector.]]}
function F.vectorp.f(a) return lisp.vectorp(a) and vars.Qt or vars.Qnil end
F.atom={'atom',1,1,0,[[Return t if OBJECT is not a cons cell.  This includes nil.]]}
function F.atom.f(a) return lisp.consp(a) and vars.Qnil or vars.Qt end
F.consp={'consp',1,1,0,[[Return t if OBJECT is a cons cell.]]}
function F.consp.f(a) return lisp.consp(a) and vars.Qt or vars.Qnil end
F.functionp={'functionp',1,1,0,[[Return t if OBJECT is a function.

An object is a function if it is callable via `funcall'; this includes
symbols with function bindings, but excludes macros and special forms.

Ordinarily return nil if OBJECT is not a function, although t might be
returned in rare cases.]]}
function F.functionp.f(a) return lisp.functionp(a) and vars.Qt or vars.Qnil end
F.hash_table_p={'hash-table-p',1,1,0,[[Return t if OBJ is a Lisp hash table object.]]}
function F.hash_table_p.f(a) return lisp.hashtablep(a) and vars.Qt or vars.Qnil end
function M.init()
    local error_tail=cons.make(vars.Qerror,vars.Qnil)
    vars.F.put(vars.Qerror,vars.Qerror_conditions,error_tail)
    vars.F.put(vars.Qerror,vars.Qerror_message,str.make('error',false))
    local function put_error(sym,tail,msg)
        vars.F.put(sym,vars.Qerror_conditions,cons.make(sym,tail))
        vars.F.put(sym,vars.Qerror_message,str.make(msg,false))
    end
    put_error(vars.Qquit,vars.Qnil,'Quit')
    put_error(vars.Qvoid_variable,error_tail,"Symbol's value as variable is void")
    put_error(vars.Qvoid_function,error_tail,"Symbol's function definition is void")
end
function M.init_syms()
    ---These are errors and should have corresponding `put_error`
    vars.defsym('Qerror','error')
    vars.defsym('Qquit','quit')
    vars.defsym('Qvoid_variable','void-variable')
    vars.defsym('Qvoid_function','void-function')

    vars.setsubr(F,'symbol_value')
    vars.setsubr(F,'default_value')
    vars.setsubr(F,'symbol_function')
    vars.setsubr(F,'symbol_name')
    vars.setsubr(F,'bare_symbol')
    vars.setsubr(F,'indirect_function')
    vars.setsubr(F,'aref')
    vars.setsubr(F,'aset')
    vars.setsubr(F,'car')
    vars.setsubr(F,'car_safe')
    vars.setsubr(F,'cdr')
    vars.setsubr(F,'setcdr')

    vars.setsubr(F,'fset')
    vars.setsubr(F,'set')
    vars.setsubr(F,'set_default')

    vars.setsubr(F,'eq')
    vars.setsubr(F,'defalias')
    vars.setsubr(F,'make_variable_buffer_local')

    vars.setsubr(F,'add1')
    vars.setsubr(F,'sub1')
    vars.setsubr(F,'logior')
    vars.setsubr(F,'lss')
    vars.setsubr(F,'leq')
    vars.setsubr(F,'gtr')
    vars.setsubr(F,'geq')
    vars.setsubr(F,'eqlsign')
    vars.setsubr(F,'neq')

    vars.setsubr(F,'string_to_number')

    vars.setsubr(F,'local_variable_if_set_p')
    vars.setsubr(F,'default_boundp')
    vars.setsubr(F,'boundp')
    vars.setsubr(F,'fboundp')
    vars.setsubr(F,'keywordp')
    vars.setsubr(F,'stringp')
    vars.setsubr(F,'null')
    vars.setsubr(F,'numberp')
    vars.setsubr(F,'listp')
    vars.setsubr(F,'symbolp')
    vars.setsubr(F,'floatp')
    vars.setsubr(F,'vectorp')
    vars.setsubr(F,'atom')
    vars.setsubr(F,'consp')
    vars.setsubr(F,'functionp')
    vars.setsubr(F,'hash_table_p')

    vars.defsym('Qquote','quote')
    vars.defsym('Qlambda','lambda')
    vars.defsym('Qtop_level','top-level')
    vars.defsym('Qerror_conditions','error-conditions')
    vars.defsym('Qerror_message','error-message')

    vars.defsym('Qlistp','listp')
    vars.defsym('Qsymbolp','symbolp')
    vars.defsym('Qintegerp','integerp')
    vars.defsym('Qstringp','stringp')
    vars.defsym('Qconsp','consp')
    vars.defsym('Qwholenump','wholenump')
    vars.defsym('Qfixnump','fixnump')
    vars.defsym('Qarrayp','arrayp')
    vars.defsym('Qchartablep','chartablep')
    vars.defsym('Qvectorp','vectorp')
    vars.defsym('Qnumber_or_marker_p','number-or-markerp')

    vars.defsym('QCtest',':test')
    vars.defsym('QCsize',':size')
    vars.defsym('QCpurecopy',':purecopy')
    vars.defsym('QCrehash_size',':rehash-size')
    vars.defsym('QCrehash_threshold',':rehash-threshold')
    vars.defsym('QCweakness',':weakness')


end
return M
