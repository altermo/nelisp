local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'

local M={}

---@type nelisp.F
local F={}
F.assoc_string={'assoc-string',2,3,0,[[Like `assoc' but specifically for strings (and symbols).

This returns the first element of LIST whose car matches the string or
symbol KEY, or nil if no match exists.  When performing the
comparison, symbols are first converted to strings, and unibyte
strings to multibyte.  If the optional arg CASE-FOLD is non-nil, both
KEY and the elements of LIST are upcased for comparison.

Unlike `assoc', KEY can also match an entry in LIST consisting of a
single string, rather than a cons cell whose car is a string.]]}
function F.assoc_string.f(key,list,case_fold)
    if lisp.symbolp(key) then
        key=vars.F.symbol_name(key)
    end
    while lisp.consp(list) do
        local tem
        local elt=lisp.xcar(list)
        local thiscar=lisp.consp(elt) and lisp.xcar(elt) or elt
        if lisp.symbolp(thiscar) then
            thiscar=vars.F.symbol_name(thiscar)
        elseif not lisp.stringp(thiscar) then
            goto continue
        end
        tem=vars.F.compare_strings(thiscar,lisp.make_fixnum(0),vars.Qnil,key,lisp.make_fixnum(0),vars.Qnil,case_fold)
        if lisp.eq(tem,vars.Qt) then
            return elt
        end
        ::continue::
        list=lisp.xcdr(list)
    end
    return vars.Qnil
end

function M.init()
    vars.V.minibuffer_prompt_properties=lisp.list(vars.Qread_only,vars.Qt)
end
function M.init_syms()
    vars.defsubr(F,'assoc_string')

    vars.defvar_lisp('minibuffer_prompt_properties','minibuffer-prompt-properties',[[Text properties that are added to minibuffer prompts.
These are in addition to the basic `field' property, and stickiness
properties.]])

    vars.defsym('Qread_only','read-only')
end
return M
