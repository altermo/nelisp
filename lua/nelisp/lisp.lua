if _G.nelisp_later then
    --Avoid `_G.nelisp_later` being undefined
    _G.nelisp_later=_G.nelisp_later
    error('TODO: remove once nelisp_later is removed')
end
local vars=require'nelisp.vars'
local b=require'nelisp.bytes'
local M={}

--- ;; Types
---@class nelisp.obj
---@field [1] nelisp.type

---@class nelisp.ptr
---@field [1] nelisp.obj

---@param a nelisp.obj
---@return nelisp.type
function M.xtype(a)
    return a[1]
end

---@enum nelisp.type
M.type={
    symbol=0,
    ---@class nelisp._symbol
    ---@field name nelisp.obj
    ---@field plist nelisp.obj
    ---@field redirect nelisp.symbol_redirect
    ---@field value nelisp.obj|nelisp.buffer_local_value|nelisp._symbol|nelisp.forward?
    ---@field fn nelisp.obj
    ---@field interned nelisp.symbol_interned
    ---@field trapped_write nelisp.symbol_trapped_write
    ---@field declared_special boolean?
    ---@field next nelisp.obj?
    ---@class nelisp.forward.context
    ---@field buffer nelisp._buffer?
    ---@alias nelisp.forward.getfn fun(context:nelisp.forward.context):nelisp.obj
    ---@alias nelisp.forward.setfn fun(v:nelisp.obj,context:nelisp.forward.context)
    ---@class nelisp.forward
    ---@field [1] nelisp.forward.getfn
    ---@field [2] nelisp.forward.setfn
    ---@field isbuffer boolean

    int0=2,
    ---@class nelisp._fixnum
    ---@field [2] number

    string=4,
    ---@class nelisp._string
    ---@field size_chars number|nil
    ---@field [2] string data
    ---@field intervals nelisp.intervals?

    vectorlike=5,
    ---@class nelisp._vectorlike
    ---@field header nelisp.pvec
    ---@class nelisp._pvec
    ---@class nelisp._pvec_special: nelisp._pvec

    cons=3,
    ---@class nelisp._cons
    ---@field [1] nelisp.type.cons
    ---@field [2] nelisp.obj car
    ---@field [3] nelisp.obj cdr

    float=7,
    ---@class nelisp._float
    ---@field [2] number
}
--- ;;; Types pseudovector (vectorlike)
function M.pseudovector_type(a)
    return (a --[[@as nelisp._vectorlike]]).header
end
---@enum nelisp.pvec
M.pvec={
    normal_vector=0,
    ---@class nelisp._normal_vector: nelisp._pvec
    ---@field size number
    ---@field contents table<number,nelisp.obj|nil> (1-indexed)

    _free=1,
    bignum=2,
    marker=3,
    ---@class nelisp._marker: nelisp._pvec_special

    overlay=4,
    ---@class nelisp._overlay: nelisp._pvec_special

    finalizer=5,
    symbol_with_pos=6,
    _misc_ptr=7,
    user_ptr=8,
    process=9,
    frame=10,
    ---@class nelisp._frame: nelisp._pvec_special

    window=11,
    bool_vector=12,
    ---@class nelisp._bool_vector: nelisp._pvec
    ---@field contents boolean[] (1-indexed)

    buffer=13,
    ---@class nelisp._buffer: nelisp._pvec_special

    hash_table=14,
    ---@class nelisp._hash_table: nelisp._pvec
    ---@field weak nelisp.obj
    ---@field hash nelisp.obj
    ---@field next nelisp.obj
    ---@field index nelisp.obj
    ---@field count number
    ---@field next_free number
    ---@field mutable boolean
    ---@field rehash_threshold number (float)
    ---@field rehash_size number (float)
    ---@field key_and_value nelisp.obj
    ---@field test nelisp.hash_table_test

    obarray=15,
    terminal=16,
    ---@class nelisp._terminal: nelisp._pvec_special

    window_configuration=17,
    subr=18,
    ---@class nelisp._subr: nelisp._pvec
    ---@field fn fun(...:nelisp.obj):nelisp.obj
    ---@field minargs number
    ---@field maxargs number
    ---@field symbol_name string
    ---@field intspec string?
    ---@field docs string

    _other=19,
    xwidget=20,
    xwidget_view=21,
    thread=22,
    mutex=23,
    condvar=24,
    module_function=25,
    native_comp_unit=26,
    ts_parser=27,
    ts_node=28,
    ts_compiled_query=29,
    sqlite=30,
    closure=31,
    char_table=32,
    ---@class nelisp._char_table: nelisp._normal_vector
    ---@field default nelisp.obj
    ---@field parent nelisp.obj
    ---@field purpose nelisp.obj
    ---@field ascii nelisp.obj
    ---@field extras nelisp.obj

    sub_char_table=33,
    ---@class nelisp._sub_char_table: nelisp._normal_vector
    ---@field depth number
    ---@field min_char number

    compiled=34,
    ---@class nelisp._compiled: nelisp._normal_vector
    ---@field contents nelisp.obj[] --(1-indexed)

    record=35,
    ---@class nelisp._record: nelisp._normal_vector
    ---@field contents nelisp.obj[] --(1-indexed)

    font=36, --NOTE: this should also be a `nelisp._normal_vector`
}


--- ;; Makers
---@param ptr nelisp._fixnum|nelisp._string|nelisp._cons|nelisp._symbol|nelisp._float|nelisp._vectorlike
---@param t nelisp.type
---@return nelisp.obj
function M.make_ptr(ptr,t)
    ---@diagnostic disable-next-line: cast-type-mismatch
    ---@cast ptr nelisp.obj
    ptr[1]=t
    return ptr
end
---@param t nelisp.type
---@return nelisp.obj
function M.make_empty_ptr(t)
    ---@diagnostic disable-next-line: missing-fields
    return M.make_ptr({},t)
end
---@param ptr nelisp._pvec
---@param pvec_t nelisp.pvec
---@return nelisp.obj
function M.make_vectorlike_ptr(ptr,pvec_t)
    ---@diagnostic disable-next-line: cast-type-mismatch
    ---@cast ptr nelisp._vectorlike
    ptr.header=pvec_t
    return M.make_ptr(ptr,M.type.vectorlike)
end


--- ;; Symbol
---@param sym nelisp.obj
---@param plist nelisp.obj
function M.set_symbol_plist(sym,plist)
    (sym --[[@as nelisp._symbol]]).plist=plist
end
---@param sym nelisp._symbol
---@param v nelisp.obj?
function M.set_symbol_val(sym,v)
    assert(sym.redirect==M.symbol_redirect.plainval)
    sym.value=v
end
---@param sym nelisp.obj
---@param fn nelisp.obj
function M.set_symbol_function(sym,fn)
    (sym --[[@as nelisp._symbol]]).fn=fn
end
---@param sym nelisp.obj
---@return nelisp.obj
function M.symbol_name(sym)
    return (sym --[[@as nelisp._symbol]]).name
end
---@param sym nelisp.obj
---@param next_ nelisp.obj?
function M.set_symbol_next(sym,next_)
    (sym --[[@as nelisp._symbol]]).next=next_
end
---@param sym nelisp._symbol
---@return nelisp.obj
function M.symbol_val(sym)
    assert((sym --[[@as nelisp._symbol]]).redirect==M.symbol_redirect.plainval)
    return sym --[[@as nelisp._symbol]].value --[[@as nelisp.obj]]
end
---@param sym nelisp.obj
function M.make_symbol_constant(sym)
    (sym --[[@as nelisp._symbol]]).trapped_write=M.symbol_trapped_write.nowrite
end
---@param sym nelisp.obj
---@param trap nelisp.symbol_trapped_write
function M.set_symbol_trapped_write(sym,trap)
    if (sym --[[@as nelisp._symbol]]).trapped_write==M.symbol_trapped_write.nowrite then
        require'nelisp.signal'.xsignal(vars.Qtrapping_constant,sym)
    end
    (sym --[[@as nelisp._symbol]]).trapped_write=trap
end
---@param sym nelisp._symbol
---@param alias nelisp._symbol
function M.set_symbol_alias(sym,alias)
    assert((sym --[[@as nelisp._symbol]]).redirect==M.symbol_redirect.varalias)
    ;(sym --[[@as nelisp._symbol]]).value=alias
end
---@param sym nelisp._symbol
---@param blv nelisp.buffer_local_value
function M.set_symbol_blv(sym,blv)
    assert(sym.redirect==M.symbol_redirect.localized)
    sym.value=blv
end
---@param sym nelisp._symbol
---@return nelisp.buffer_local_value
function M.symbol_blv(sym)
    assert(sym.redirect==M.symbol_redirect.localized)
    return (sym --[[@as nelisp._symbol]]).value --[[@as nelisp.buffer_local_value]]
end
---@param sym nelisp._symbol
---@return nelisp._symbol
function M.symbol_alias(sym)
    assert(sym.redirect==M.symbol_redirect.varalias)
    return (sym --[[@as nelisp._symbol]]).value --[[@as nelisp._symbol]]
end
---@enum nelisp.symbol_redirect
M.symbol_redirect={
    plainval=1,
    varalias=2,
    localized=3,
    forwarded=4,
}
---@enum nelisp.symbol_interned
M.symbol_interned={
    uninterned=0,
    interned=1,
    interned_in_initial_obarray=2,
}
---@enum nelisp.symbol_trapped_write
M.symbol_trapped_write={
    untrapped=0,
    nowrite=1,
    trapped=2,
}


--- ;; Vector
---@param a nelisp.obj
---@return number
function M.asize(a)
    ---@diagnostic disable-next-line: invisible
    return (a --[[@as nelisp._normal_vector]]).size or 0
end
---@param a nelisp.obj
---@param idx number
---@return nelisp.obj
function M.aref(a,idx)
    assert(0<=idx and idx<M.asize(a))
    ---@diagnostic disable-next-line: invisible
    return ((a --[[@as nelisp._normal_vector]]).contents[idx+1] or vars.Qnil)
end
---@param a nelisp.obj
---@param idx number
---@param val nelisp.obj
function M.aset(a,idx,val)
    assert(0<=idx and idx<M.asize(a))
    ---@diagnostic disable-next-line: invisible
    a --[[@as nelisp._normal_vector]].contents[idx+1]=not M.nilp(val) and val or nil
end

--- ;; P functions
---@overload fun(x:nelisp.obj):boolean
function M.symbolp(x)
    return M.baresymbolp(x) or (M.symbolwithposp(x) and error('TODO') or false)
end
---@overload fun(x:nelisp.obj):boolean
function M.vectorp(x)
    return M.vectorlikep(x) and (x --[[@as nelisp._vectorlike]]).header==M.pvec.normal_vector
end
---@overload fun(x:nelisp.obj):boolean
function M.functionp(x)
    if M.symbolp(x) and not M.nilp(vars.F.fboundp(x)) then
        x=vars.F.indirect_function(x,vars.Qt)
        if M.consp(x) and M.eq(M.xcar(x),vars.Qautoload) then
            error('TODO')
        end
    end
    if M.subrp(x) then
        error('TODO')
    elseif M.compiledp(x) or M.module_functionp(x) then
        return true
    elseif M.consp(x) then
        local car=M.xcar(x)
        return M.eq(car,vars.Qlambda) or M.eq(car,vars.Qclosure)
    end
    return false
end
---@overload fun(lo:number,x:nelisp.obj,hi:number):boolean
function M.ranged_fixnump(lo,x,hi)
    return M.fixnump(x) and lo<=M.fixnum(x) and M.fixnum(x)<=hi
end
---@overload fun(x:nelisp.obj):boolean
function M.subr_native_compiled_dynp(_) return false end
---@overload fun(x:nelisp.obj):boolean
function M.integerp(x) return M.fixnump(x) or M.bignump(x) end
---@overload fun(x:nelisp.obj):boolean
function M.numberp(x) return M.integerp(x) or M.floatp(x) end
---@overload fun(x:nelisp.obj):boolean
function M.fixnatp(x) return M.fixnump(x) and 0<=M.fixnum(x) end
---@overload fun(x:nelisp.obj):boolean
function M.arrayp(x) return M.vectorp(x) or M.stringp(x) or M.chartablep(x) or M.boolvectorp(x) end
---@overload fun(x:nelisp.obj):boolean
function M.symbolconstantp(x) return (x --[[@as nelisp._symbol]]).trapped_write==M.symbol_trapped_write.nowrite end
---@overload fun(x:nelisp.obj):boolean
function M.nilp(x) return x==vars.Qnil end
---@overload fun(x:nelisp.obj):boolean
function M.symbolinternedininitialobarrayp(x) return (x --[[@as nelisp._symbol]]).interned==M.symbol_interned.interned_in_initial_obarray end
---@overload fun(x:nelisp.obj):boolean
function M._listp(x) return M.consp(x) or M.nilp(x) end
---@overload fun(x:nelisp.obj):boolean
function M.autoloadp(x) return M.consp(x) and M.eq(vars.Qautoload,M.xcar(x)) end
--- ;;; P functions type
---@overload fun(x:nelisp.obj):boolean
function M.baresymbolp(x) return M.xtype(x)==M.type.symbol end
---@overload fun(x:nelisp.obj):boolean
function M.vectorlikep(x) return M.xtype(x)==M.type.vectorlike end
---@overload fun(x:nelisp.obj):boolean
function M.stringp(x) return M.xtype(x)==M.type.string end
---@overload fun(x:nelisp.obj):boolean
function M.consp(x) return M.xtype(x)==M.type.cons end
---@overload fun(x:nelisp.obj):boolean
function M.fixnump(x) return M.xtype(x)==M.type.int0 end
---@overload fun(x:nelisp.obj):boolean
function M.floatp(x) return M.xtype(x)==M.type.float end
---- ;;; p functions vectorlike
---@overload fun(a:nelisp.obj,code:nelisp.pvec):boolean
function M.pseudovectorp(a,code)
    return M.vectorlikep(a) and (a --[[@as nelisp._vectorlike]]).header==code
end
---@overload fun(x:nelisp.obj):boolean
function M.symbolwithposp(x) return M.pseudovectorp(x,M.pvec.symbol_with_pos) end
---@overload fun(x:nelisp.obj):boolean It's better to have all p function in the same file
function M.bufferp(x) return M.pseudovectorp(x,M.pvec.buffer) end
---@overload fun(x:nelisp.obj):boolean
function M.subrp(x) return M.pseudovectorp(x,M.pvec.subr) end
---@overload fun(x:nelisp.obj):boolean
function M.compiledp(x) return M.pseudovectorp(x,M.pvec.compiled) end
---@overload fun(x:nelisp.obj):boolean
function M.module_functionp(x) return M.pseudovectorp(x,M.pvec.module_function) end
---@overload fun(x:nelisp.obj):boolean
function M.chartablep(x) return M.pseudovectorp(x,M.pvec.char_table) end
---@overload fun(x:nelisp.obj):boolean
function M.bignump(x) return M.pseudovectorp(x,M.pvec.bignum) end
---@overload fun(x:nelisp.obj):boolean
function M.hashtablep(x) return M.pseudovectorp(x,M.pvec.hash_table) end
---@overload fun(x:nelisp.obj):boolean
function M.recordp(x) return M.pseudovectorp(x,M.pvec.record) end
---@overload fun(x:nelisp.obj):boolean
function M.markerp(x) return M.pseudovectorp(x,M.pvec.marker) end
---@overload fun(x:nelisp.obj):boolean
function M.subchartablep(x) return M.pseudovectorp(x,M.pvec.sub_char_table) end
---@overload fun(x:nelisp.obj):boolean
function M.boolvectorp(x) return M.pseudovectorp(x,M.pvec.bool_vector) end
---@overload fun(x:nelisp.obj):boolean
function M.framep(x) return M.pseudovectorp(x,M.pvec.frame) end
---@overload fun(x:nelisp.obj):boolean
function M.fontp(x) return M.pseudovectorp(x,M.pvec.font) end
---@overload fun(x:nelisp.obj):boolean
function M.overlayp(x) return M.pseudovectorp(x,M.pvec.overlay) end
---@overload fun(x:nelisp.obj):boolean
function M.terminalp(x) return M.pseudovectorp(x,M.pvec.terminal) end

--- ;; Other
---@param x nelisp.obj
---@param y nelisp.obj
---@return boolean
function M.eq(x,y)
    assert(not M.pseudovectorp(x,M.pvec.symbol_with_pos),'TODO')
    assert(not M.pseudovectorp(y,M.pvec.symbol_with_pos),'TODO')
    return x==y
end

---@param x nelisp.obj
function M.loadhist_attach(x)
    if _G.nelisp_later then
        error('TODO')
    end
end
function M.event_head(event)
    return M.consp(event) and M.xcar(event) or event
end
--This is set in `configure.ac` in gnu-emacs
M.IS_DIRECTORY_SEP=function (c)
    --TODO: change depending on operating system
    return c==b'/'
end
---@param key nelisp.obj
---@return number
function M.xhash(key)
    return tonumber(tostring(key):sub(8)) or -1
end
---@param lower number
---@param num number
---@param upper number
---@return number
function M.clip_to_bounds(lower,num,upper)
    return num<lower and lower or num<=upper and num or upper
end


--- ;; Checkers
---@param ok boolean
---@param predicate nelisp.obj
---@param x nelisp.obj
function M.check_type(ok,predicate,x)
    if not ok then
        require'nelisp.signal'.wrong_type_argument(predicate,x)
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
---@overload fun(x:nelisp.obj)
function M.check_hash_table(x) M.check_type(M.hashtablep(x),vars.Qhash_table_p,x) end
---@overload fun(x:nelisp.obj)
function M.check_frame(x) M.check_type(M.framep(x),vars.Qframep,x) end
---@overload fun(x:nelisp.obj)
function M.check_buffer(x) M.check_type(M.bufferp(x),vars.Qbufferp,x) end
---@overload fun(x:nelisp.obj)
function M.check_overlay(x) M.check_type(M.overlayp(x),vars.Qbufferp,x) end
---@overload fun(x:nelisp.obj)
function M.check_number(x) M.check_type(M.numberp(x),vars.Qnumberp,x) end
---@param x nelisp.obj
---@return number
function M.check_vector_or_string(x)
    if M.vectorp(x) then
        return M.asize(x)
    elseif M.stringp(x) then
        return M.schars(x)
    else
        require'nelisp.signal'.wrong_type_argument(vars.Qarrayp,x)
        error('unreachable')
    end
end
---@param x nelisp.obj
---@return nelisp.obj
function M.check_number_coerce_marker(x)
    if M.markerp(x) then
        error('TODO')
    end
    M.check_type(M.numberp(x),vars.Qnumber_or_marker_p,x)
    return x
end
---@param x nelisp.obj
function M.check_string_car(x)
    M.check_type(M.stringp(M.xcar(x)),vars.Qstringp,M.xcar(x))
end
---@param x nelisp.obj
---@param lo number
---@param hi number
---@return number
function M.check_fixnum_range(x,lo,hi)
    M.check_integer(x)
    if lo<=M.fixnum(x) and M.fixnum(x)<=hi then
        return M.fixnum(x)
    end
    require'nelisp.signal'.args_out_of_range(x,M.make_fixnum(lo),M.make_fixnum(hi))
    error('unreachable')
end


--- ;; List functions (Cons functions)
---@generic T: nelisp.obj|boolean
---@param x nelisp.obj
---@param fn fun(x:nelisp.obj):'continue'|'break'|T|nil
---@param safe boolean?
---@return T
---@return nelisp.obj
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
        local result=fn(x)
        if result=='break' then
            return nil,x
        elseif result=='continue' then
        elseif result~=nil then
            return result,x
        end
        x=M.xcdr(x)
    end
    return nil,x
end
---@generic T: nelisp.obj|boolean
---@param x nelisp.obj
---@param fn fun(x:nelisp.obj):'continue'|'break'|T|nil
---@return T
---@return nelisp.obj
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
---@return nelisp.obj
function M.list(...)
    local alloc=require'nelisp.alloc'
    local args={...}
    local val=vars.Qnil
    for i=#args,1,-1 do
        val=alloc.cons(args[i],val)
    end
    return val
end


--- ;; String
---@param x nelisp.obj
---@return number
function M.schars(x)
    local s=x --[[@as nelisp._string]]
    local nbytes=assert(s.size_chars==nil and #s[2] or s.size_chars)
    return nbytes
end
---@param x nelisp.obj
---@return number
function M.sbytes(x)
    return #((x --[[@as nelisp._string]])[2])
end
---@param x nelisp.obj
---@return string
function M.sdata(x)
    return (x --[[@as nelisp._string]])[2]
end
---@param x nelisp.obj
---@param idx number
---@return number
function M.sref(x,idx)
    local p=(x --[[@as nelisp._string]])
    if #p[2]==idx then
        return 0
    end
    return string.byte((x --[[@as nelisp._string]])[2],idx+1)
end
---@param x nelisp.obj
---@return boolean
function M.string_multibyte(x)
    return (x --[[@as nelisp._string]]).size_chars~=nil
end
---@param x nelisp.obj
---@return nelisp.intervals?
function M.string_intervals(x)
    return (x --[[@as nelisp._string]]).intervals
end
---@param x nelisp.obj
---@param intervals nelisp.intervals?
function M.set_string_intervals(x,intervals)
    (x --[[@as nelisp._string]]).intervals=intervals
end


--- ;; Cons
---@param c nelisp.obj
---@return nelisp.obj
function M.xcar(c)
    return (c --[[@as nelisp._cons]])[2]
end
---@param c nelisp.obj
---@return nelisp.obj
function M.xcdr(c)
    return (c --[[@as nelisp._cons]])[3]
end
---@param c nelisp.obj
---@param newcdr nelisp.obj
function M.xsetcdr(c,newcdr)
    (c --[[@as nelisp._cons]])[3]=newcdr
end
---@param c nelisp.obj
---@param newcar nelisp.obj
function M.xsetcar(c,newcar)
    (c --[[@as nelisp._cons]])[2]=newcar
end


--- ;; fixnum
---@param x nelisp.obj
---@return number
function M.fixnum(x)
    return (x --[[@as nelisp._fixnum]])[2]
end
local fixnum_cache=setmetatable({},{__mode='v'})
---@param n number
---@return nelisp.obj
function M.make_fixnum(n)
    if not fixnum_cache[n] then
        fixnum_cache[n]=M.make_ptr({[2]=n},M.type.int0)
    end
    return fixnum_cache[n]
end


--- ;; float
---@param f nelisp.obj
---@param n number (float)
---@return nil
function M.xfloat_init(f,n)
    (f --[[@as nelisp._float]])[2]=n
end
---@param f nelisp.obj
---@return number (float)
function M.xfloat_data(f)
    return (f --[[@as nelisp._float]])[2]
end
---@param f nelisp.obj
---@return number (float)
function M.xfloatint(f)
    if M.floatp(f) then
        return M.xfloat_data(f)
    elseif M.fixnump(f) then
        return M.fixnum(f)
    elseif M.bignump(f) then
        error('TODO')
    else
        error('unreachable')
    end
end


--- ;; hashtable
---@class nelisp.hash_table_test
---@field name nelisp.obj
---@field user_hash_function nelisp.obj
---@field user_cmp_function nelisp.obj
---@field cmpfn (fun(a:nelisp.obj,b:nelisp.obj,h:nelisp._hash_table):boolean)|0
---@field hashfn fun(a:nelisp.obj,h:nelisp._hash_table):number

--- ;; compiled
--- @enum nelisp.compiled_idx
M.compiled_idx={
    arglist=1,
    bytecode=2,
    constants=3,
    stack_depth=4,
    doc_string=5,
    interactive=6,
}

--- ;; bool vector
---@param a nelisp.obj
---@return number
function M.bool_vector_size(a)
    return #(a --[[@as nelisp._bool_vector]]).contents
end
---@param a nelisp.obj
---@param idx number
---@return boolean
function M.bool_vector_bitref(a,idx)
    assert(0<=idx and idx<M.bool_vector_size(a))
    return (a --[[@as nelisp._bool_vector]]).contents[idx+1]
end
---@param a nelisp.obj
---@param idx number
---@param val boolean
function M.bool_vector_set(a,idx,val)
    assert(0<=idx and idx<M.bool_vector_size(a))
    ;(a --[[@as nelisp._bool_vector]]).contents[idx+1]=val
end
return M
