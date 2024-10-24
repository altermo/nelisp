local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local signal=require'nelisp.signal'
local alloc=require'nelisp.alloc'
local chars=require'nelisp.chars'

local M={}
local CHARTAB_SIZE_BITS_0=6
local CHARTAB_SIZE_BITS_1=4
local CHARTAB_SIZE_BITS_2=5
local CHARTAB_SIZE_BITS_3=7
local chartab_bits={
    (CHARTAB_SIZE_BITS_1+CHARTAB_SIZE_BITS_2+CHARTAB_SIZE_BITS_3),
    (CHARTAB_SIZE_BITS_2+CHARTAB_SIZE_BITS_3),
    (CHARTAB_SIZE_BITS_3),
    0}
local chartab_chars={
    bit.lshift(1,chartab_bits[1]),
    bit.lshift(1,chartab_bits[2]),
    bit.lshift(1,chartab_bits[3]),
    bit.lshift(1,chartab_bits[4])}
local chartab_size={
    bit.lshift(1,CHARTAB_SIZE_BITS_0),
    bit.lshift(1,CHARTAB_SIZE_BITS_1),
    bit.lshift(1,CHARTAB_SIZE_BITS_2),
    bit.lshift(1,CHARTAB_SIZE_BITS_3)}
local function chartab_idx(c,depth,min_char)
    return bit.rshift(c-min_char,assert(chartab_bits[depth+1]))
end
---@param depth number
---@param min_char number
---@param default nelisp.obj
---@return nelisp.obj
local function make_subchartable(depth,min_char,default)
    local v=alloc.make_vector(chartab_size[depth+1],default) --[[@as nelisp._sub_char_table]]
    v.depth=depth
    v.min_char=min_char
    return lisp.make_vectorlike_ptr(v,lisp.pvec.sub_char_table)
end
---@param tbl nelisp.obj
---@param idx number
---@param val nelisp.obj
local function set_char_table_contents(tbl,idx,val)
    assert(0<=idx and idx<=chartab_size[1])
    ;(tbl --[[@as nelisp._char_table]]).contents[idx+1]=val
end
---@param t nelisp._char_table
---@return number
local function chartable_extra_slots(t)
    return lisp.asize(t.extras)
end
---@param t nelisp.obj
---@return boolean
local function uniproptablep(t)
    return lisp.eq((t --[[@as nelisp._char_table]]).purpose,vars.Qchar_code_property_table)
        and chartable_extra_slots(t --[[@as nelisp._char_table]])==5
end
---@param ctable nelisp.obj
---@param c number
---@param val nelisp.obj
---@param is_uniprop boolean
local function sub_char_table_set(ctable,c,val,is_uniprop)
    local tbl=(ctable --[[@as nelisp._sub_char_table]])
    local depth=tbl.depth
    local min_char=tbl.min_char
    local i=chartab_idx(c,depth,min_char)
    if depth==3 then
        tbl.contents[i+1]=val
    else
        local sub=tbl.contents[i+1] or vars.Qnil
        if not lisp.subchartablep(sub) then
            if is_uniprop then
                error('TODO')
            else
                sub=make_subchartable(depth+1,min_char+i*chartab_chars[depth+1],sub)
                tbl.contents[i+1]=sub
            end
        end
        sub_char_table_set(sub,c,val,is_uniprop)
    end
end
---@param ctable nelisp.obj
---@return nelisp.obj
local function char_table_ascii(ctable)
    local sub=(ctable --[[@as nelisp._char_table]]).contents[1] or vars.Qnil
    if not lisp.subchartablep(sub) then return sub end
    sub=(sub --[[@as nelisp._sub_char_table]]).contents[1] or vars.Qnil
    if not lisp.subchartablep(sub) then return sub end
    local val=(sub --[[@as nelisp._sub_char_table]]).contents[1] or vars.Qnil
    if uniproptablep(ctable) then
        error('TODO')
    end
    return val
end
---@param ctable nelisp.obj
---@param c number
---@param val nelisp.obj
function M.set(ctable,c,val)
    local tbl=(ctable --[[@as nelisp._char_table]])
    if chars.asciicharp(c)
        and lisp.subchartablep(tbl.ascii) then
        (tbl.ascii --[[@as nelisp._sub_char_table]]).contents[c+1]=val
    else
        local i=chartab_idx(c,0,0)
        local sub=tbl.contents[i+1] or vars.Qnil
        if not lisp.subchartablep(sub) then
            sub=make_subchartable(1,i*chartab_chars[1],sub)
            set_char_table_contents(ctable,i,sub)
        end
        sub_char_table_set(sub,c,val,uniproptablep(ctable))
        if chars.asciicharp(c) then
            tbl.ascii=char_table_ascii(ctable)
        end
    end
end
---@param ctable nelisp.obj
---@param c number
---@param is_uniprop boolean
---@return nelisp.obj
local function sub_char_table_ref(ctable,c,is_uniprop)
    local tbl=(ctable --[[@as nelisp._sub_char_table]])
    local idx=chartab_idx(c,tbl.depth,tbl.min_char)
    local val=tbl.contents[idx+1] or vars.Qnil
    if is_uniprop then
        error('TODO')
    end
    if lisp.subchartablep(val) then
        val=sub_char_table_ref(val,c,is_uniprop)
    end
    return val
end
---@param ctable nelisp.obj
---@param c number
---@return nelisp.obj
function M.ref(ctable,c)
    local tbl=(ctable --[[@as nelisp._char_table]])
    local val
    if chars.asciicharp(c) then
        val=tbl.ascii
        if lisp.subchartablep(val) then
            val=(val --[[@as nelisp._sub_char_table]]).contents[c+1] or vars.Qnil
        end
    else
        val=tbl.contents[chartab_idx(c,0,0)+1] or vars.Qnil
        if lisp.subchartablep(val) then
            val=sub_char_table_ref(val,c,uniproptablep(ctable))
        end
    end
    if lisp.nilp(val) then
        val=tbl.default
        if lisp.nilp(val) and lisp.chartablep(tbl.parent) then
            M.ref(tbl.parent,c)
        end
    end
    return val
end
---@param ctable nelisp.obj
---@return nelisp.obj
local function copy_sub_char_table(ctable)
    local tbl=(ctable --[[@as nelisp._sub_char_table]])
    local depth=tbl.depth
    local min_char=tbl.min_char
    local copy=make_subchartable(depth,min_char,vars.Qnil)
    for i=0,chartab_size[depth+1]-1 do
        local val=(ctable --[[@as nelisp._sub_char_table]]).contents[i+1] or vars.Qnil
        ;(copy --[[@as nelisp._sub_char_table]]).contents[i+1]=(lisp.subchartablep(val)
        and copy_sub_char_table(val)
        or val)
    end
    return copy
end
---@param ctable nelisp.obj
---@return nelisp.obj
function M.copy_char_table(ctable)
    local tbl=(ctable --[[@as nelisp._char_table]])
    local size=lisp.asize(ctable)
    local copy=alloc.make_vector(size,'nil') --[[@as nelisp._char_table]]
    copy.default=tbl.default
    copy.parent=tbl.parent
    copy.purpose=tbl.purpose
    for i=0,chartab_size[1]-1 do
        copy.contents[i+1]=(lisp.subchartablep(tbl.contents[i+1] or vars.Qnil)
        and copy_sub_char_table(tbl.contents[i+1] or vars.Qnil)
        or tbl.contents[i+1] or vars.Qnil)
    end
    local obj=lisp.make_vectorlike_ptr(copy,lisp.pvec.char_table)
    copy.ascii=char_table_ascii(obj)
    copy.extras=alloc.make_vector(chartable_extra_slots(tbl),'nil')
    for i=0,chartable_extra_slots(tbl)-1 do
        M.set_extra(obj,i,lisp.aref(tbl.extras,i))
    end
    return obj
end
---@param ctable nelisp.obj
---@param idx number
---@param val nelisp.obj
function M.set_extra(ctable,idx,val)
    assert(0<=idx and idx<chartable_extra_slots(ctable --[[@as nelisp._char_table]]))
    lisp.aset((ctable --[[@as nelisp._char_table]]).extras,idx,val)
end

local F={}
F.char_table_extra_slot={'char-table-extra-slot',2,2,0,[[Return the value of CHAR-TABLE's extra-slot number N.]]}
function F.char_table_extra_slot.f(ctable,n)
    lisp.check_chartable(ctable)
    lisp.check_fixnum(n)
    if lisp.fixnum(n)<0 or lisp.fixnum(n)>=chartable_extra_slots(ctable) then
        signal.args_out_of_range(ctable,n)
    end
    return lisp.aref((ctable --[[@as nelisp._char_table]]).extras,lisp.fixnum(n))
end
F.set_char_table_extra_slot={'set-char-table-extra-slot',3,3,0,[[Set CHAR-TABLE's extra-slot number N to VALUE.]]}
function F.set_char_table_extra_slot.f(ctable,n,value)
    lisp.check_chartable(ctable)
    lisp.check_fixnum(n)
    if lisp.fixnum(n)<0 or lisp.fixnum(n)>=chartable_extra_slots(ctable) then
        signal.args_out_of_range(ctable,n)
    end
    lisp.aset((ctable --[[@as nelisp._char_table]]).extras,lisp.fixnum(n),value)
    return value
end
F.make_char_table={'make-char-table',1,2,0,[[Return a newly created char-table, with purpose PURPOSE.
Each element is initialized to INIT, which defaults to nil.

PURPOSE should be a symbol.  If it has a `char-table-extra-slots'
property, the property's value should be an integer between 0 and 10
that specifies how many extra slots the char-table has.  Otherwise,
the char-table has no extra slot.]]}
function F.make_char_table.f(purpose,init)
    lisp.check_symbol(purpose)
    local n=vars.F.get(purpose,vars.Qchar_table_extra_slots)
    local n_extras
    if lisp.nilp(n) then
        n_extras=0
    else
        lisp.check_fixnat(n)
        if lisp.fixnum(n)>10 then
            signal.args_out_of_range(n,vars.Qnil)
        end
        n_extras=lisp.fixnum(n)
    end
    local vector=alloc.make_vector(chartab_size[1],init) --[[@as nelisp._char_table]]
    -- Is this correct?:
    vector.ascii=init
    vector.default=init
    vector.extras=alloc.make_vector(n_extras,init)
    --vector.ascii=vars.Qnil
    --vector.default=vars.Qnil
    vector.parent=vars.Qnil
    vector.purpose=purpose
    return lisp.make_vectorlike_ptr(vector,lisp.pvec.char_table)

end

function M.init_syms()
    vars.defsym('Qchar_code_property_table','char-code-property-table')

    vars.defsubr(F,'make_char_table')
    vars.defsubr(F,'char_table_extra_slot')
    vars.defsubr(F,'set_char_table_extra_slot')
end
return M
