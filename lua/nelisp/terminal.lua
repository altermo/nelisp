local vars=require'nelisp.vars'
local frame_=require'nelisp.frame'
local lisp=require'nelisp.lisp'
local nvim=require'nelisp.nvim'
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

function M.init_syms()
    vars.defsubr(F,'frame_terminal')
end
return M
