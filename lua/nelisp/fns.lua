local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'
local cons=require'nelisp.obj.cons'
local fixnum=require'nelisp.obj.fixnum'
local symbol=require'nelisp.obj.symbol'
local hash_table=require'nelisp.obj.hash_table'
local signal=require'nelisp.signal'
local str=require'nelisp.obj.str'

local M={}

local F={}
F.assq={'assq',2,2,0,[[Return non-nil if KEY is `eq' to the car of an element of ALIST.
The value is actually the first element of ALIST whose car is KEY.
Elements of ALIST that are not conses are ignored.]]}
---@param key nelisp.obj
---@param alist nelisp.obj
---@return nelisp.obj
function F.assq.f(key,alist)
    local ret,tail=lisp.for_each_tail(alist,function (tail)
        if lisp.consp(lisp.xcar(tail)) and lisp.eq(lisp.xcar(lisp.xcar(tail) --[[@as nelisp.cons]]),key) then
            return lisp.xcar(tail)
        end
    end)
    if ret then return ret end
    lisp.check_list_end(tail,alist)
    return vars.Qnil
end
function M.assq_no_quit(key,alist)
    while not lisp.nilp(alist) do
        if lisp.consp(lisp.xcar(alist)) and lisp.eq(lisp.xcar(lisp.xcar(alist) --[[@as nelisp.cons]]),key) then
            return lisp.xcar(alist)
        end
        alist=lisp.xcdr(alist)
    end
    return vars.Qnil
end
F.member={'member',2,2,0,[[Return non-nil if ELT is an element of LIST.  Comparison done with `equal'.
The value is actually the tail of LIST whose car is ELT.]]}
function F.member.f(elt,list)
    if lisp.symbolp(elt) or lisp.fixnump(elt) then
        return vars.F.memq(elt,list)
    end
    local _,tail=lisp.for_each_tail(list,function (tail)
        if not lisp.nilp(vars.F.equal(elt,lisp.xcar(tail))) then
            return tail
        end
    end)
    lisp.check_list_end(tail,list)
    return vars.Qnil
end
F.memq={'memq',2,2,0,[[Return non-nil if ELT is an element of LIST.  Comparison done with `eq'.
The value is actually the tail of LIST whose car is ELT.]]}
function F.memq.f(elt,list)
    local ret,tail=lisp.for_each_tail(list,function (tail)
        if lisp.eq(lisp.xcar(tail),elt) then
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
                tail=lisp.xcdr(tail)
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
            local dummy=vars.F.funcall({fn,lisp.xcar(tail)})
            if vals then
                vals[i+1]=dummy
            end
            tail=lisp.xcdr(tail)
        end
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
        signal.wrong_type_argument(vars.Qlistp,sequence)
    end
    local leni=fixnum.tonumber(vars.F.length(sequence))
    local args={}
    local nmapped=mapcar1(leni,args,func,sequence)
    return vars.F.list({unpack(args,1,nmapped)})
end
F.nconc={'nconc',0,-2,0,[[Concatenate any number of lists by altering them.
Only the last argument is not altered, and need not be a list.
usage: (nconc &rest LISTS)]]}
function F.nconc.f(args)
    local val=vars.Qnil
    for k,v in ipairs(args) do
        local tem=v
        if lisp.nilp(tem) then
            goto continue
        elseif lisp.nilp(val) then
            val=tem
        end
        if k==#args then
            break
        end
        lisp.check_cons(tem)
        local tail
        _,tem=lisp.for_each_tail(tem,function(t)
            tail=t
        end)
        tem=args[k+1]
        vars.F.setcdr(tail,tem)
        if lisp.nilp(tem) then
            args[k+1]=tail
        end
        ::continue::
    end
    return val
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
    elseif lisp.nilp(sequence) then
        val=0
    else
        error('TODO')
    end
    return fixnum.make(val)
end
local function plist_put(plist,prop,val)
    local prev=vars.Qnil
    local tail=plist
    local has_visited={}
    while lisp.consp(tail) do
        if not lisp.consp(lisp.xcdr(tail)) then
            break
        end
        if lisp.eq(lisp.xcar(tail),prop) then
            vars.F.setcar(lisp.xcdr(tail),val)
            return plist
        end
        if has_visited[tail] then
            require'nelisp.signal'.xsignal(vars.Qcircular_list,plist)
        end
        prev=tail
        tail=lisp.xcdr(tail) --[[@as nelisp.cons]]
        has_visited[tail]=true
        tail=lisp.xcdr(tail)
    end
    lisp.check_type(lisp.nilp(tail),vars.Qplistp,plist)
    if lisp.nilp(prev) then
        return vars.F.cons(prop,vars.F.cons(val,plist))
    end
    local newcell=vars.F.cons(prop,vars.F.cons(val,lisp.xcdr(lisp.xcdr(prev --[[@as nelisp.cons]]) --[[@as nelisp.cons]])))
    vars.F.setcdr(lisp.xcdr(prev),newcell)
    return plist
end
F.put={'put',3,3,0,[[Store SYMBOL's PROPNAME property with value VALUE.
It can be retrieved with `(get SYMBOL PROPNAME)'.]]}
function F.put.f(sym,propname,value)
    lisp.check_symbol(sym)
    symbol.set_plist(sym,plist_put(symbol.get_plist(sym),propname,value))
    return value
end
function M.plist_get(plist,prop)
    local tail=plist
    local has_visited={}
    while lisp.consp(tail) do
        if not lisp.consp(lisp.xcdr(tail)) then
            break
        end
        if lisp.eq(lisp.xcar(tail),prop) then
            return lisp.xcar(lisp.xcdr(tail) --[[@as nelisp.cons]])
        end
        if has_visited[tail] then
            require'nelisp.signal'.xsignal(vars.Qcircular_list,plist)
        end
        has_visited[tail]=true
        tail=lisp.xcdr(tail) --[[@as nelisp.cons]]
        tail=lisp.xcdr(tail)
    end
    return vars.Qnil
end
F.get={'get',2,2,0,[[Return the value of SYMBOL's PROPNAME property.
This is the last value stored with `(put SYMBOL PROPNAME VALUE)'.]]}
function F.get.f(sym,propname)
    lisp.check_symbol(sym)
    local propval=M.plist_get(vars.F.cdr(vars.F.assq(symbol,vars.V.overriding_plist_environment)),propname)
    if not lisp.nilp(propval) then
        return propval
    end
    return M.plist_get(symbol.get_plist(sym),propname)
end
F.featurep={'featurep',1,2,0,[[Return t if FEATURE is present in this Emacs.

Use this to conditionalize execution of lisp code based on the
presence or absence of Emacs or environment extensions.
Use `provide' to declare that a feature is available.  This function
looks at the value of the variable `features'.  The optional argument
SUBFEATURE can be used to check a specific subfeature of FEATURE.]]}
function F.featurep.f(feature,subfeature)
    lisp.check_symbol(feature)
    local tem=vars.F.memq(feature,vars.V.features)
    if not lisp.nilp(tem) and not lisp.nilp(subfeature) then
        error('TODO')
    end
    return lisp.nilp(tem) and vars.Qnil or vars.Qt
end
F.provide={'provide',1,2,0,[[Announce that FEATURE is a feature of the current Emacs.
The optional argument SUBFEATURES should be a list of symbols listing
particular subfeatures supported in this version of FEATURE.]]}
function F.provide.f(feature,subfeatures)
    lisp.check_symbol(feature)
    lisp.check_list(subfeatures)
    if not lisp.nilp(vars.V.autoload_queue) then
        error('TODO')
    end
    local tem=vars.F.memq(feature,vars.V.features)
    if lisp.nilp(tem) then
        vars.V.features=vars.F.cons(feature,vars.V.features)
    end
    if not lisp.nilp(subfeatures) then
        error('TODO')
    end
    lisp.loadhist_attach(vars.F.cons(vars.Qprovide,feature))
    tem=vars.F.assq(feature,vars.V.after_load_alist)
    if lisp.consp(tem) then
        error('TODO')
    end
    return feature
end
F.make_hash_table={'make-hash-table',0,-2,0,[[Create and return a new hash table.

Arguments are specified as keyword/argument pairs.  The following
arguments are defined:

:test TEST -- TEST must be a symbol that specifies how to compare
keys.  Default is `eql'.  Predefined are the tests `eq', `eql', and
`equal'.  User-supplied test and hash functions can be specified via
`define-hash-table-test'.

:size SIZE -- A hint as to how many elements will be put in the table.
Default is 65.

:rehash-size REHASH-SIZE - Indicates how to expand the table when it
fills up.  If REHASH-SIZE is an integer, increase the size by that
amount.  If it is a float, it must be > 1.0, and the new size is the
old size multiplied by that factor.  Default is 1.5.

:rehash-threshold THRESHOLD -- THRESHOLD must a float > 0, and <= 1.0.
Resize the hash table when the ratio (table entries / table size)
exceeds an approximation to THRESHOLD.  Default is 0.8125.

:weakness WEAK -- WEAK must be one of nil, t, `key', `value',
`key-or-value', or `key-and-value'.  If WEAK is not nil, the table
returned is a weak table.  Key/value pairs are removed from a weak
hash table when there are no non-weak references pointing to their
key, value, one of key or value, or both key and value, depending on
WEAK.  WEAK t is equivalent to `key-and-value'.  Default value of WEAK
is nil.

:purecopy PURECOPY -- If PURECOPY is non-nil, the table can be copied
to pure storage when Emacs is being dumped, making the contents of the
table read only. Any further changes to purified tables will result
in an error.

usage: (make-hash-table &rest KEYWORD-ARGS)]]}
function F.make_hash_table.f(args)
    if not _G.nelisp_later then
        error('TODO')
    end
    return hash_table.make(nil,nil)
end
F.puthash={'puthash',3,3,0,[[Associate KEY with VALUE in hash table TABLE.
If KEY is already present in table, replace its current value with
VALUE.  In any case, return VALUE.]]}
function F.puthash.f(key,value,t)
    if not _G.nelisp_later then
        error('TODO')
    end
    hash_table.put(t,key,value)
    return value
end

F.delq={'delq',2,2,0,[[Delete members of LIST which are `eq' to ELT, and return the result.
More precisely, this function skips any members `eq' to ELT at the
front of LIST, then removes members `eq' to ELT from the remaining
sublist by modifying its list structure, then returns the resulting
list.

Write `(setq foo (delq element foo))' to be sure of correctly changing
the value of a list `foo'.  See also `remq', which does not modify the
argument.]]}
function F.delq.f(elt,list)
    local prev=vars.Qnil
    local _,tail=lisp.for_each_tail(list,function (tail)
        local tem=lisp.xcar(tail)
        if lisp.eq(tem,elt) then
            if lisp.nilp(prev) then
                list=lisp.xcdr(tail)
            else
                vars.F.setcdr(prev,lisp.xcdr(tail))
            end
        else
            prev=tail
        end
    end)
    lisp.check_list_end(tail,list)
    return list
end
F.concat={'concat',0,-2,0,[[Concatenate all the arguments and make the result a string.
The result is a string whose elements are the elements of all the arguments.
Each argument may be a string or a list or vector of characters (integers).

Values of the `composition' property of the result are not guaranteed
to be `eq'.
usage: (concat &rest SEQUENCES)]]}
function F.concat.f(args)
    local dest_multibyte=false
    for _,arg in ipairs(args) do
        if lisp.stringp(arg) then
            if str.is_multibyte(arg) then
                dest_multibyte=true
            end
        end
    end
    local strs={}
    for _,arg in ipairs(args) do
        if lisp.stringp(arg) then
            if str.get_intervals(arg) then
                error('TODO')
            end
            if str.is_multibyte(arg)==dest_multibyte then
                table.insert(strs,lisp.sdata(arg))
            else
                error('TODO')
            end
        elseif lisp.vectorp(arg) then
            error('TODO')
        elseif lisp.nilp(arg) then
        elseif lisp.consp(arg) then
            error('TODO')
        else
            signal.wrong_type_argument(vars.Qsequencep,arg)
        end
    end
    local out_str=table.concat(strs)
    return str.make(out_str,dest_multibyte)
end

function M.init()
    vars.V.features=cons.make(vars.Qemacs,vars.Qnil)
end
function M.init_syms()
    vars.setsubr(F,'assq')
    vars.setsubr(F,'member')
    vars.setsubr(F,'memq')
    vars.setsubr(F,'nthcdr')
    vars.setsubr(F,'nth')
    vars.setsubr(F,'mapcar')
    vars.setsubr(F,'nconc')
    vars.setsubr(F,'length')
    vars.setsubr(F,'put')
    vars.setsubr(F,'get')
    vars.setsubr(F,'featurep')
    vars.setsubr(F,'provide')
    vars.setsubr(F,'make_hash_table')
    vars.setsubr(F,'puthash')
    vars.setsubr(F,'delq')
    vars.setsubr(F,'concat')

    vars.defvar_lisp('features','features',[[A list of symbols which are the features of the executing Emacs.
Used by `featurep' and `require', and altered by `provide'.]])

  vars.defvar_lisp('overriding_plist_environment','overriding-plist-environment',[[An alist that overrides the plists of the symbols which it lists.
Used by the byte-compiler to apply `define-symbol-prop' during
compilation.]])
  vars.V.overriding_plist_environment=vars.Qnil

    vars.defsym('Qplistp','plistp')
    vars.defsym('Qprovide','provide')
end
return M
