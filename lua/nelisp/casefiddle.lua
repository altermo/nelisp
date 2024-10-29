local vars=require'nelisp.vars'
local alloc=require'nelisp.alloc'
local lisp=require'nelisp.lisp'
local M={}

local F={}
F.capitalize={'capitalize',1,1,0,[[Convert argument to capitalized form and return that.
This means that each word's first character is converted to either
title case or upper case, and the rest to lower case.

The argument may be a character or string.  The result has the same
type.  (See `downcase' for further details about the type.)

The argument object is not altered--the value is a copy.  If argument
is a character, characters which map to multiple code points when
cased, e.g. ﬁ, are returned unchanged.]]}
function F.capitalize.f(arg)
    if not _G.nelisp_later then
        error('TODO')
    end
    return alloc.make_string(vim.fn.toupper(lisp.sdata(arg)))
end
function M.init_syms()
    vars.defsubr(F,'capitalize')
end
return M
