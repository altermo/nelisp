local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local alloc=require'nelisp.alloc'

local M={}

function M.init()
    vars.F.put(vars.Qcategory_table,vars.Qchar_table_extra_slots,lisp.make_fixnum(2))
    vars.standard_category_table=vars.F.make_char_table(vars.Qcategory_table,vars.Qnil)
    vars.standard_category_table --[[@as nelisp._char_table]].default=vars.F.make_bool_vector(lisp.make_fixnum(128),vars.Qnil)
    vars.F.set_char_table_extra_slot(vars.standard_category_table,lisp.make_fixnum(0),alloc.make_vector(95,'nil'))
end
function M.init_syms()
    vars.defsym('Qcategory_table','category-table')
end
return M
