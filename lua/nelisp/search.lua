local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local str=require'nelisp.obj.str'
local fixnum=require'nelisp.obj.fixnum'
local lread=require'nelisp.lread'
local b=require'nelisp.bytes'
local print_=require'nelisp.print'
local signal=require'nelisp.signal'
local M={}
local function at_endline_loc_p(...)
    if not _G.nelisp_later then
        error('TODO')
    end
    return false
end
---@param s nelisp.str
---@return string
local function eregex_to_vimregex(s)
    if not _G.nelisp_later then
        error('TODO: signal error on bad pattern')
    end
    local buf=lread.make_readcharfun(s,0)
    local ret_buf=print_.make_printcharfun()
    ret_buf.write('\\V')
    while true do
        local c=buf.read()
        if c==-1 then
            break
        end
        if c==b'\\' then
            c=buf.read()
            if c==-1 then
                signal.xsignal(vars.Qinvalid_regexp,str.make('Trailing backslash','auto'))
            end
            error('TODO')
        elseif c==b'^' then
            if not (buf.idx==1 or at_endline_loc_p(buf,buf.idx)) then
                goto normal_char
            end
            ret_buf.write('\\^')
        elseif c==b' ' then
            error('TODO')
        elseif c==b'$' then
            error('TODO')
        elseif c==b'+' or c==b'*' or c==b'?' then
            error('TODO')
        elseif c==b'.' then
            error('TODO')
        elseif c==b'[' then
            ret_buf.write('\\[')
            local p=print_.make_printcharfun()
            c=buf.read()
            if c==b'^' then
                ret_buf.write('^')
                c=buf.read()
            end
            if c==b']' then
                ret_buf.write(']')
                c=buf.read()
            end
            while c~=b']' do
                p.write(c)
                if c==-1 then
                    signal.xsignal(vars.Qinvalid_regexp,'Unmatched [ or [^')
                end
                c=buf.read()
            end
            p.write(c)
            local pat=p.out()
            if pat:find('[:word:]',1,true) then
                error('TODO')
            elseif pat:find('[:ascii:]',1,true) then
                error('TODO')
            elseif pat:find('[:nonascii:]',1,true) then
                error('TODO')
            elseif pat:find('[:ff:]',1,true) then
                error('TODO')
            elseif pat:find('[:return:]',1,true) then
                pat=pat:gsub('%[:return:%]',':return:[]')
            elseif pat:find('[:tab:]',1,true) then
                pat=pat:gsub('%[:tab:%]',':tab:[]')
            elseif pat:find('[:escape:]',1,true) then
                pat=pat:gsub('%[:escape:%]',':escape:[]')
            elseif pat:find('[:backspace:]',1,true) then
                pat=pat:gsub('%[:backspace:%]',':backspace:[]')
            elseif pat:find('[:ident:]',1,true) then
                pat=pat:gsub('%[:ident:%]',':ident:[]')
            elseif pat:find('[:keyword:]',1,true) then
                pat=pat:gsub('%[:keyword:%]',':keyword:[]')
            elseif pat:find('[:fname:]',1,true) then
                pat=pat:gsub('%[:fname:%]',':fname:[]')
            end
            ret_buf.write(pat)
        else
            goto normal_char
        end
        goto continue
        ::normal_char::
        error('TODO: '..string.char(c))
        ::continue::
    end
    return ret_buf.out()
end

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
    local idx=vim.fn.match(lisp.sdata(s),eregex_to_vimregex(regexp))
    vim.print(eregex_to_vimregex(regexp))
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
