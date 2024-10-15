local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local fixnum=require'nelisp.obj.fixnum'
local signal=require'nelisp.signal'
local char_table=require'nelisp.obj.char_table'

local M={}

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
    if lisp.nilp(n) then
        n=0
    else
        lisp.check_fixnat(n)
        if fixnum.tonumber(n)>10 then
            signal.args_out_of_range(n,vars.Qnil)
        end
        n=fixnum.tonumber(n)
    end
    return char_table.make(purpose,n,init,nil)
end

function M.init_syms()
    vars.setsubr(F,'make_char_table')
end
return M
