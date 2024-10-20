local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'
local str=require'nelisp.obj.str'
local print_=require'nelisp.print'
local b=require'nelisp.bytes'
local signal=require'nelisp.signal'
local symbol=require'nelisp.obj.symbol'

local M={}

local F={}
local function styled_format(args,message)
    lisp.check_string(args[1])
    local formatlen=lisp.sbytes(args[1])
    local idx=1
    local argsidx=2
    local buf=print_.make_printcharfun()
    local multibyte_format=str.is_multibyte(args[1])
    local multibyte=multibyte_format
    local fmt_props=not not str.get_intervals(args[1])
    local arg_intervals=false
    local n=0
    for i=2,#args do
        if multibyte then break end
        if lisp.stringp(args[i]) and str.is_multibyte(args[i]) then
            multibyte=true
        end
    end
    while idx<=formatlen do
        local format_char=str.index1(args[1],idx)
        idx=idx+1
        if format_char==b'%' then
            local c=str.index1_neg(args[1],idx)
            local num=0
            local num_end
            if string.match('%d',string.char(c)) then
                error('TODO')
            end
            local flags={}
            idx=idx-1
            while true do
                idx=idx+1
                c=str.index1_neg(args[1],idx)
                if c==b'-' then flags.minus=true
                elseif c==b'+' then flags.plus=true
                elseif c==b' ' then flags.space=true
                elseif c==b'#' then flags.sharp=true
                elseif c==b'0' then flags.zero=true
                else break end
            end
            if not flags.plus then flags.space=nil end
            if not flags.minus then flags.zero=nil end
            num=0
            local field_width=num
            local precision_given=num_end==b'.'
            local precision=precision_given and error('TODO') or math.huge
            if idx>formatlen then
                signal.error('Format string ends in middle of format specifier')
            end
            c=str.index1_neg(args[1],idx)
            idx=idx+1
            if c==b'%' then
                error('TODO')
            end
            n=n+1
            if n>#args then
                signal.error('Not enough arguments for format string')
            end
            local arg=args[argsidx]
            argsidx=argsidx+1
            if c==b'S' or (c==b's' and not lisp.stringp(arg) and not lisp.symbolp(arg)) then
                local noescape=c==b'S' and vars.Qnil or vars.Qt
                arg=vars.F.prin1_to_string(arg,noescape,vars.Qnil)
                if str.is_multibyte(arg) then
                    error('TODO')
                end
                c=b's'
            elseif c==b'c' then
                error('TODO')
            end
            if lisp.symbolp(arg) then
                arg=symbol.get_name(arg)
                if str.is_multibyte(arg) then
                    error('TODO')
                end
            end
            local float_conversion=c==b'e' or c==b'f' or c==b'g'
            if c==b's' then
                if formatlen==2 and idx-1==formatlen then
                    error('TODO')
                end
                local prec=-1
                if precision_given then
                    prec=precision
                end
                local nbytes,nchars_string,width
                if prec==0 then
                    error('TODO')
                else
                    if not _G.nelisp_later then
                        error('TODO')
                    end
                    nchars_string=lisp.schars(arg)
                    width=nchars_string
                    if prec==-1 then
                        nbytes=lisp.sbytes(arg)
                    else
                        error('TODO')
                    end
                end
                local convbytes=nbytes
                if convbytes>0 and multibyte and not str.is_multibyte(arg) then
                    error('TODO')
                end
                local padding=width<field_width and field_width-width or 0
                if fmt_props then
                    error('TODO')
                end
                if not flags.minus and padding>0 then
                    buf.write((' '):rep(padding))
                end
                if multibyte then
                    error('TODO: more checks and set maybe_combine_byte')
                end
                if str.is_multibyte(arg) then
                    error('TODO')
                end
                buf.write(lisp.sdata(arg))
                if flags.minus and padding>0 then
                    buf.write((' '):rep(padding))
                end
                if str.get_intervals(args[1]) then
                    error('TODO')
                end
            else
                error('TODO')
            end
        else
            if format_char==b'`' or format_char==b'\'' then
                error('TODO')
            elseif format_char==b'`' then
                error('TODO')
            else
                if multibyte_format then
                    error('TODO')
                elseif multibyte then
                    error('TODO')
                else
                    buf.write(format_char)
                end
            end
        end
    end
    local val=str.make(buf.out(),multibyte)
    if str.get_intervals(args[1]) or arg_intervals then
        error('TODO')
    end
    return val
end
F.format_message={'format-message',1,-2,0,[[Format a string out of a format-string and arguments.
The first argument is a format control string.
The other arguments are substituted into it to make the result, a string.

This acts like `format', except it also replaces each grave accent (\\=`)
by a left quote, and each apostrophe (\\=') by a right quote.  The left
and right quote replacement characters are specified by
`text-quoting-style'.

usage: (format-message STRING &rest OBJECTS)]]}
function F.format_message.f(args)
    return styled_format(args,true)
end
F.message={'message',1,-2,0,[[Display a message at the bottom of the screen.
The message also goes into the `*Messages*' buffer, if `message-log-max'
is non-nil.  (In keyboard macros, that's all it does.)
Return the message.

In batch mode, the message is printed to the standard error stream,
followed by a newline.

The first argument is a format control string, and the rest are data
to be formatted under control of the string.  Percent sign (%), grave
accent (\\=`) and apostrophe (\\=') are special in the format; see
`format-message' for details.  To display STRING without special
treatment, use (message "%s" STRING).

If the first argument is nil or the empty string, the function clears
any existing message; this lets the minibuffer contents show.  See
also `current-message'.

usage: (message FORMAT-STRING &rest ARGS)]]}
---@param args nelisp.obj[]
function F.message.f(args)
    if lisp.nilp(args[1]) or (lisp.stringp(args[1]) and str.index1_neg(args[1] --[[@as nelisp.str]],1)==-1) then
        error('TODO')
    end
    local val=vars.F.format_message(args)
    print(lisp.sdata(val))
    return val
end
F.system_name={'system-name',0,0,0,[[Return the host name of the machine you are running on, as a string.]]}
function F.system_name.f()
    if lisp.eq(vars.V.system_name,vars.cached_system_name) then
        local name=vim.fn.hostname():gsub('[\t ]','-')
        if name~=lisp.sdata(vars.V.system_name) then
            vars.V.system_name=str.make(name,'auto')
        end
        vars.cached_system_name=vars.V.system_name
    end
    return vars.V.system_name
end

function M.init_syms()
    vars.setsubr(F,'format_message')
    vars.setsubr(F,'message')
    vars.setsubr(F,'system_name')

    vars.defvar_lisp('system_name','system-name',[[The host name of the machine Emacs is running on.]])
    vars.V.system_name=vars.Qnil
    vars.cached_system_name=vars.Qnil
end
return M
