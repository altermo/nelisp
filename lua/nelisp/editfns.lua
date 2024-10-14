local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'
local str=require'nelisp.obj.str'

local M={}

local F={}
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
    if not _G.nelisp_later then
        error('TODO')
    end
    if lisp.stringp(args[1]) then
        print(str.to_lua_string(args[1] --[[@as nelisp.str]]))
        return args[1]
    end
end

function M.init_syms()
    vars.setsubr(F,'message')
end
return M
