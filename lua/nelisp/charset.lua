local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local signal=require'nelisp.signal'

---@enum
local idx={
    charset_id=0,
    charset_name=1,
    charset_plist=2,
    charset_map=3,
    charset_decoder=4,
    charset_encoder=5,
    charset_subset=6,
    charset_superset=7,
    charset_unify_map=8,
    charset_deunifier=9,
    charset_attr_max=10
}

local M={}

local F={}
local function charset_symbol_attributes(sym)
    return vars.F.gethash(sym,vars.charset_hash_table,vars.Qnil)
end
local function check_charset_get_attr(x)
    if not lisp.symbolp(x) then
        signal.wrong_type_argument(vars.Qcharsetp,x)
    end
    local attr=charset_symbol_attributes(x)
    if lisp.nilp(attr) then
        signal.wrong_type_argument(vars.Qcharsetp,x)
    end
    return attr
end
F.set_charset_plist={'set-charset-plist',2,2,0,[[Set CHARSET's property list to PLIST.]]}
function F.set_charset_plist.f(charset,plist)
    local attrs=check_charset_get_attr(charset)
    lisp.aset(attrs,idx.charset_plist,plist)
    return plist
end

function M.init_syms()
    vars.defsubr(F,'set_charset_plist')

    vars.defsym('Qemacs','emacs')
end
return M
