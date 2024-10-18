local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local str=require'nelisp.obj.str'
local fixnum=require'nelisp.obj.fixnum'
local M={}

local F={}
local function string_match_1(regexp,s,start,posix,modify_data)
    lisp.check_string(regexp)
    lisp.check_string(s)
    if not _G.nelisp_later then
        error('TODO')
    end
    if not lisp.nilp(start) then
        error('TODO')
    end
    regexp=str.make('^[0-9]+\\.\\([0-9]+\\)',false)
    local reg=lisp.sdata(regexp)
    reg=reg:gsub('\\?%(',function(a) return a=='(' and '\\(' or '(' end)
    reg=reg:gsub('\\?%)',function (a) return a==')' and '\\)' or ')' end)
    local idx=vim.fn.match(lisp.sdata(s),'\\v'..reg)
    return idx==-1 and vars.Qnil or fixnum.make(idx)
end
F.string_match={'string-match',2,4,0,[[Return index of start of first match for REGEXP in STRING, or nil.
Matching ignores case if `case-fold-search' is non-nil.
If third arg START is non-nil, start search at that index in STRING.

If INHIBIT-MODIFY is non-nil, match data is not changed.

If INHIBIT-MODIFY is nil or missing, match data is changed, and
`match-end' and `match-beginning' give indices of substrings matched
by parenthesis constructs in the pattern.  You can use the function
`match-string' to extract the substrings matched by the parenthesis
constructions in REGEXP.  For index of first char beyond the match, do
(match-end 0).]]}
function F.string_match.f(regexp,s,start,inhibit_modify)
    return string_match_1(regexp,s,start,false,lisp.nilp(inhibit_modify))
end

function M.init_syms()
    vars.setsubr(F,'string_match')
end
return M
