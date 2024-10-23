local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local signal=require'nelisp.signal'
local alloc=require'nelisp.alloc'

local M={}
local CHARTAB_SIZE_BITS_0=bit.lshift(1,6)
---@param ct nelisp.obj
---@param idx number
---@param val nelisp.obj
function M.set(ct,idx,val)
    if not _G.nelisp_later then
        error('TODO')
    end
end
---@param ct nelisp.obj
---@param idx number
---@return nelisp.obj
function M.ref(ct,idx)
    if not _G.nelisp_later then
        error('TODO')
    end
    return vars.Qnil
end

local F={}
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
    local size=CHARTAB_SIZE_BITS_0+n_extras
    local vector=alloc.make_vector(size,init) --[[@as nelisp._char_table]]
    vector.ascii=vars.Qnil
    vector.default=vars.Qnil
    vector.parents=vars.Qnil
    vector.purpose=purpose
    return lisp.make_vectorlike_ptr(vector,lisp.pvec.char_table)

end

function M.init_syms()
    vars.defsubr(F,'make_char_table')
end
return M
