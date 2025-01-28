local vars=require'nelisp.vars'
local frame_=require'nelisp.frame'
local lisp=require'nelisp.lisp'
local nvim=require'nelisp.nvim'
local alloc=require'nelisp.alloc'
local signal=require'nelisp.signal'
local M={}

---@type nelisp.F
local F={}
---@param frame nelisp.obj
---@return nelisp._frame
local function decode_live_frame(frame)
    if lisp.nilp(frame) then
        error('TODO')
    end
    frame_.check_live_frame(frame)
    return frame --[[@as nelisp._frame]]
end
F.frame_terminal={'frame-terminal',0,1,0,[[Return the terminal that FRAME is displayed on.
If FRAME is nil, use the selected frame.

The terminal device is represented by its integer identifier.]]}
function F.frame_terminal.f(frame)
    local t=nvim.frame_terminal(decode_live_frame(frame))
    if t==nil then
        return vars.Qnil
    else
        return t
    end
end
---@param terminal nelisp.obj
---@return nelisp._terminal?
local function decode_terminal(terminal)
    if lisp.nilp(terminal) then
        error('TODO')
    end
    local t=(lisp.terminalp(terminal) and terminal --[[@as nelisp._terminal]]) or
        (frame_.framep(terminal) and nvim.frame_terminal(terminal --[[@as nelisp._frame]]))
    if (not t) or (not nvim.terminal_live_p(t)) then
        return nil
    end
    return t
end
---@param terminal nelisp.obj
---@return nelisp._terminal
local function decode_live_terminal(terminal)
    local t=decode_terminal(terminal)
    if t==nil then
        signal.wrong_type_argument(vars.Qterminal_live_p,terminal)
        error('unreachable')
    end
    return t
end
F.terminal_name={'terminal-name',0,1,0,[[Return the name of the terminal device TERMINAL.
It is not guaranteed that the returned value is unique among opened devices.

TERMINAL may be a terminal object, a frame, or nil (meaning the
selected frame's terminal).]]}
function F.terminal_name.f(terminal)
    local t=decode_live_terminal(terminal)
    local name=nvim.terminal_name(t)
    if name==nil then
        return vars.Qnil
    else
        return alloc.make_string(name)
    end
end

function M.init_syms()
    vars.defsubr(F,'frame_terminal')
    vars.defsubr(F,'terminal_name')
end
return M
