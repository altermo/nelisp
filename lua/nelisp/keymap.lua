local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local fixnum=require'nelisp.obj.fixnum'
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

function M.init()
    vars.F.put(vars.Qkeymap,vars.Qchar_table_extra_slots,fixnum.zero)
end

function M.init_syms()
    vars.setsubr(F,'make_keymap')

    vars.defsym('Qkeymap','keymap')
end
return M
