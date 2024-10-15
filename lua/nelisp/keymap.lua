local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local fixnum=require'nelisp.obj.fixnum'
local signal=require'nelisp.signal'
local str=require'nelisp.obj.str'
local b=require'nelisp.bytes'
local M={}

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
    error('TODO')
end
local function possibly_translate_key_sequence(key)
    if lisp.vectorp(key) then
        error('TODO')
    end
    return key
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
        error('TODO')
    elseif lisp.fixnump(idx) then
        idx=fixnum.make(bit.band(fixnum.tonumber(idx --[[@as nelisp.fixnum]]),bit.bor(b.CHAR_META,b.CHAR_META-1)))
    end
    local tail=lisp.xcdr(keymap)
    while lisp.consp(tail) do
        ---@cast tail nelisp.cons
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
            if lisp.fixnatp(idx) and bit.band(fixnum.tonumber(idx --[[@as nelisp.fixnum]]),b.CHAR_MODIFIER_MASK)==0 then
                vars.F.aset(elt,idx,sdef)
                return def
            end
            error('TODO')
        else
            error('TODO')
        end
        tail=lisp.xcdr(tail)
    end
    error('TODO')
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
    local meta_bit=(lisp.vectorp(key) or (lisp.stringp(key) and str.is_multibyte(key))) and b.CHAR_META or 0x80
    if lisp.vectorp(def) then
        error('TODO')
    end
    key=possibly_translate_key_sequence(key)
    local idx=0
    local metized=false
    while true do
        local c=vars.F.aref(key,fixnum.make(idx))
        if lisp.consp(c) then
            error('TODO')
        end
        if lisp.symbolp(c) then
            error('TODO')
        end
        if lisp.fixnump(c) and bit.band(fixnum.tonumber(c),meta_bit)>0 and not metized then
            error('TODO')
        else
            if lisp.fixnump(c) then
                c=fixnum.make(bit.band(fixnum.tonumber(c),bit.bnot(meta_bit)))
            end
            metized=false
            idx=idx+1
        end
        if not lisp.fixnump(c) and not lisp.symbolp(c) and
            (not lisp.consp(c) or (lisp.fixnump(lisp.xcar(c --[[@as nelisp.cons]])) and idx~=length)) then
            error('TODO')
        end
        if idx==length then
            return store_in_keymap(keymap,c,def,not lisp.nilp(remove))
        end
        error('TODO')
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

function M.init()
    vars.F.put(vars.Qkeymap,vars.Qchar_table_extra_slots,fixnum.zero)
end

function M.init_syms()
    vars.setsubr(F,'make_keymap')
    vars.setsubr(F,'define_key')
    vars.setsubr(F,'make_sparse_keymap')

    vars.defsym('Qkeymap','keymap')
end
return M
