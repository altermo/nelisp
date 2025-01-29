local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local signal=require'nelisp.signal'
local intervals=require'nelisp.intervals'
local buffer=require'nelisp.buffer'

local M={}
function M.copy_textprop(start,end_,src,pos,dest,props)
    if _G.nelisp_later then
        error('TODO')
    end
end

---@type nelisp.F
local F={}
---@return nelisp.obj
local function validate_plist(list)
    if lisp.nilp(list) then
        return vars.Qnil
    end
    if lisp.consp(list) then
        local tail=list
        while lisp.consp(tail) do
            tail=lisp.xcdr(tail)
            if not lisp.consp(tail) then
                signal.error('Odd length text property list')
            end
            tail=lisp.xcdr(tail)
        end
        return list
    end
    return lisp.list(list,vars.Qnil)
end
---@param begin nelisp.ptr
---@param end_ nelisp.ptr
---@return nelisp.intervals?
local function validate_interval_range(obj,begin,end_,force)
    lisp.check_type(lisp.stringp(obj) or lisp.bufferp(obj),vars.Qbuffer_or_string_p,obj)
    local begin0=begin[1]
    local end0=end_[1]
    begin[1]=buffer.check_fixnum_coerce_marker(begin[1])
    end_[1]=buffer.check_fixnum_coerce_marker(end_[1])
    if lisp.eq(begin[1],end_[1]) and begin~=end_ then
        return nil
    end
    if lisp.fixnum(begin[1])>lisp.fixnum(end_[1]) then
        local n=begin[1]
        begin[1]=end_[1]
        end_[1]=n
    end
    local i,searchpos
    if lisp.bufferp(obj) then
        error('TODO')
    else
        local len=lisp.schars(obj)
        if not (0<=lisp.fixnum(begin[1]) and lisp.fixnum(begin[1])<=lisp.fixnum(end_[1])
            and lisp.fixnum(end_[1])<=len) then
            signal.args_out_of_range(begin0,end0)
        end
        i=lisp.string_intervals(obj)
        if len==0 then
            return nil
        end
        searchpos=lisp.fixnum(begin[1])
    end
    if not i then
        return force and intervals.create_root_interval(obj) or i
    end
    return intervals.find_interval(i,searchpos)
end
---@param plist nelisp.obj
---@param i nelisp.intervals
---@return boolean
local function interval_has_all_properties(plist,i)
    local tail1=plist
    while lisp.consp(tail1) do
        local sym1=lisp.xcar(tail1)
        local found=false
        local tail2=i.plist
        while lisp.consp(tail2) do
            if lisp.eq(sym1,lisp.xcar(tail2)) then
                if not lisp.eq(vars.F.car(lisp.xcdr(tail2)),vars.F.car(lisp.xcdr(tail1))) then
                    return false
                end
                found=true
                break
            end
            tail2=vars.F.cdr(lisp.xcdr(tail2))
        end
        if not found then
            return false
        end
        tail1=vars.F.cdr(lisp.xcdr(tail1))
    end
    return true
end
---@param plist nelisp.obj
---@param i nelisp.intervals
---@param obj nelisp.obj
---@param set_type 'REPLACE'|'APPEND'|'PREPEND'
---@param destructive boolean
---@return boolean
local function add_properties(plist,i,obj,set_type,destructive)
    local tail1=plist
    local changed=false
    while lisp.consp(tail1) do
        local found=false
        local sym1=lisp.xcar(tail1)
        local val1=vars.F.car(lisp.xcdr(tail1))
        local tail2=i.plist
        while lisp.consp(tail2) do
            if lisp.eq(sym1,lisp.xcar(tail2)) then
                error('TODO')
            end
            tail2=vars.F.cdr(lisp.xcdr(tail2))
        end
        if not found then
            if lisp.bufferp(obj) then
                error('TODO')
            end
            i.plist=vars.F.cons(sym1,vars.F.cons(val1,i.plist))
            changed=true
        end
        tail1=vars.F.cdr(lisp.xcdr(tail1))
    end
    return changed
end
---@param set_type 'REPLACE'|'APPEND'|'PREPEND'
---@param start nelisp.obj
---@param end_ nelisp.obj
---@param destructive boolean
---@return nelisp.obj
local function add_text_properties_1(start,end_,properties,obj,set_type,destructive)
    if lisp.bufferp(obj) then
        error('TODO')
    end
    properties=validate_plist(properties)
    if lisp.nilp(properties) then
        return vars.Qnil
    end
    if lisp.nilp(obj) then
        error('TODO')
    end
    ---@type nelisp.ptr
    local sstart={start}
    ---@type nelisp.ptr
    local send={end_}
    local i=validate_interval_range(obj,sstart,send,true)
    if not i then
        return vars.Qnil
    end
    local s=lisp.fixnum(sstart[1])
    local len=lisp.fixnum(send[1])-s
    if interval_has_all_properties(properties,i) then
        error('TODO')
    elseif i.position~=s then
        error('TODO')
    end
    if lisp.bufferp(obj) then
        error('TODO')
    end
    local modifiers=false
    while true do
        assert(i)
        if intervals.length(i)>=len then
            if interval_has_all_properties(properties,i) then
                error('TODO')
            end
            if intervals.length(i)==len then
                add_properties(properties,i,obj,set_type,destructive)
                if lisp.bufferp(obj) then
                    error('TODO')
                end
                return vars.Qt
            end
            error('TODO')
        end
        len=len-intervals.length(i)
        modifiers=modifiers or add_properties(properties,i,obj,set_type,destructive)
        i=intervals.next_interval(i)
    end
end
F.add_text_properties={'add-text-properties',3,4,0,[[Add properties to the text from START to END.
The third argument PROPERTIES is a property list
specifying the property values to add.  If the optional fourth argument
OBJECT is a buffer (or nil, which means the current buffer),
START and END are buffer positions (integers or markers).
If OBJECT is a string, START and END are 0-based indices into it.
Return t if any property value actually changed, nil otherwise.]]}
function F.add_text_properties.f(start,end_,properties,obj)
    return add_text_properties_1(start,end_,properties,obj,'REPLACE',true)
end

function M.init()
    vars.V.text_property_default_nonsticky=lisp.list(vars.F.cons(vars.Qsyntax_table,vars.Qt),vars.F.cons(vars.Qdisplay,vars.Qt))
end
function M.init_syms()
    vars.defsubr(F,'add_text_properties')

    vars.defvar_lisp('text_property_default_nonsticky','text-property-default-nonsticky',[[Alist of properties vs the corresponding non-stickiness.
Each element has the form (PROPERTY . NONSTICKINESS).

If a character in a buffer has PROPERTY, new text inserted adjacent to
the character doesn't inherit PROPERTY if NONSTICKINESS is non-nil,
inherits it if NONSTICKINESS is nil.  The `front-sticky' and
`rear-nonsticky' properties of the character override NONSTICKINESS.]])
end
return M
