local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local signal=require'nelisp.signal'
local b=require'nelisp.bytes'
local data=require'nelisp.data'
local fns=require'nelisp.fns'
local alloc=require'nelisp.alloc'
local M={}
local modifiers_t={
    up=1,
    down=2,
    drag=4,
    click=8,
    double=16,
    triple=32,
}

local F={}
F.make_keymap={'make-keymap',0,1,0,[[Construct and return a new keymap, of the form (keymap CHARTABLE . ALIST).
CHARTABLE is a char-table that holds the bindings for all characters
without modifiers.  All entries in it are initially nil, meaning
"command undefined".  ALIST is an assoc-list which holds bindings for
function keys, mouse events, and any other things that appear in the
input stream.  Initially, ALIST is nil.

The optional arg STRING supplies a menu name for the keymap
in case you use it as a menu with `x-popup-menu'.]]}
function F.make_keymap.f(s)
    local tail=not lisp.nilp(s) and lisp.list(s) or vars.Qnil
    return vars.F.cons(vars.Qkeymap,vars.F.cons(vars.F.make_char_table(vars.Qkeymap,vars.Qnil),tail))
end
---@param error_if_not_keymap boolean
local function get_keymap(obj,error_if_not_keymap,autoload)
    if lisp.nilp(obj) then
        if error_if_not_keymap then
            signal.wrong_type_argument(vars.Qkeymapp,obj)
        end
        return vars.Qnil
    end
    if lisp.consp(obj) and lisp.eq(lisp.xcar(obj),vars.Qkeymap) then
        return obj
    end
    local tem=data.indirect_function(obj)
    if lisp.consp(tem) then
        if lisp.eq(lisp.xcar(tem),vars.Qkeymap) then
            return tem
        end
        if (autoload or not error_if_not_keymap) and lisp.eq(lisp.xcar(tem),vars.Qautoload) and lisp.symbolp(obj) then
            error('TODO')
        end
    end
    if error_if_not_keymap then
        signal.wrong_type_argument(vars.Qkeymapp,obj)
    end
    return vars.Qnil
end
local function possibly_translate_key_sequence(key)
    if lisp.vectorp(key) and lisp.asize(key)==1 and lisp.stringp(lisp.aref(key,0)) then
        error('TODO')
    end
    return key
end
local function parse_modifiers_uncached(sym)
    lisp.check_symbol(sym)
    local mods=0
    local name=lisp.symbol_name(sym)
    local i=0
    if not _G.nelisp_later then
        error('TODO: maybe ...-1 is incorrect, and it should just be ...')
    end
    while i<lisp.sbytes(name)-1 do
        local this_mod_end=0
        local this_mod=0
        local c=lisp.sref(name,i)
        if c=='A' then error('TODO')
        elseif c=='C' then error('TODO')
        elseif c=='H' then error('TODO')
        elseif c=='M' then error('TODO')
        elseif c=='S' then error('TODO')
        elseif c=='s' then error('TODO')
        elseif c=='d' then error('TODO')
        elseif c=='t' then error('TODO')
        elseif c=='u' then error('TODO')
        end
        if this_mod_end==0 then
            break
        end
        error('TODO')
    end
    if (bit.band(mods,bit.bor(modifiers_t.down,modifiers_t.drag
        ,modifiers_t.double,modifiers_t.triple))==0)
        and i+7==lisp.sbytes(name)
        and lisp.sdata(name):sub(i-1,i-1+6)=='mouse-'
        and b'0'<=lisp.sref(name,i+6) and lisp.sref(name,i+6)<=b'9'
    then
        error('TODO')
    end
    if (bit.band(mods,bit.bor(modifiers_t.double,modifiers_t.triple))==0)
        and i+6==lisp.sbytes(name)
        and lisp.sdata(name):sub(i-1,i-1+6)=='mouse-'
    then
        error('TODO')
    end
    return mods,i
end
local function lispy_modifier_list(modifiers)
    local ret=vars.Qnil
    while modifiers>0 do
        error('TODO')
    end
    return ret
end
local function parse_modifiers(sym)
    if lisp.fixnump(sym) then
        error('TODO')
    elseif not lisp.symbolp(sym) then
        error('TODO')
    end
    local elements=vars.F.get(sym,vars.Qevent_symbol_element_mask)
    if lisp.consp(elements) then
        return elements
    end
    local modifiers,end_=parse_modifiers_uncached(sym)
    local unmodified=vars.F.intern(alloc.make_string(lisp.sdata(lisp.symbol_name(sym)):sub(end_+1)),vars.Qnil)
    local mask=lisp.make_fixnum(modifiers)
    elements=lisp.list(unmodified,mask)
    vars.F.put(sym,vars.Qevent_symbol_element_mask,elements)
    vars.F.put(sym,vars.Qevent_symbol_elements,vars.F.cons(unmodified,lispy_modifier_list(modifiers)))
    return elements
end
local function apply_modifiers_uncached(modifiers,base)
    local new_mods={}
    for _,v in ipairs({{b.CHAR_ALT,'A'},{b.CHAR_CTL,'C'},{b.CHAR_HYPER,'H'},{b.CHAR_SHIFT,'S'},
        {b.CHAR_SUPER,'s'},{modifiers_t.double,'double'},{modifiers_t.triple,'triple'},{modifiers_t.up,'up'}
        ,{modifiers_t.down,'down'},{modifiers_t.drag,'drag'},{modifiers_t.click,'click'}}) do
        if bit.band(modifiers,v[1])>0 then
            table.insert(new_mods,v[2]..'-')
        end
    end
    local new_name=alloc.make_multibyte_string(table.concat(new_mods)..base,-1)
    return vars.F.intern(new_name,vars.Qnil)
end
local function apply_modifiers(modifiers,base)
    if lisp.fixnump(base) then
        error('TODO')
    end
    local cache=vars.F.get(base,vars.Qmodifier_cache)
    local idx=lisp.make_fixnum(bit.band(modifiers,bit.bnot(modifiers_t.click)))
    local entry=fns.assq_no_quit(idx,cache)
    local new_symbol
    if lisp.consp(entry) then
        new_symbol=lisp.xcdr(entry)
    else
        new_symbol=apply_modifiers_uncached(modifiers,lisp.sdata(lisp.symbol_name(base)))
        entry=vars.F.cons(idx,new_symbol)
        vars.F.put(base,vars.Qmodifier_cache,vars.F.cons(entry,cache))
    end
    if lisp.nilp(vars.F.get(new_symbol,vars.Qevent_kind)) then
        local kind=vars.F.get(base,vars.Qevent_kind)
        if not lisp.nilp(kind) then
            vars.F.put(new_symbol,vars.Qevent_kind,kind)
        end
    end
    return new_symbol
end
local function reorder_modifiers(sym)
    local parsed=parse_modifiers(sym)
    return apply_modifiers(lisp.fixnum(lisp.xcar(lisp.xcdr(parsed))),lisp.xcar(parsed))
end
local function store_in_keymap(keymap,idx,def,remove)
    if lisp.eq(idx,vars.Qkeymap) then
        signal.error("`keymap' is reserved for embedded parent maps")
    end
    if not lisp.consp(keymap) or not lisp.eq(lisp.xcar(keymap),vars.Qkeymap) then
        signal.error('attempt to define a key in a non-keymap')
    end
    if lisp.consp(idx) and lisp.chartablep(lisp.xcar(idx)) then
        error('TODO')
    else
        idx=lisp.event_head(idx)
    end
    if lisp.symbolp(idx) then
        idx=reorder_modifiers(idx)
    elseif lisp.fixnump(idx) then
        idx=lisp.make_fixnum(bit.band(lisp.fixnum(idx),bit.bor(b.CHAR_META,b.CHAR_META-1)))
    end
    local tail=lisp.xcdr(keymap)
    local insertion_point=keymap
    while lisp.consp(tail) do
        local elt=lisp.xcar(tail)
        if lisp.vectorp(elt) then
            error('TODO')
        elseif lisp.chartablep(elt) then
            local sdef=def
            if remove then
                sdef=vars.Qnil
            elseif lisp.nilp(def) then
                sdef=vars.Qt
            end
            if lisp.fixnatp(idx) and bit.band(lisp.fixnum(idx),b.CHAR_MODIFIER_MASK)==0 then
                vars.F.aset(elt,idx,sdef)
                return def
            end
            error('TODO')
        elseif lisp.consp(elt) then
            if lisp.eq(vars.Qkeymap,lisp.xcar(elt)) then
                error('TODO')
            elseif lisp.eq(idx,lisp.xcar(elt)) then
                if remove then
                    error('TODO')
                else
                    lisp.xsetcdr(elt,def)
                end
                return def
            elseif lisp.consp(idx) and lisp.chartablep(lisp.xcar(idx)) and lisp.chartablep(lisp.xcar(elt)) then
                error('TODO')
            end
        elseif lisp.eq(elt,vars.Qkeymap) then
            break
        end
        tail=lisp.xcdr(tail)
    end
    if not remove then
        local elt
        if lisp.consp(idx) and lisp.chartablep(lisp.xcar(idx)) then
            error('TODO')
        else
            elt=vars.F.cons(idx,def)
        end
        lisp.xsetcdr(insertion_point,vars.F.cons(elt,lisp.xcdr(insertion_point)))
    end
    return def
end
local function get_keyelt(obj,autoload)
    while true do
        if not lisp.consp(obj) then
            return obj
        elseif lisp.eq(lisp.xcar(obj),vars.Qmenu_item) then
            error('TODO')
        elseif lisp.stringp(lisp.xcar(obj)) then
            error('TODO')
        else
            return obj
        end
    end
end
local function keymapp(m)
    return not lisp.nilp(get_keymap(m,false,false))
end
local function access_keymap_1(map,idx,t_ok,noinherit,autoload)
    idx=lisp.event_head(idx)
    if lisp.symbolp(idx) then
        idx=reorder_modifiers(idx)
    elseif lisp.fixnump(idx) then
        idx=lisp.make_fixnum(bit.band(lisp.fixnum(idx),bit.bor(b.CHAR_META,b.CHAR_META-1)))
    end
    if lisp.fixnump(idx) and bit.band(lisp.fixnum(idx),b.CHAR_META)>0 then
        error('TODO')
    end
    local tail=lisp.consp(map) and lisp.eq(vars.Qkeymap,lisp.xcar(map)) and lisp.xcdr(map) or map
    local retval=nil
    local t_bindning=nil
    while true do
        if not lisp.consp(tail) then
            tail=get_keymap(tail,false,autoload)
            if not lisp.consp(tail) then
                break
            end
        end

        local val=nil
        local binding=lisp.xcar(tail)
        local submap=get_keymap(binding,false,autoload)
        if lisp.eq(binding,vars.Qkeymap) then
            if noinherit or (retval==nil or lisp.nilp(retval)) then
                break
            end
            error('TODO')
        elseif lisp.consp(submap) then
            error('TODO')
        elseif lisp.consp(binding) then
            local key=lisp.xcar(binding)
            if lisp.eq(key,idx) then
                val=lisp.xcdr(binding)
            elseif (t_ok and lisp.eq(key,vars.Qt)) then
                error('TODO')
            end
        elseif lisp.vectorp(binding) then
            error('TODO')
        elseif lisp.chartablep(binding) then
            if lisp.fixnump(idx) and bit.band(lisp.fixnum(idx),b.CHAR_MODIFIER_MASK)==0 then
                val=vars.F.aref(binding,idx)
                if lisp.nilp(val) then
                    val=nil
                end
            end
        end
        if val then
            if lisp.eq(val,vars.Qt) then
                val=vars.Qnil
            end
            val=get_keyelt(val,autoload)
            if not keymapp(val) then
                error('TODO')
            elseif retval==nil or lisp.nilp(retval) then
                retval=val
            else
                error('TODO')
            end
        end

        tail=lisp.xcdr(tail)
    end
    return retval~=nil and retval or (t_bindning~=nil and get_keyelt(t_bindning,autoload) or nil)
end
---@return nelisp.obj
local function access_keymap(map,idx,t_ok,noinherit,autoload)
    return access_keymap_1(map,idx,t_ok,noinherit,autoload) or vars.Qnil
end
local function silly_event_symbol_error(sym)
    if not _G.nelisp_later then
        error('TODO')
    end
end
local function define_as_prefix(keymap,c)
    local cmd=vars.F.make_sparse_keymap(vars.Qnil)
    store_in_keymap(keymap,c,cmd,false)
    return cmd
end
F.define_key={'define-key',3,4,0,[[In KEYMAP, define key sequence KEY as DEF.
This is a legacy function; see `keymap-set' for the recommended
function to use instead.

KEYMAP is a keymap.

KEY is a string or a vector of symbols and characters, representing a
sequence of keystrokes and events.  Non-ASCII characters with codes
above 127 (such as ISO Latin-1) can be represented by vectors.
Two types of vector have special meanings:
 [remap COMMAND] remaps any key binding for COMMAND.
 [t] creates a default definition, which applies to any event with no
    other definition in KEYMAP.

DEF is anything that can be a key's definition:
 nil (means key is undefined in this keymap),
 a command (a Lisp function suitable for interactive calling),
 a string (treated as a keyboard macro),
 a keymap (to define a prefix key),
 a symbol (when the key is looked up, the symbol will stand for its
    function definition, which should at that time be one of the above,
    or another symbol whose function definition is used, etc.),
 a cons (STRING . DEFN), meaning that DEFN is the definition
    (DEFN should be a valid definition in its own right) and
    STRING is the menu item name (which is used only if the containing
    keymap has been created with a menu name, see `make-keymap'),
 or a cons (MAP . CHAR), meaning use definition of CHAR in keymap MAP,
 or an extended menu item definition.
 (See info node `(elisp)Extended Menu Items'.)

If REMOVE is non-nil, the definition will be removed.  This is almost
the same as setting the definition to nil, but makes a difference if
the KEYMAP has a parent, and KEY is shadowing the same binding in the
parent.  With REMOVE, subsequent lookups will return the binding in
the parent, and with a nil DEF, the lookups will return nil.

If KEYMAP is a sparse keymap with a binding for KEY, the existing
binding is altered.  If there is no binding for KEY, the new pair
binding KEY to DEF is added at the front of KEYMAP.]]}
function F.define_key.f(keymap,key,def,remove)
    keymap=get_keymap(keymap,true,true)
    local length=lisp.check_vector_or_string(key)
    if length==0 then
        return vars.Qnil
    end
    local meta_bit=(lisp.vectorp(key) or (lisp.stringp(key) and lisp.string_multibyte(key))) and b.CHAR_META or 0x80
    if lisp.vectorp(def) then
        error('TODO')
    end
    key=possibly_translate_key_sequence(key)
    local idx=0
    local metized=false
    while true do
        local c=vars.F.aref(key,lisp.make_fixnum(idx))
        if lisp.consp(c) then
            error('TODO')
        end
        if lisp.symbolp(c) then
            silly_event_symbol_error(c)
        end
        if lisp.fixnump(c) and bit.band(lisp.fixnum(c),meta_bit)>0 and not metized then
            c=vars.V.meta_prefix_char
            metized=true
        else
            if lisp.fixnump(c) then
                c=lisp.make_fixnum(bit.band(lisp.fixnum(c),bit.bnot(meta_bit)))
            end
            metized=false
            idx=idx+1
        end
        if not lisp.fixnump(c) and not lisp.symbolp(c) and
            (not lisp.consp(c) or (lisp.fixnump(lisp.xcar(c)) and idx~=length)) then
            error('TODO')
        end
        if idx==length then
            return store_in_keymap(keymap,c,def,not lisp.nilp(remove))
        end
        local cmd=access_keymap(keymap,c,false,true,true)
        if lisp.nilp(cmd) then
            cmd=define_as_prefix(keymap,c)
        end
        keymap=get_keymap(cmd,false,true)
        if not lisp.consp(keymap) then
            error('TODO')
        end
    end
end
F.make_sparse_keymap={'make-sparse-keymap',0,1,0,[[Construct and return a new sparse keymap.
Its car is `keymap' and its cdr is an alist of (CHAR . DEFINITION),
which binds the character CHAR to DEFINITION, or (SYMBOL . DEFINITION),
which binds the function key or mouse event SYMBOL to DEFINITION.
Initially the alist is nil.

The optional arg STRING supplies a menu name for the keymap
in case you use it as a menu with `x-popup-menu'.]]}
function F.make_sparse_keymap.f(s)
    if not lisp.nilp(s) then
        return lisp.list(vars.Qkeymap,s)
    end
    return lisp.list(vars.Qkeymap)
end
F.use_global_map={'use-global-map',1,1,0,[[Select KEYMAP as the global keymap.]]}
function F.use_global_map.f(keymap)
    keymap=get_keymap(keymap,true,true)
    if not _G.nelisp_later then
        error('TODO')
    end
    return vars.Qnil
end
local function keymap_parent(keymap,autoload)
    keymap=get_keymap(keymap,true,autoload)
    local list=lisp.xcdr(keymap)
    while lisp.consp(list) do
        if keymapp(list) then
            return list
        end
        list=lisp.xcdr(list)
    end
    return get_keymap(list,false,autoload)
end
local function keymap_memberp(map,maps)
    if lisp.nilp(map) then return false end
    while keymapp(maps) and not lisp.eq(map,maps) do
        maps=keymap_parent(maps,false)
    end
    return lisp.eq(map,maps)
end
F.set_keymap_parent={'set-keymap-parent',2,2,0,[[Modify KEYMAP to set its parent map to PARENT.
Return PARENT.  PARENT should be nil or another keymap.]]}
function F.set_keymap_parent.f(keymap,parent)
    keymap=get_keymap(keymap,true,true)
    if not lisp.nilp(parent) then
        parent=get_keymap(parent,true,false)
        if keymap_memberp(keymap,parent) then
            signal.error('Cyclic keymap inheritance')
        end
    end
    local prev=keymap
    while true do
        local list=lisp.xcdr(prev)
        if not lisp.consp(list) or keymapp(list) then
            lisp.xsetcdr(prev,parent)
            return parent
        end
        prev=list
    end
end

function M.init()
    vars.F.put(vars.Qkeymap,vars.Qchar_table_extra_slots,lisp.make_fixnum(0))

    vars.modifier_symbols={}
    local lread=require'nelisp.lread'
    for _,v in ipairs({'up','dow','drag','click','double','triple',0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,'alt','super','hyper','shift','control','meta'}) do
        if v==0 then
            table.insert(vars.modifier_symbols,0)
        else
            local sym=lisp.make_empty_ptr(lisp.type.symbol)
            lread.define_symbol(sym,v)
            table.insert(vars.modifier_symbols,sym)
        end
    end

    vars.V.minibuffer_local_map=vars.F.make_sparse_keymap(vars.Qnil)
end

function M.init_syms()
    vars.defsubr(F,'make_keymap')
    vars.defsubr(F,'define_key')
    vars.defsubr(F,'make_sparse_keymap')
    vars.defsubr(F,'use_global_map')
    vars.defsubr(F,'set_keymap_parent')

    vars.defvar_lisp('minibuffer_local_map','minibuffer-local-map',[[Default keymap to use when reading from the minibuffer.]])

    vars.defsym('Qkeymap','keymap')
    vars.defsym('Qmenu_item','menu-item')
    vars.defsym('Qevent_symbol_element_mask','event-symbol-element-mask')
    vars.defsym('Qevent_symbol_elements','event-symbol-elements')
    vars.defsym('Qmodifier_cache','modifier-cache')
    vars.defsym('Qevent_kind','event-kind')

    vars.defvar_lisp('meta_prefix_char','meta-prefix-char',[[Meta-prefix character code.
Meta-foo as command input turns into this character followed by foo.]])
    vars.V.meta_prefix_char=lisp.make_fixnum(27)
end
return M
