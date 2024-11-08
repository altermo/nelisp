local nvim=require'nelisp.nvim'
local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'

local M={}
---@param f nelisp.obj
function M.check_live_frame(f)
    lisp.check_type(lisp.framep(f) and nvim.frame_live_p(f),vars.Qframe_live_p,f)
end

F={}
F.frame_list={'frame-list',0,0,0,[[Return a list of all live frames.
The return value does not include any tooltip frame.]]}
function F.frame_list.f()
    return lisp.list(unpack(nvim.get_all_frames()))
end

function M.init_syms()
    vars.defsubr(F,'frame_list')
end
return M
