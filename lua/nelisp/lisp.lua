local types=require'nelisp.obj.types'
local cons=require'nelisp.obj.cons'
local vars=require'nelisp.vars'
local fixnum=require'nelisp.obj.fixnum'
local str=require'nelisp.obj.str'
local vec=require'nelisp.obj.vec'
local symbol=require'nelisp.obj.symbol'
local b=require'nelisp.bytes'
local type_of=types.type
local M={}
---@overload fun(x:nelisp.obj):boolean
function M.symbolp(x)
    assert(type_of(x)~=types.symbol_with_pos,'TODO')
    return type_of(x)==types.symbol
end
---@overload fun(x:nelisp.obj):boolean
function M.functionp(x)
    if M.symbolp(x) and not M.nilp(vars.F.fboundp(x)) then
        error('TODO')
    end
    if M.subrp(x) then
        error('TODO')
    elseif M.compiledp(x) or M.module_functionp(x) then
        return true
    elseif M.consp(x) then
        error('TODO')
    end
    return false
end
---@overload fun(x:nelisp.obj):boolean
function M.subr_native_compiled_dynp(_) return false end
---@overload fun(x:nelisp.obj):boolean
function M.integerp(x) return M.fixnump(x) or M.bignump(x) end
---@overload fun(x:nelisp.obj):boolean
function M.numberp(x) return M.integerp(x) or M.floatp(x) end
---@overload fun(x:nelisp.obj):boolean
function M.fixnatp(x) return M.fixnump(x) and 0<=fixnum.tonumber(x --[[@as nelisp.fixnum]]) end
---@overload fun(x:nelisp.obj):boolean
function M.arrayp(x) return M.vectorp(x) or M.stringp(x) or M.chartablep(x) or M.bool_vectorp(x) end
---@overload fun(x:nelisp.symbol):boolean
function M.symbolconstantp(x) return symbol.get_trapped_wire(x)==symbol.trapped_wire.nowrite end
---@overload fun(x:nelisp.obj):boolean
function M.vectorlikep(x) return type_of(x)>=types._normal_vector or type_of(x)==types.vec end

---@overload fun(x:nelisp.obj):boolean
function M.baresymbolp(x) return type_of(x)==types.symbol end
---@overload fun(x:nelisp.obj):boolean
function M.vectorp(x) return type_of(x)==types.vec end
---@overload fun(x:nelisp.obj):boolean
function M.stringp(x) return type_of(x)==types.str end
---@overload fun(x:nelisp.obj):boolean
function M.consp(x) return type_of(x)==types.cons end
---@overload fun(x:nelisp.obj):boolean
function M.subrp(x) return type_of(x)==types.subr end
---@overload fun(x:nelisp.obj):boolean
function M.fixnump(x) return type_of(x)==types.fixnum end
---@overload fun(x:nelisp.obj):boolean
function M.bignump(x) return type_of(x)==types.bignum end
---@overload fun(x:nelisp.obj):boolean
function M.bufferp(x) return type_of(x)==types.buffer end
---@overload fun(x:nelisp.obj):boolean
function M.compiledp(x) return type_of(x)==types.compiled end
---@overload fun(x:nelisp.obj):boolean
function M.module_functionp(x) return type_of(x)==types.module_function end
---@overload fun(x:nelisp.obj):boolean
function M.chartablep(x) return type_of(x)==types.char_table end
---@overload fun(x:nelisp.obj):boolean
function M.floatp(x) return type_of(x)==types.float end
---@overload fun(x:nelisp.obj):boolean
function M.recodrp(x) return type_of(x)==types.record end
---@overload fun(x:nelisp.obj):boolean
function M.markerp(x) return type_of(x)==types.marker end
---@overload fun(x:nelisp.obj):boolean
function M.hashtablep(x) return type_of(x)==types.hash_table end

---@overload fun(x:nelisp.obj):boolean
function M.nilp(x) return x==vars.Qnil end


---@param x nelisp.obj
---@param y nelisp.obj
---@return boolean
function M.eq(x,y)
    assert(type_of(x)~=types.symbol_with_pos,'TODO')
    assert(type_of(y)~=types.symbol_with_pos,'TODO')
    return x==y
end

local function wrong_type_argument(predicate,x)
    _={predicate,x}
    --TODO
    error('No error handling yet, error: wrong_type_argument}')
end

---@param ok boolean
---@param predicate nelisp.obj
---@param x nelisp.obj
function M.check_type(ok,predicate,x)
    if not ok then
        wrong_type_argument(predicate,x)
    end
end
---@overload fun(x:nelisp.obj)
function M.check_list(x) M.check_type(M.consp(x) or M.nilp(x),vars.Qlistp,x) end
---@overload fun(x:nelisp.obj,y:nelisp.obj)
function M.check_list_end(x,y) M.check_type(M.nilp(x),vars.Qlistp,y) end
---@overload fun(x:nelisp.obj)
function M.check_symbol(x) M.check_type(M.symbolp(x),vars.Qsymbolp,x) end
---@overload fun(x:nelisp.obj)
function M.check_integer(x) M.check_type(M.integerp(x),vars.Qintegerp,x) end
---@overload fun(x:nelisp.obj)
function M.check_string(x) M.check_type(M.stringp(x),vars.Qstringp,x) end
---@overload fun(x:nelisp.obj)
function M.check_cons(x) M.check_type(M.consp(x),vars.Qconsp,x) end
---@overload fun(x:nelisp.obj)
function M.check_fixnat(x) M.check_type(M.fixnatp(x),vars.Qwholenump,x) end
---@overload fun(x:nelisp.obj)
function M.check_fixnum(x) M.check_type(M.fixnump(x),vars.Qfixnump,x) end
---@overload fun(x:nelisp.obj)
function M.check_array(x,predicate) M.check_type(M.arrayp(x),predicate,x) end
---@overload fun(x:nelisp.obj)
function M.check_chartable(x) M.check_type(M.chartablep(x),vars.Qchartablep,x) end
---@overload fun(x:nelisp.obj)
function M.check_vector(x) M.check_type(M.vectorp(x),vars.Qvectorp,x) end

---@param x nelisp.obj
---@return number
function M.check_vector_or_string(x)
    if M.vectorp(x) then
        return vec.length(x --[[@as nelisp.vec]])
    elseif M.stringp(x) then
        return M.schars(x --[[@as nelisp.str]])
    else
        require'nelisp.signal'.wrong_type_argument(vars.Qarrayp,x)
        error('unreachable')
    end
end
---@param x nelisp.obj
---@return nelisp.fixnum|nelisp.bignum|nelisp.float
function M.check_number_coerce_marker(x)
    if M.markerp(x) then
        error('TODO')
    end
    M.check_type(M.numberp(x),vars.Qnumber_or_marker_p,x)
    return x --[[@as nelisp.fixnum|nelisp.bignum|nelisp.float]]
end
---@param x nelisp.cons
function M.check_string_car(x)
    M.check_type(M.stringp(M.xcar(x)),vars.Qstringp,M.xcar(x))
end

---@param x nelisp.obj
---@param fn fun(x:nelisp.cons):'continue'|'break'|nelisp.obj?
---@param safe boolean?
function M.for_each_tail(x,fn,safe)
    local has_visited={}
    while M.consp(x) do
        if has_visited[x] then
            if not safe then
                require'nelisp.signal'.xsignal(vars.Qcircular_list,x)
            end
            return nil,x
        end
        has_visited[x]=true
        ---@cast x nelisp.cons
        local result=fn(x)
        if result=='break' then
            return nil,x
        elseif result=='continue' then
        elseif result then
            return result --[[@as nelisp.obj]],x
        end
        x=M.xcdr(x)
    end
    return nil,x
end
---@param x nelisp.obj
---@param fn fun(x:nelisp.cons):'continue'|'break'|nelisp.obj?
function M.for_each_tail_safe(x,fn)
    return M.for_each_tail(x,fn,true)
end
---@param x nelisp.obj
---@return number
function M.list_length(x)
    local i=0
    local _,list=M.for_each_tail(x,function ()
        i=i+1
    end)
    M.check_list_end(list,list)
    return i
end
---@param ... nelisp.obj
---@return nelisp.cons
function M.list(...)
    local args={...}
    local val=vars.Qnil
    for i=#args,1,-1 do
        val=vars.F.cons(args[i],val)
    end
    return val
end

---@param x nelisp.obj
function M.loadhist_attach(x)
    if not _G.nelisp_later then
        error('TODO')
    end
end

---@param x nelisp.str
function M.schars(x)
    return str.char_length(x)
end
---@param x nelisp.str
function M.sbytes(x)
    return str.byte_length(x)
end
---@param x nelisp.str
---@return string
function M.sdata(x)
    return str.data(x)
end

---@param c nelisp.cons
function M.xcar(c)
    return cons.car(c)
end
---@param c nelisp.cons
function M.xcdr(c)
    return cons.cdr(c)
end
---@param c nelisp.cons
---@param newcdr nelisp.obj
function M.setcdr(c,newcdr)
    cons.setcdr(c,newcdr)
end
---@param c nelisp.cons
---@param newcar nelisp.obj
function M.setcar(c,newcar)
    cons.setcar(c,newcar)
end

function M.event_head(event)
    return M.consp(event) and M.xcar(event) or event
end

--This is set in `configure.ac` in gnu-emacs
M.IS_DIRECTORY_SEP=function (c)
    --TODO: change depending on operating system
    return c==b'/'
end

return M
