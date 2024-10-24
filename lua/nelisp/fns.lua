local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'
local signal=require'nelisp.signal'
local print_=require'nelisp.print'
local textprop=require'nelisp.textprop'
local alloc=require'nelisp.alloc'
local overflow=require'nelisp.overflow'
local chartab=require'nelisp.chartab'
local chars=require'nelisp.chars'

local M={}

---@param obj nelisp.obj
---@return number
local function sxhash(obj)
    error('TODO')
end
function M.concat_to_string(args)
    local dest_multibyte=false
    local some_multibyte=false
    for _,arg in ipairs(args) do
        if lisp.stringp(arg) then
            if lisp.string_multibyte(arg) then
                dest_multibyte=true
            else
                some_multibyte=true
            end
        else
            error('TODO')
        end
    end
    if dest_multibyte and some_multibyte then
        error('TODO')
    end
    local buf=print_.make_printcharfun()
    for _,arg in ipairs(args) do
        if lisp.stringp(arg) then
            if lisp.string_intervals(arg) then
                error('TODO')
            end
            if lisp.string_multibyte(arg)==dest_multibyte then
                buf.write(lisp.sdata(arg))
            else
                error('TODO')
            end
        else
            error('TODO')
        end
    end
    return dest_multibyte and error('TODO') or alloc.make_unibyte_string(buf.out())
end

local F={}
local function concat_to_list(args)
    local nargs=#args-1
    local last_tail=args[#args]
    local result=vars.Qnil
    local last=vars.Qnil
    for i=1,nargs do
        local arg=args[i]
        if lisp.consp(arg) then
            local head=vars.F.cons(lisp.xcar(arg),vars.Qnil)
            local prev=head
            arg=lisp.xcdr(arg)
            local _,_end=lisp.for_each_tail(arg,function (a)
                local next_=vars.F.cons(lisp.xcar(a),vars.Qnil)
                lisp.xsetcdr(prev,next_)
                prev=next_
            end)
            lisp.check_list_end(_end,arg)
            if lisp.nilp(result) then
                result=head
            else
                lisp.xsetcdr(last,head)
            end
            last=prev
        else
            error('TODO')
        end
    end
    if result==vars.Qnil then
        result=last_tail
    else
        lisp.xsetcdr(last,last_tail)
    end
    return result
end
F.append={'append',0,-2,0,[[Concatenate all the arguments and make the result a list.
The result is a list whose elements are the elements of all the arguments.
Each argument may be a list, vector or string.

All arguments except the last argument are copied.  The last argument
is just used as the tail of the new list.

usage: (append &rest SEQUENCES)]]}
function F.append.f(args)
    if #args==0 then
        return vars.Qnil
    end
    return concat_to_list(args)
end
F.assq={'assq',2,2,0,[[Return non-nil if KEY is `eq' to the car of an element of ALIST.
The value is actually the first element of ALIST whose car is KEY.
Elements of ALIST that are not conses are ignored.]]}
---@param key nelisp.obj
---@param alist nelisp.obj
---@return nelisp.obj
function F.assq.f(key,alist)
    local ret,tail=lisp.for_each_tail(alist,function (tail)
        if lisp.consp(lisp.xcar(tail)) and lisp.eq(lisp.xcar(lisp.xcar(tail)),key) then
            return lisp.xcar(tail)
        end
    end)
    if ret then return ret end
    lisp.check_list_end(tail,alist)
    return vars.Qnil
end
function M.assq_no_quit(key,alist)
    while not lisp.nilp(alist) do
        if lisp.consp(lisp.xcar(alist)) and lisp.eq(lisp.xcar(lisp.xcar(alist)),key) then
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
    local ret,tail=lisp.for_each_tail(list,function (tail)
        if not lisp.nilp(vars.F.equal(elt,lisp.xcar(tail))) then
            return tail
        end
    end)
    if ret then return ret end
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
        num=lisp.fixnum(n)
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
    local leni=lisp.fixnum(vars.F.length(sequence))
    local args={}
    local nmapped=mapcar1(leni,args,func,sequence)
    return vars.F.list({unpack(args,1,nmapped)})
end
F.nreverse={'nreverse',1,1,0,[[Reverse order of items in a list, vector or string SEQ.
If SEQ is a list, it should be nil-terminated.
This function may destructively modify SEQ to produce the value.]]}
function F.nreverse.f(seq)
    if lisp.nilp(seq) then
        return seq
    elseif lisp.consp(seq) then
        local prev=vars.Qnil
        local tail=seq
        while lisp.consp(tail) do
            local next_=lisp.xcdr(tail)
            if next_==seq then
                require'nelisp.signal'.xsignal(vars.Qcircular_list,seq)
            end
            vars.F.setcdr(tail,prev)
            prev=tail
            tail=next_
        end
        lisp.check_list_end(tail,seq)
        return prev
    else
        error('TODO')
    end
end
F.reverse={'reverse',1,1,0,[[Return the reversed copy of list, vector, or string SEQ.
See also the function `nreverse', which is used more often.]]}
function F.reverse.f(seq)
    if lisp.nilp(seq) then
        return vars.Qnil
    elseif lisp.consp(seq) then
        local new=vars.Qnil
        local _,t=lisp.for_each_tail(seq,function (t)
            new=vars.F.cons(lisp.xcar(t),new)
        end)
        lisp.check_list_end(t,seq)
        return new
    else
        error('TODO')
    end
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
    elseif lisp.stringp(sequence) then
        val=lisp.schars(sequence)
    else
        error('TODO')
        signal.wrong_type_argument(vars.Qsequencep,sequence)
    end
    return lisp.make_fixnum(val)
end
---@param h nelisp._hash_table
---@param idx number
---@return number
local function hash_index(h,idx)
    return lisp.fixnum(lisp.aref(h.index,idx))
end
---@param h nelisp._hash_table
---@param key nelisp.obj
---@return number
---@return number
function M.hash_lookup(h,key)
    local hash_code=h.test.hashfn(key,h)
    assert(type(hash_code)=='number')
    local i=hash_index(h,hash_code%lisp.asize(h.index))
    while 0<=i do
        if lisp.eq(key,lisp.aref(h.key_and_value,i*2))
            or (h.test.cmpfn~=0
            and lisp.eq(lisp.make_fixnum(hash_code),lisp.aref(h.hash,i))
            and h.test.cmpfn(key,lisp.aref(h.key_and_value,i*2),h)) then
            break
        end
        i=lisp.fixnum(lisp.aref(h.next,i))
    end
    return i,hash_code
end
---@param h nelisp._hash_table
local function maybe_resize_hash_table(h)
    if h.next_free<0 then
        error('TODO')
    end
end
---@param h nelisp._hash_table
---@param key nelisp.obj
---@param val nelisp.obj
---@param hash number
---@return number
function M.hash_put(h,key,val,hash)
    maybe_resize_hash_table(h)
    h.count=h.count+1
    local i=h.next_free
    assert(lisp.nilp(lisp.aref(h.hash,i)))
    assert(lisp.aref(h.key_and_value,i*2)==vars.Qunique)
    h.next_free=lisp.fixnum(lisp.aref(h.next,i))
    lisp.aset(h.key_and_value,i*2,key)
    lisp.aset(h.key_and_value,i*2+1,val)
    lisp.aset(h.hash,i,val)
    local start_of_bucket=hash%lisp.asize(h.index)
    lisp.aset(h.next,i,lisp.aref(h.index,start_of_bucket))
    lisp.aset(h.index,start_of_bucket,lisp.make_fixnum(i))
    return i
end
---@param a nelisp.obj
---@param b nelisp.obj
---@param kind 'plain'|'no_quit'|'including_properties'
---@param depth number
---@param ht table
local function internal_equal(a,b,kind,depth,ht)
    ::tail_recursion::
    if depth>10 then
        assert(kind~='no_quit')
        if depth>200 then
            signal.error('Stack overflow in equal')
        end
        local t=lisp.xtype(a)
        if t==lisp.type.cons or t==lisp.type.vectorlike then
            local val=ht[a]
            if val then
                if not lisp.nilp(vars.F.memq(b,val)) then
                    return true
                else
                    ht[a]=vars.F.cons(b,ht[a])
                end
            else
                ht[a]=vars.F.cons(b,vars.Qnil)
            end
        end
    end
    if lisp.symbolwithposp(a) then
        error('TODO')
    end
    if lisp.symbolwithposp(b) then
        error('TODO')
    end
    if a==b then
        return true
    elseif lisp.xtype(a)~=lisp.xtype(b) then
        return false
    end
    local t=lisp.xtype(a)
    if t==lisp.type.float then
        error('TODO')
    elseif t==lisp.type.cons then
        if kind=='no_quit' then
            error('TODO')
        else
            local o2=b
            local ret=lisp.for_each_tail(a,function (o1)
                if not lisp.consp(o2) then
                    return false
                end
                if not internal_equal(lisp.xcar(o1),lisp.xcar(o2),kind,depth+1,ht) then
                    return false
                end
                o2=lisp.xcdr(o2)
                if lisp.eq(lisp.xcdr(o1),o2) then
                    return true
                end
            end)
            if ret then return ret end
            depth=depth+1
            goto tail_recursion
        end
    elseif t==lisp.type.vectorlike then
        error('TODO')
    elseif t==lisp.type.string then
        return lisp.schars(a)==lisp.schars(b)
            and lisp.sdata(a)==lisp.sdata(b)
            and (kind~='including_properties' or error('TODO'))
    end
    return false
end
F.equal={'equal',2,2,0,[[Return t if two Lisp objects have similar structure and contents.
They must have the same data type.
Conses are compared by comparing the cars and the cdrs.
Vectors and strings are compared element by element.
Numbers are compared via `eql', so integers do not equal floats.
\(Use `=' if you want integers and floats to be able to be equal.)
Symbols must match exactly.]]}
function F.equal.f(a,b)
    return internal_equal(a,b,'plain',0,{}) and vars.Qt or vars.Qnil
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
        tail=lisp.xcdr(tail)
        has_visited[tail]=true
        tail=lisp.xcdr(tail)
    end
    lisp.check_type(lisp.nilp(tail),vars.Qplistp,plist)
    if lisp.nilp(prev) then
        return vars.F.cons(prop,vars.F.cons(val,plist))
    end
    local newcell=vars.F.cons(prop,vars.F.cons(val,lisp.xcdr(lisp.xcdr(prev))))
    vars.F.setcdr(lisp.xcdr(prev),newcell)
    return plist
end
F.put={'put',3,3,0,[[Store SYMBOL's PROPNAME property with value VALUE.
It can be retrieved with `(get SYMBOL PROPNAME)'.]]}
function F.put.f(sym,propname,value)
    lisp.check_symbol(sym)
    lisp.set_symbol_plist(
        sym,plist_put((sym --[[@as nelisp._symbol]]).plist,propname,value))
    return value
end
F.plist_put={'plist-put',3,4,0,[[Change value in PLIST of PROP to VAL.
PLIST is a property list, which is a list of the form
\(PROP1 VALUE1 PROP2 VALUE2 ...).

The comparison with PROP is done using PREDICATE, which defaults to `eq'.

If PROP is already a property on the list, its value is set to VAL,
otherwise the new PROP VAL pair is added.  The new plist is returned;
use `(setq x (plist-put x prop val))' to be sure to use the new value.
The PLIST is modified by side effects.]]}
function F.plist_put.f(plist,prop,val,predicate)
    if lisp.nilp(predicate) then
        return plist_put(plist,prop,val)
    end
    error('TODO')
end
function M.plist_get(plist,prop)
    local tail=plist
    local has_visited={}
    while lisp.consp(tail) do
        if not lisp.consp(lisp.xcdr(tail)) then
            break
        end
        if lisp.eq(lisp.xcar(tail),prop) then
            return lisp.xcar(lisp.xcdr(tail))
        end
        if has_visited[tail] then
            require'nelisp.signal'.xsignal(vars.Qcircular_list,plist)
        end
        has_visited[tail]=true
        tail=lisp.xcdr(tail)
        tail=lisp.xcdr(tail)
    end
    return vars.Qnil
end
F.get={'get',2,2,0,[[Return the value of SYMBOL's PROPNAME property.
This is the last value stored with `(put SYMBOL PROPNAME VALUE)'.]]}
function F.get.f(sym,propname)
    lisp.check_symbol(sym)
    local propval=M.plist_get(vars.F.cdr(vars.F.assq(sym,vars.V.overriding_plist_environment)),propname)
    if not lisp.nilp(propval) then
        return propval
    end
    return M.plist_get((sym --[[@as nelisp._symbol]]).plist,propname)
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
    if not lisp.nilp(vars.autoload_queue) then
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
---@param rehash_threshold number (float)
---@param size number
---@return number
local function hash_index_size(rehash_threshold,size)
    local n=math.floor(size/rehash_threshold)
    n=n-n%2
    while true do
        if n>overflow.max then
            signal.error('Hash table too large')
        end
        if n%3~=0 and n%5~=0 and n%7~=0 then
            return n
        end
        n=n+2
    end
end
---@param test nelisp.hash_table_test
---@param size number
---@param rehash_size number (float)
---@param rehash_threshold number (float)
---@param weak nelisp.obj
local function make_hash_table(test,size,rehash_size,rehash_threshold,weak)
    ---@type nelisp._hash_table
    local h={
        test=test,
        weak=weak,
        rehash_threshold=rehash_threshold,
        rehash_size=rehash_size,
        count=0,
        key_and_value=alloc.make_vector(size*2,vars.Qunique),
        hash=alloc.make_vector(size,'nil'),
        next=alloc.make_vector(size,lisp.make_fixnum(-1)),
        index=alloc.make_vector(hash_index_size(rehash_threshold,size),lisp.make_fixnum(-1)),
        mutable=true,
        next_free=0
    }
    for i=0,size-2 do
        lisp.aset(h.next,i,lisp.make_fixnum(i+1))
    end
    return lisp.make_vectorlike_ptr(h,lisp.pvec.hash_table)
end
---@param key nelisp.obj
---@param args nelisp.obj[]
---@param used table<number,true?>
---@return nelisp.obj|false
local function get_key_arg(key,args,used)
    for i=2,#args do
        if not used[i-1] and lisp.eq(args[i-1],key) then
            used[i-1]=true
            used[i]=true
            return args[i]
        end
    end
    return false
end
local function hashfn_eq(key,_)
    assert(not lisp.symbolwithposp(key),'TODO')
    -- Do we need ...^XTYPE(key)?
    return lisp.xhash(key)
end
local function cmpfn_equal(key1,key2,_)
    return not lisp.nilp(vars.F.equal(key1,key2))
end
local function hashfn_equal(key,_)
    return sxhash(key)
end
local function cmpfn_eql(key1,key2,_)
    return not lisp.nilp(vars.F.eql(key1,key2))
end
local function hashfn_eql(key,h)
    return ((lisp.floatp(key) or lisp.bignump(key)) and hashfn_equal or hashfn_eq)(key,h)
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
    local used={}
    local test=get_key_arg(vars.QCtest,args,used) or vars.Qeql
    local testdesc
    if lisp.eq(test,vars.Qeq) then
        ---@type nelisp.hash_table_test
        testdesc={
            name=vars.Qeq,
            user_cmp_function=vars.Qnil,
            user_hash_function=vars.Qnil,
            cmpfn=0,
            hashfn=hashfn_eq
        }
    elseif lisp.eq(test,vars.Qeql) then
        ---@type nelisp.hash_table_test
        testdesc={
            name=vars.Qeql,
            user_cmp_function=vars.Qnil,
            user_hash_function=vars.Qnil,
            cmpfn=cmpfn_eql,
            hashfn=hashfn_eql
        }
    elseif lisp.eq(test,vars.Qequal) then
        ---@type nelisp.hash_table_test
        testdesc={
            name=vars.Qequal,
            user_cmp_function=vars.Qnil,
            user_hash_function=vars.Qnil,
            cmpfn=cmpfn_equal,
            hashfn=hashfn_equal
        }
    else
        error('TODO')
    end
    local _=get_key_arg(vars.QCpurecopy,args,used)
    local size_arg=get_key_arg(vars.QCsize,args,used) or vars.Qnil
    local size
    if lisp.nilp(size_arg) then
        size=65
    elseif lisp.fixnatp(size_arg) then
        size=lisp.fixnum(size_arg)
    else
        signal.signal_error('Invalid hash table size',size_arg)
    end
    local rehash_size_arg=get_key_arg(vars.QCrehash_size,args,used)
    local rehash_size
    if rehash_size_arg==false then
        rehash_size=1.5-1
    else
        error('TODO')
    end
    local rehash_threshold_arg=get_key_arg(vars.QCrehash_threshold,args,used)
    local rehash_threshold=rehash_threshold_arg==false and 0.8125 or error('TODO')
    if not (0<rehash_threshold and rehash_threshold<=1) then
        signal.signal_error("Invalid hash table rehash threshold",rehash_threshold_arg)
    end
    local weak=get_key_arg(vars.QCweakness,args,used) or vars.Qnil
    if lisp.eq(weak,vars.Qt) then
        weak=vars.Qkey_and_value
    end
    if not lisp.nilp(weak)
        and not lisp.eq(weak,vars.Qkey)
        and not lisp.eq(weak,vars.Qvalue)
        and not lisp.eq(weak,vars.Qkey_or_value)
        and not lisp.eq(weak,vars.Qkey_and_value) then
        lisp.signal_error('Invalid hash table weakness',weak)
    end
    for i=1,#args-1 do
        if not used[i] then
            lisp.signal_error('Invalid hash table weakness',args[i])
        end
    end
    return make_hash_table(testdesc,size,rehash_size,rehash_threshold,weak)
end
---@param obj nelisp.obj
---@return nelisp._hash_table
local function check_hash_table(obj)
    lisp.check_hash_table(obj)
    return obj --[[@as nelisp._hash_table]]
end
---@param obj nelisp.obj
---@param h nelisp._hash_table
local function check_mutable_hash_table(obj,h)
    if not h.mutable then
        signal.signal_error('hash table test modifies table',obj)
    end
end
F.puthash={'puthash',3,3,0,[[Associate KEY with VALUE in hash table TABLE.
If KEY is already present in table, replace its current value with
VALUE.  In any case, return VALUE.]]}
function F.puthash.f(key,value,t)
    local h=check_hash_table(t)
    check_mutable_hash_table(t,h)
    local i,hash=M.hash_lookup(h,key)
    if i>=0 then
        lisp.aset(h.key_and_value,2*i+1,value)
    else
        M.hash_put(h,key,value,hash)
    end
    return value
end
F.gethash={'gethash',2,3,0,[[Look up KEY in TABLE and return its associated value.
If KEY is not found, return DFLT which defaults to nil.]]}
function F.gethash.f(key,table,dflt)
    local h=check_hash_table(table)
    local i=M.hash_lookup(h,key)
    return i>=0 and lisp.aref(h.key_and_value,2*i+1) or dflt
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
            if lisp.string_multibyte(arg) then
                dest_multibyte=true
            end
        end
    end
    local strs={}
    for _,arg in ipairs(args) do
        if lisp.stringp(arg) then
            if lisp.string_intervals(arg) then
                error('TODO')
            end
            if lisp.string_multibyte(arg)==dest_multibyte then
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
    return (dest_multibyte and error('TODO') or alloc.make_unibyte_string)(out_str)
end
F.copy_sequence={'copy-sequence',1,1,0,[[Return a copy of a list, vector, string, char-table or record.
The elements of a list, vector or record are not copied; they are
shared with the original.  See Info node `(elisp) Sequence Functions'
for more details about this sharing and its effects.
If the original sequence is empty, this function may return
the same empty object instead of its copy.]]}
function F.copy_sequence.f(arg)
    if lisp.nilp(arg) then
        return arg
    elseif lisp.consp(arg) then
        local val=vars.F.cons(vars.F.car(arg),vars.Qnil)
        local prev=val
        local _,tail=lisp.for_each_tail(lisp.xcdr(arg),function (tail)
            local c=vars.F.cons(lisp.xcar(tail),vars.Qnil)
            lisp.xsetcdr(prev,c)
            prev=c
        end)
        lisp.check_list_end(tail,tail)
        return val
    elseif lisp.chartablep(arg) then
        return chartab.copy_char_table(arg)
    else
        error('TODO')
        signal.wrong_type_argument(vars.Qsequencep,arg)
    end
end
local function validate_subarray(array,from,to,size)
    local f,t
    if lisp.fixnump(from) then
        f=lisp.fixnum(from)
        if f<0 then
            f=size+f
        end
    elseif lisp.nilp(from) then
        f=0
    else
        signal.wrong_type_argument(vars.Qintegerp,from)
    end
    if lisp.fixnump(to) then
        t=lisp.fixnum(to)
        if t<0 then
            t=size+t
        end
    elseif lisp.nilp(to) then
        t=size
    else
        signal.wrong_type_argument(vars.Qintegerp,to)
    end
    if 0<=f and f<=t and t<=size then
        return f,t
    end
    signal.args_out_of_range(array,from,to)
    error('unreachable')
end
local function string_char_to_byte(s,idx)
    local best_above=lisp.schars(s)
    local best_above_byte=lisp.sbytes(s)
    if best_above==best_above_byte then
        return idx
    end
    error('TODO')
end
F.string_to_multibyte={'string-to-multibyte',1,1,0,[[Return a multibyte string with the same individual chars as STRING.
If STRING is multibyte, the result is STRING itself.
Otherwise it is a newly created string, with no text properties.

If STRING is unibyte and contains an 8-bit byte, it is converted to
the corresponding multibyte character of charset `eight-bit'.

This differs from `string-as-multibyte' by converting each byte of a correct
utf-8 sequence to an eight-bit character, not just bytes that don't form a
correct sequence.]]}
function F.string_to_multibyte.f(s)
    lisp.check_string(s)
    if lisp.string_multibyte(s) then
        return s
    end
    local nchars=lisp.schars(s)
    local data=lisp.sdata(s)
    if data:find('[\x80-\xff]') then
        data=chars.str_to_multibyte(data)
    end
    return alloc.make_multibyte_string(data,nchars)
end
F.substring={'substring',1,3,0,[[Return a new string whose contents are a substring of STRING.
The returned string consists of the characters between index FROM
\(inclusive) and index TO (exclusive) of STRING.  FROM and TO are
zero-indexed: 0 means the first character of STRING.  Negative values
are counted from the end of STRING.  If TO is nil, the substring runs
to the end of STRING.

The STRING argument may also be a vector.  In that case, the return
value is a new vector that contains the elements between index FROM
\(inclusive) and index TO (exclusive) of that vector argument.

With one argument, just copy STRING (with properties, if any).]]}
function F.substring.f(s,from,to)
    local size=lisp.check_vector_or_string(s)
    local f,t=validate_subarray(s,from,to,size)
    local res
    if lisp.stringp(s) then
        local from_byte=f~=0 and string_char_to_byte(s,f) or 0
        local to_byte=t==size and lisp.sbytes(s) or string_char_to_byte(s,t)
        res=alloc.make_specified_string(lisp.sdata(s):sub(from_byte+1,to_byte),to_byte-from_byte,lisp.string_multibyte(s))
        textprop.copy_textprop(lisp.make_fixnum(f),lisp.make_fixnum(t),s,lisp.make_fixnum(0),res,vars.Qnil)
    else
        error('TODO')
    end
    return res
end
F.string_equal={'string-equal',2,2,0,[[Return t if two strings have identical contents.
Case is significant, but text properties are ignored.
Symbols are also allowed; their print names are used instead.

See also `string-equal-ignore-case'.]]}
function F.string_equal.f(a,b)
    if lisp.symbolp(a) then
        a=lisp.symbol_name(a)
    end
    if lisp.symbolp(b) then
        b=lisp.symbol_name(b)
    end
    lisp.check_string(a)
    lisp.check_string(b)
    return lisp.sdata(a)==lisp.sdata(b) and vars.Qt or vars.Qnil
end
F.require={'require',1,3,0,[[If FEATURE is not already loaded, load it from FILENAME.
If FEATURE is not a member of the list `features', then the feature was
not yet loaded; so load it from file FILENAME.

If FILENAME is omitted, the printname of FEATURE is used as the file
name, and `load' is called to try to load the file by that name, after
appending the suffix `.elc', `.el', or the system-dependent suffix for
dynamic module files, in that order; but the function will not try to
load the file without any suffix.  See `get-load-suffixes' for the
complete list of suffixes.

To find the file, this function searches the directories in `load-path'.

If the optional third argument NOERROR is non-nil, then, if
the file is not found, the function returns nil instead of signaling
an error.  Normally the return value is FEATURE.

The normal messages issued by `load' at start and end of loading
FILENAME are suppressed.]]}
function F.require.f(feature,filename,noerror)
    lisp.check_symbol(feature)
    local from_file=not lisp.nilp(vars.V.load_in_progress)
    if not from_file then
        if not _G.nelisp_later then
            error('TODO')
        end
    end
    local tem
    if from_file then
        tem=vars.F.cons(vars.Qrequire,feature)
        if lisp.nilp(vars.F.member(tem,vars.V.current_load_list)) then
            lisp.loadhist_attach(tem)
        end
    end
    tem=vars.F.memq(feature,vars.V.features)
    if lisp.nilp(tem) then
        error('TODO')
    end
    return feature
end

function M.init()
    vars.V.features=lisp.list(vars.Qemacs)
    vars.F.make_var_non_special(vars.Qfeatures)
end
function M.init_syms()
    vars.defsubr(F,'append')
    vars.defsubr(F,'assq')
    vars.defsubr(F,'member')
    vars.defsubr(F,'memq')
    vars.defsubr(F,'nthcdr')
    vars.defsubr(F,'nth')
    vars.defsubr(F,'mapcar')
    vars.defsubr(F,'nreverse')
    vars.defsubr(F,'reverse')
    vars.defsubr(F,'nconc')
    vars.defsubr(F,'length')
    vars.defsubr(F,'equal')
    vars.defsubr(F,'put')
    vars.defsubr(F,'plist_put')
    vars.defsubr(F,'get')
    vars.defsubr(F,'featurep')
    vars.defsubr(F,'provide')
    vars.defsubr(F,'make_hash_table')
    vars.defsubr(F,'puthash')
    vars.defsubr(F,'gethash')
    vars.defsubr(F,'delq')
    vars.defsubr(F,'concat')
    vars.defsubr(F,'copy_sequence')
    vars.defsubr(F,'string_to_multibyte')
    vars.defsubr(F,'substring')
    vars.defsubr(F,'string_equal')
    vars.defsubr(F,'require')

    vars.defvar_lisp('features','features',[[A list of symbols which are the features of the executing Emacs.
Used by `featurep' and `require', and altered by `provide'.]])
    vars.defsym('Qfeatures','features')

    vars.defvar_lisp('overriding_plist_environment','overriding-plist-environment',[[An alist that overrides the plists of the symbols which it lists.
Used by the byte-compiler to apply `define-symbol-prop' during
compilation.]])
    vars.V.overriding_plist_environment=vars.Qnil

    vars.defsym('Qhash_table_p','hash-table-p')
    vars.defsym('Qplistp','plistp')
    vars.defsym('Qprovide','provide')
    vars.defsym('Qrequire','require')
    vars.defsym('Qeq','eq')
    vars.defsym('Qeql','eql')
    vars.defsym('Qequal','equal')

    vars.defsym('Qkey','key')
    vars.defsym('Qvalue','value')
    vars.defsym('Qkey_or_value','key-or-value')
    vars.defsym('Qkey_and_value','key-and-value')
end
return M
