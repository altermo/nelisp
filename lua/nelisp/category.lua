local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local alloc=require'nelisp.alloc'
local signal=require'nelisp.signal'
local nvim=require'nelisp.nvim'
local b=require'nelisp.bytes'

local M={}

---@type nelisp.F
local F={}
local function categoryp(c)
    return lisp.ranged_fixnump(0x20,c,0x7e)
end
local function check_category(c)
    lisp.check_type(categoryp(c),vars.Qcategoryp,c)
end
local function check_category_table(tbl)
    if lisp.nilp(tbl) then
        return nvim.buffer_category_table(nvim.get_current_buffer() --[[@as nelisp._buffer]])
    end
    error('TODO')
end
local function category_docstring(tbl,c)
    return lisp.aref(vars.F.char_table_extra_slot(tbl,lisp.make_fixnum(0)),c-b' ')
end
local function set_category_docstring(tbl,c,docstring)
    lisp.aset(vars.F.char_table_extra_slot(tbl,lisp.make_fixnum(0)),c-b' ',docstring)
end
F.define_category={'define-category',2,3,0,[[Define CATEGORY as a category which is described by DOCSTRING.
CATEGORY should be an ASCII printing character in the range ` ' to `~'.
DOCSTRING is the documentation string of the category.  The first line
should be a terse text (preferably less than 16 characters),
and the rest lines should be the full description.
The category is defined only in category table TABLE, which defaults to
the current buffer's category table.]]}
function F.define_category.f(category,docstring,tbl)
    check_category(category)
    lisp.check_string(docstring)
    tbl=check_category_table(tbl)
    if not lisp.nilp(category_docstring(tbl,lisp.fixnum(category))) then
        signal.error("Category `%c' is already defined",lisp.fixnum(category))
    end
    set_category_docstring(tbl,lisp.fixnum(category),docstring)
    return vars.Qnil
end

function M.init()
    vars.F.put(vars.Qcategory_table,vars.Qchar_table_extra_slots,lisp.make_fixnum(2))
    vars.standard_category_table=vars.F.make_char_table(vars.Qcategory_table,vars.Qnil)
    vars.standard_category_table --[[@as nelisp._char_table]].default=vars.F.make_bool_vector(lisp.make_fixnum(128),vars.Qnil)
    vars.F.set_char_table_extra_slot(vars.standard_category_table,lisp.make_fixnum(0),alloc.make_vector(95,'nil'))
end
function M.init_syms()
    vars.defsubr(F,'define_category')

    vars.defsym('Qcategory_table','category-table')
    vars.defsym('Qcategoryp','categoryp')
end
return M
