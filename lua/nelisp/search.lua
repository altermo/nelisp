local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local lread=require'nelisp.lread'
local b=require'nelisp.bytes'
local print_=require'nelisp.print'
local signal=require'nelisp.signal'
local alloc=require'nelisp.alloc'
local M={}
local function at_endline_loc_p(...)
    if not _G.nelisp_later then
        error('TODO')
    end
    return false
end
local search_regs={start={},end_={}}
local last_search_thing=vars.Qnil
---@param s nelisp.obj
---@return string
---@return table
local function eregex_to_vimregex(s)
    local signal_err=function (msg)
        signal.xsignal(vars.Qinvalid_regexp,alloc.make_string(msg))
    end
    if not _G.nelisp_later then
        error('TODO: signal error on bad pattern')
    end
    local data={}
    local in_buf=lread.make_readcharfun(s,0)
    local out_buf=print_.make_printcharfun()
    local parens=0 --vim doesn't have a way to get the position of a sub-match, so this is the current workaround
    local depth=0
    while true do
        local c=in_buf.read()
        if c==-1 then
            break
        end
        if c==b'\\' then
            c=in_buf.read()
            if c==-1 then
                signal_err('Trailing backslash')
            end
            if c==b'(' then
                if not _G.nelisp_later then
                    error('TODO: signal error on bad capture')
                end
                c=in_buf.read()
                in_buf.unread()
                if c~=-1 then
                    if c==b'?' then
                        error('TODO')
                    end
                end
                depth=depth+1
                do
                    parens=parens+1
                    out_buf.write('\\)')
                    data.sub_patterns=(data.sub_patterns or 0)+1
                end
                out_buf.write('\\(')
            elseif c==b')' then
                if depth==0 then
                    signal_err('Unmatched ) or \\)')
                end
                if depth>1 then
                    error('TODO')
                end
                depth=depth-1
                out_buf.write('\\)')
            elseif c==b'|' then
                error('TODO')
            elseif c==b'{' then
                error('TODO')
            elseif c==b'=' then
                error('TODO')
            elseif c==b's' then
                error('TODO')
            elseif c==b'S' then
                error('TODO')
            elseif c==b'c' then
                error('TODO')
            elseif c==b'C' then
                error('TODO')
            elseif c==b'w' then
                error('TODO')
            elseif c==b'W' then
                error('TODO')
            elseif c==b'<' then
                error('TODO')
            elseif c==b'>' then
                error('TODO')
            elseif c==b'_' then
                error('TODO')
            elseif c==b'b' then
                error('TODO')
            elseif c==b'B' then
                error('TODO')
            elseif c==b'`' then
                error('TODO')
            elseif c==b"'" then
                error('TODO')
            elseif string.char(c):match('[1-9]') then
                error('TODO')
            elseif c==b'\\' then
                out_buf.write('\\\\')
            else
                out_buf.write(c)
            end
        elseif c==b'^' then
            if not (in_buf.idx==0 or at_endline_loc_p(in_buf,in_buf.idx)) then
                goto normal_char
            end
            data.start_match=true
            out_buf.write('\\^')
        elseif c==b' ' then
            error('TODO')
        elseif c==b'$' then
            error('TODO')
        elseif c==b'+' or c==b'*' or c==b'?' then
            if not _G.nelisp_later then
                error('TODO: if previous expression is not valid the treat it as a literal')
            end
            out_buf.write('\\')
            out_buf.write(c)
        elseif c==b'.' then
            error('TODO')
        elseif c==b'[' then
            out_buf.write('\\[')
            local p=print_.make_printcharfun()
            c=in_buf.read()
            if c==-1 then
                signal_err('Unmatched [ or [^')
            end
            if c==b'^' then
                out_buf.write('^')
                c=in_buf.read()
            end
            if c==b']' then
                out_buf.write(']')
                c=in_buf.read()
            end
            while c~=b']' do
                p.write(c)
                if c==-1 then
                    signal_err('Unmatched [ or [^')
                end
                c=in_buf.read()
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
            out_buf.write(pat)
        else
            goto normal_char
        end
        goto continue
        ::normal_char::
        out_buf.write(c)
        ::continue::
    end
    return '\\V'..('\\('):rep(parens)..out_buf.out(),data
end

local F={}
local function string_match_1(regexp,s,start,posix,modify_data)
    lisp.check_string(regexp)
    lisp.check_string(s)
    if not _G.nelisp_later then
        error('TODO')
    end
    local pos_bytes=0
    if not lisp.nilp(start) then
        local len=lisp.schars(s)
        lisp.check_fixnum(start)
        local pos=lisp.fixnum(start)
        if pos<0 and -pos<=len then
            pos=len+pos
        elseif pos>0 and pos>len then
            signal.args_out_of_range(s,start)
        end
        if lisp.string_multibyte(s) then
            error('TODO')
        else
            pos_bytes=pos
        end
    end
    local vregex,data=eregex_to_vimregex(regexp)
    if data.start_match and pos_bytes>0 then
        return vars.Qnil
    end
    local _,pat_start,pat_end=unpack(vim.fn.matchstrpos(lisp.sdata(s),vregex,pos_bytes))
    if not _G.nelisp_later then
        error('TODO: somehow also return the positions of the submatches (or nil if they didn\'t match)')
    end
    if start==-1 or pat_end==-1 then
        return vars.Qnil
    end
    search_regs={
        start={pat_start},
        end_={pat_end},
    }
    if data.sub_patterns then
        local idx=2
        local list=vim.fn.matchlist(lisp.sdata(s),vregex,pos_bytes)
        local sub_patterns=data.sub_patterns
        while idx<=data.sub_patterns+1 do
            local sub_start=#list[idx]
            local sub_end=#list[idx+sub_patterns]+sub_start
            if list[idx+sub_patterns]=='' then
                table.insert(search_regs.start,vars.Qnil)
                table.insert(search_regs.end_,vars.Qnil)
            else
                table.insert(search_regs.start,sub_start)
                table.insert(search_regs.end_,sub_end)
            end
            idx=idx+1
        end
    end
    last_search_thing=vars.Qt
    return lisp.make_fixnum(pat_start)
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
F.match_data={'match-data',0,3,0,[[Return a list of positions that record text matched by the last search.
Element 2N of the returned list is the position of the beginning of the
match of the Nth subexpression; it corresponds to `(match-beginning N)';
element 2N + 1 is the position of the end of the match of the Nth
subexpression; it corresponds to `(match-end N)'.  See `match-beginning'
and `match-end'.
If the last search was on a buffer, all the elements are by default
markers or nil (nil when the Nth pair didn't match); they are integers
or nil if the search was on a string.  But if the optional argument
INTEGERS is non-nil, the elements that represent buffer positions are
always integers, not markers, and (if the search was on a buffer) the
buffer itself is appended to the list as one additional element.

Use `set-match-data' to reinstate the match data from the elements of
this list.

Note that non-matching optional groups at the end of the regexp are
elided instead of being represented with two `nil's each.  For instance:

  (progn
    (string-match "^\\(a\\)?\\(b\\)\\(c\\)?$" "b")
    (match-data))
  => (0 1 nil nil 0 1)

If REUSE is a list, store the value in REUSE by destructively modifying it.
If REUSE is long enough to hold all the values, its length remains the
same, and any unused elements are set to nil.  If REUSE is not long
enough, it is extended.  Note that if REUSE is long enough and INTEGERS
is non-nil, no consing is done to make the return value; this minimizes GC.

If optional third argument RESEAT is non-nil, any previous markers on the
REUSE list will be modified to point to nowhere.

Return value is undefined if the last search failed.]]}
function F.match_data.f(integers,reuse,reseat)
    if not lisp.nilp(reseat) then
        error('TODO')
    end
    if lisp.nilp(last_search_thing) then
        return vars.Qnil
    end
    local data={}
    if #search_regs.start~=1 then
        error('TODO')
    else
        if lisp.eq(last_search_thing,vars.Qt) then
            if search_regs.start[1]==-1 then
                data[1]=vars.Qnil
                data[2]=vars.Qnil
            else
                data[1]=lisp.make_fixnum(search_regs.start[1])
                data[2]=lisp.make_fixnum(search_regs.end_[1])
            end
        else
            error('TODO')
        end
    end
    if not lisp.consp(reuse) then
        reuse=vars.F.list(data)
    else
        error('TODO')
    end
    return reuse
end
F.set_match_data={'set-match-data',1,2,0,[[Set internal data on last search match from elements of LIST.
LIST should have been created by calling `match-data' previously.

If optional arg RESEAT is non-nil, make markers on LIST point nowhere.]]}
function F.set_match_data.f(list,reseat)
    lisp.check_list(list)
    local length=lisp.list_length(list)/2
    last_search_thing=vars.Qt
    local num_regs=search_regs and #search_regs.start or 0
    local i=0
    while lisp.consp(list) do
        local marker=lisp.xcar(list)
        if lisp.bufferp(marker) then
            error('TODO')
        end
        if i>=length then
            break
        end
        if lisp.nilp(marker) then
            search_regs.start[i+1]=-1
            list=lisp.xcdr(list)
        else
            if lisp.markerp(marker) then
                error('TODO')
            end
            local form=marker
            if not lisp.nilp(reseat) and lisp.markerp(marker) then
                error('TODO')
            end
            list=lisp.xcdr(list)
            if not lisp.consp(list) then
                break
            end
            marker=lisp.xcar(list)
            if lisp.markerp(marker) then
                error('TODO')
            end
            search_regs.start[i+1]=lisp.fixnum(form)
            search_regs.end_[i+1]=lisp.fixnum(marker)
        end
        list=lisp.xcdr(list)
        i=i+1
    end
    while i<num_regs do
        search_regs.start[i+1]=nil
        search_regs.end_[i+1]=nil
        i=i+1
    end
    return vars.Qnil
end
local function match_limit(num,beginning)
    lisp.check_fixnum(num)
    local n=lisp.fixnum(num)
    if n<0 then
        signal.args_out_of_range(num,lisp.make_fixnum(0))
    end
    if #search_regs.start<=0 then
        signal.error('No match data, because no search succeeded')
    end
    if n>=#search_regs.start or search_regs.start[n+1]<0 then
        return vars.Qnil
    end
    return lisp.make_fixnum(beginning and search_regs.start[n+1] or search_regs.end_[n+1])
end
F.match_beginning={'match-beginning',1,1,0,[[Return position of start of text matched by last search.
SUBEXP, a number, specifies which parenthesized expression in the last
  regexp.
Value is nil if SUBEXPth pair didn't match, or there were less than
  SUBEXP pairs.
Zero means the entire text matched by the whole regexp or whole string.

Return value is undefined if the last search failed.]]}
function F.match_beginning.f(subexp)
    return match_limit(subexp,true)
end
F.match_end={'match-end',1,1,0,[[Return position of end of text matched by last search.
SUBEXP, a number, specifies which parenthesized expression in the last
  regexp.
Value is nil if SUBEXPth pair didn't match, or there were less than
  SUBEXP pairs.
Zero means the entire text matched by the whole regexp or whole string.

Return value is undefined if the last search failed.]]}
function F.match_end.f(subexp)
    return match_limit(subexp,false)
end

function M.init_syms()
    vars.defsubr(F,'string_match')
    vars.defsubr(F,'match_data')
    vars.defsubr(F,'set_match_data')
    vars.defsubr(F,'match_beginning')
    vars.defsubr(F,'match_end')
end
return M
