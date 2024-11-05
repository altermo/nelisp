local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'
local print_=require'nelisp.print'
local b=require'nelisp.bytes'
local signal=require'nelisp.signal'
local alloc=require'nelisp.alloc'
local overflow=require'nelisp.overflow'
local chars=require'nelisp.chars'

local M={}

local F={}
local function str2num(idx,s)
    ---@type number?
    local n=0
    while string.char(lisp.sref(s,idx)):match('%d') do
        n=overflow.add(overflow.mul(n,10),lisp.sref(s,idx)-b'0')
        if n==nil then
            n=overflow.max
        end
        idx=idx+1
    end
    return n,idx
end
local function styled_format(args,message)
    lisp.check_string(args[1])
    local formatlen=lisp.sbytes(args[1])
    local idx=0
    local argsidx=2
    local buf=print_.make_printcharfun()
    local multibyte_format=lisp.string_multibyte(args[1])
    local multibyte=multibyte_format
    local fmt_props=not not lisp.string_intervals(args[1])
    local arg_intervals=false
    local quoting_style=message and vars.F.text_quoting_style() or vars.Qnil
    local n=0
    for i=2,#args do
        if multibyte then break end
        if lisp.stringp(args[i]) and lisp.string_multibyte(args[i]) then
            multibyte=true
        end
    end
    while idx<formatlen do
        local format_char=lisp.sref(args[1],idx)
        idx=idx+1
        if format_char==b'%' then
            local c=lisp.sref(args[1],idx)
            local num=0
            local num_end
            if string.char(c):match('%d') then
                num,num_end=str2num(idx,args[1])
                if lisp.sref(args[1],num_end)==b'$' then
                    error('TODO')
                end
            end
            local flags={}
            idx=idx-1
            while true do
                idx=idx+1
                c=lisp.sref(args[1],idx)
                if c==b'-' then flags.minus=true
                elseif c==b'+' then flags.plus=true
                elseif c==b' ' then flags.space=true
                elseif c==b'#' then flags.sharp=true
                elseif c==b'0' then flags.zero=true
                else break end
            end
            if not flags.plus then flags.space=nil end
            if not flags.minus then flags.zero=nil end
            num,num_end=str2num(idx,args[1])
            local field_width=num
            local precision_given=lisp.sref(args[1],num_end)==b'.'
            local precision=precision_given and error('TODO') or math.huge
            idx=num_end
            if idx>=formatlen then
                signal.error('Format string ends in middle of format specifier')
            end
            c=lisp.sref(args[1],idx)
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
                if lisp.string_multibyte(arg) then
                    error('TODO')
                end
                c=b's'
            elseif c==b'c' then
                if lisp.fixnump(arg) and not chars.asciicharp(lisp.fixnum(arg)) then
                    error('TODO')
                end
                if not lisp.eq(arg,args[n+1]) then
                    c=b's'
                end
                flags.zero=false
            end
            if lisp.symbolp(arg) then
                arg=lisp.symbol_name(arg)
                if lisp.string_multibyte(arg) then
                    error('TODO')
                end
            end
            local float_conversion=c==b'e' or c==b'f' or c==b'g'
            if c==b's' then
                if formatlen==2 and idx==formatlen then
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
                if convbytes>0 and multibyte and not lisp.string_multibyte(arg) then
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
                if lisp.string_multibyte(arg) then
                    error('TODO')
                end
                buf.write(lisp.sdata(arg))
                if flags.minus and padding>0 then
                    buf.write((' '):rep(padding))
                end
                if lisp.string_intervals(args[1]) then
                    error('TODO')
                end
            elseif not (c==b'c' or c==b'd' or float_conversion or c==b'i' or c==b'o' or c==b'x' or c==b'X') then
                if multibyte_format then
                    signal.error('Invalid format operation %%%c',chars.stringcharandlength(lisp.sdata(args[1]):sub(idx)))
                elseif c<=127 then
                    signal.error('Invalid format operation %%%c',c)
                else
                    signal.error('Invalid format operation char #o%03o',c)
                end
            elseif not (lisp.fixnump(arg) or ((lisp.bignump(arg) or lisp.floatp(arg)) and c~=b'c')) then
                signal.error("Format specifier doesn't match argument type")
            else
                if c==b'd' or c==b'i' then
                    flags.sharp=false
                end
                local p
                local convspec='%'
                convspec=convspec..(flags.plus and '+' or '')
                convspec=convspec..(flags.space and ' ' or '')
                convspec=convspec..(flags.sharp and '#' or '')
                local prec=-1
                if precision_given then
                    prec=math.min(precision,16382)
                end
                if field_width then
                    convspec=convspec..field_width
                end
                convspec=convspec..'.*'
                if not (float_conversion or c==b'c') then
                    convspec=convspec..'l'
                    flags.zero=not precision_given and flags.zero
                end
                convspec=convspec..string.char(c)
                if float_conversion then
                    error('TODO')
                elseif c==b'c' then
                    p=string.char(lisp.fixnum(arg))
                elseif c==b'd' or c==b'i' then
                    if lisp.fixnump(arg) then
                        local x=lisp.fixnum(arg)
                        ---@diagnostic disable-next-line: redundant-parameter
                        p=vim.fn.printf(convspec,prec,x)
                    else
                        error('TODO')
                    end
                else
                    error('TODO')
                end
                if not _G.nelisp_later then
                    error('TODO')
                end
                buf.write(p)
            end
        else
            if (format_char==b'`' or format_char==b'\'') and lisp.eq(quoting_style,vars.Qcurve) then
                error('TODO')
            elseif format_char==b'`' and lisp.eq(quoting_style,vars.Qstraight) then
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
    local val=alloc.make_specified_string(buf.out(),-1,multibyte)
    if lisp.string_intervals(args[1]) or arg_intervals then
        error('TODO')
    end
    return val
end
F.format={'format',1,-2,0,[[Format a string out of a format-string and arguments.
The first argument is a format control string.
The other arguments are substituted into it to make the result, a string.

The format control string may contain %-sequences meaning to substitute
the next available argument, or the argument explicitly specified:

%s means produce a string argument.  Actually, produces any object with `princ'.
%d means produce as signed number in decimal.
%o means produce a number in octal.
%x means produce a number in hex.
%X is like %x, but uses upper case.
%e means produce a number in exponential notation.
%f means produce a number in decimal-point notation.
%g means produce a number in exponential notation if the exponent would be
   less than -4 or greater than or equal to the precision (default: 6);
   otherwise it produces in decimal-point notation.
%c means produce a number as a single character.
%S means produce any object as an s-expression (using `prin1').

The argument used for %d, %o, %x, %e, %f, %g or %c must be a number.
%o, %x, and %X treat arguments as unsigned if `binary-as-unsigned' is t
  (this is experimental; email 32252@debbugs.gnu.org if you need it).
Use %% to put a single % into the output.

A %-sequence other than %% may contain optional field number, flag,
width, and precision specifiers, as follows:

  %<field><flags><width><precision>character

where field is [0-9]+ followed by a literal dollar "$", flags is
[+ #0-]+, width is [0-9]+, and precision is a literal period "."
followed by [0-9]+.

If a %-sequence is numbered with a field with positive value N, the
Nth argument is substituted instead of the next one.  A format can
contain either numbered or unnumbered %-sequences but not both, except
that %% can be mixed with numbered %-sequences.

The + flag character inserts a + before any nonnegative number, while a
space inserts a space before any nonnegative number; these flags
affect only numeric %-sequences, and the + flag takes precedence.
The - and 0 flags affect the width specifier, as described below.

The # flag means to use an alternate display form for %o, %x, %X, %e,
%f, and %g sequences: for %o, it ensures that the result begins with
\"0\"; for %x and %X, it prefixes nonzero results with \"0x\" or \"0X\";
for %e and %f, it causes a decimal point to be included even if the
precision is zero; for %g, it causes a decimal point to be
included even if the precision is zero, and also forces trailing
zeros after the decimal point to be left in place.

The width specifier supplies a lower limit for the length of the
produced representation.  The padding, if any, normally goes on the
left, but it goes on the right if the - flag is present.  The padding
character is normally a space, but it is 0 if the 0 flag is present.
The 0 flag is ignored if the - flag is present, or the format sequence
is something other than %d, %o, %x, %e, %f, and %g.

For %e and %f sequences, the number after the "." in the precision
specifier says how many decimal places to show; if zero, the decimal
point itself is omitted.  For %g, the precision specifies how many
significant digits to produce; zero or omitted are treated as 1.
For %s and %S, the precision specifier truncates the string to the
given width.

Text properties, if any, are copied from the format-string to the
produced text.

usage: (format STRING &rest OBJECTS)]]}
function F.format.f(args)
    return styled_format(args,false)
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
    if lisp.nilp(args[1]) or (lisp.stringp(args[1]) and lisp.sbytes(args[1])==0) then
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
            vars.V.system_name=alloc.make_string(name)
        end
        vars.cached_system_name=vars.V.system_name
    end
    return vars.V.system_name
end
F.propertize={'propertize',1,-2,0,[[Return a copy of STRING with text properties added.
First argument is the string to copy.
Remaining arguments form a sequence of PROPERTY VALUE pairs for text
properties to add to the result.

See Info node `(elisp) Text Properties' for more information.
usage: (propertize STRING &rest PROPERTIES)]]}
function F.propertize.f(args)
    if #args%2==0 then
        signal.xsignal(vars.Qwrong_number_of_arguments,vars.Qpropertize,lisp.make_fixnum(#args))
    end
    local properties=vars.Qnil
    lisp.check_string(args[1])
    local str=vars.F.copy_sequence(args[1])
    for i=2,#args,2 do
        properties=vars.F.cons(args[i],vars.F.cons(args[i+1],properties))
    end
    vars.F.add_text_properties(lisp.make_fixnum(0),lisp.make_fixnum(lisp.schars(str)),properties,str)
    return str
end
F.char_to_string={'char-to-string',1,1,0,[[Convert arg CHAR to a string containing that character.
usage: (char-to-string CHAR)]]}
function F.char_to_string.f(char)
    chars.check_character(char)
    local c=lisp.fixnum(char)
    local str=chars.charstring(c)
    return alloc.make_string_from_bytes(str,1)
end

function M.init_syms()
    vars.defsubr(F,'format')
    vars.defsubr(F,'format_message')
    vars.defsubr(F,'message')
    vars.defsubr(F,'system_name')
    vars.defsubr(F,'propertize')
    vars.defsubr(F,'char_to_string')

    vars.defvar_lisp('system_name','system-name',[[The host name of the machine Emacs is running on.]])
    vars.V.system_name=vars.Qnil
    vars.cached_system_name=vars.Qnil
end
return M
