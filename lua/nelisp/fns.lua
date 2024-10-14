local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'
local cons=require'nelisp.obj.cons'
local fixnum=require'nelisp.obj.fixnum'
local symbol=require'nelisp.obj.symbol'

local F={}
F.assq={'assq',2,2,0,[[Return non-nil if KEY is `eq' to the car of an element of ALIST.
The value is actually the first element of ALIST whose car is KEY.
Elements of ALIST that are not conses are ignored.]]}
---@param key nelisp.obj
---@param alist nelisp.obj
---@return nelisp.obj
function F.assq.f(key,alist)
    local ret,tail=lisp.for_each_tail(alist,function (tail)
        if lisp.consp(cons.car(tail)) and lisp.eq(cons.car(cons.car(tail) --[[@as nelisp.cons]]),key) then
            return cons.car(tail)
        end
    end)
    if ret then return ret end
    lisp.check_list_end(tail,alist)
    return vars.Qnil
end
F.member={'member',2,2,0,[[Return non-nil if ELT is an element of LIST.  Comparison done with `equal'.
The value is actually the tail of LIST whose car is ELT.]]}
function F.member.f(elt,list)
    if lisp.symbolp(elt) or lisp.fixnump(elt) then
        return vars.F.memq(elt,list)
    end
    error('TODO')
end
F.memq={'memq',2,2,0,[[Return non-nil if ELT is an element of LIST.  Comparison done with `eq'.
The value is actually the tail of LIST whose car is ELT.]]}
function F.memq.f(elt,list)
    local ret,tail=lisp.for_each_tail(list,function (tail)
        if lisp.eq(cons.car(tail),elt) then
            return tail
        end
    end)
    if ret then return ret end
    lisp.check_list_end(tail,list)
    return vars.Qnil
end
F.nthcdr={'nthcdr',2,2,0,[[Take cdr N times on LIST, return the result.]]}
function F.nthcdr.f(n,list)
    local tail=list
    lisp.check_integer(n)
    local num
    if lisp.fixnump(n) then
        num=fixnum.tonumber(n)
        if num<=127 then
            for _=1,num do
                if not lisp.consp(tail) then
                    lisp.check_list_end(tail,list)
                    return vars.Qnil
                end
                tail=cons.cdr(tail)
            end
            return tail
        end
    else
        error('TODO')
    end
    error('TODO')
end
F.nth={'nth',2,2,0,[[Return the Nth element of LIST.
N counts from zero.  If LIST is not that long, nil is returned.]]}
function F.nth.f(n,list)
    return vars.F.car(vars.F.nthcdr(n,list))
end
local function mapcar1(leni,vals,fn,seq)
    if lisp.nilp(seq) then
        return 0
    elseif lisp.consp(seq) then
        local tail=seq
        for i=0,leni-1 do
            if not lisp.consp(tail) then
                return i
            end
            local dummy=vars.F.funcall({fn,cons.car(tail)})
            if vals then
                vals[i+1]=dummy
            end
            tail=cons.cdr(tail)
        end
        error('TODO')
    else
        error('TODO')
    end
    return leni
end

F.mapcar={'mapcar',2,2,0,[[Apply FUNCTION to each element of SEQUENCE, and make a list of the results.
The result is a list just as long as SEQUENCE.
SEQUENCE may be a list, a vector, a bool-vector, or a string.]]}
function F.mapcar.f(func,sequence)
    if lisp.chartablep(sequence) then
        error('TODO: err')
    end
    local leni=fixnum.tonumber(vars.F.length(sequence))
    local args={}
    local nmapped=mapcar1(leni,args,func,sequence)
    return vars.F.list(nmapped,args)
end
F.length={'length',1,1,0,[[Return the length of vector, list or string SEQUENCE.
A byte-code function object is also allowed.

If the string contains multibyte characters, this is not necessarily
the number of bytes in the string; it is the number of characters.
To get the number of bytes, use `string-bytes'.

If the length of a list is being computed to compare to a (small)
number, the `length<', `length>' and `length=' functions may be more
efficient.]]}
function F.length.f(sequence)
    local val
    if lisp.consp(sequence) then
        val=lisp.list_length(sequence)
    else
        error('TODO')
    end
    return fixnum.make(val)
end
F.put={'put',3,3,0,[[Store SYMBOL's PROPNAME property with value VALUE.
It can be retrieved with `(get SYMBOL PROPNAME)'.]]}
function F.put.f(sym,propname,value)
    lisp.check_symbol(sym)
    symbol.put(sym,propname,value)
    return value
end

local M={}
function M.init_syms()
    vars.setsubr(F,'assq')
    vars.setsubr(F,'member')
    vars.setsubr(F,'memq')
    vars.setsubr(F,'nthcdr')
    vars.setsubr(F,'nth')
    vars.setsubr(F,'mapcar')
    vars.setsubr(F,'length')
    vars.setsubr(F,'put')
end
return M
