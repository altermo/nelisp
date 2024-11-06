local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local signal=require'nelisp.signal'
local M={}

local F={}
local function check_lface(_) end
local function resolve_face_name(face_name,signal_p)
    if lisp.stringp(face_name) then
        face_name=vars.F.intern(face_name)
    end
    if lisp.nilp(face_name) or not lisp.symbolp(face_name) then
        return face_name
    end
    local orig_face=face_name
    local hare=face_name
    local has_visited={}
    while true do
        face_name=hare
        hare=vars.F.get(face_name,vars.Qface_alias)
        if lisp.nilp(hare) or not lisp.symbolp(hare) then
            break
        end
        if has_visited[hare] then
            if signal_p then
                signal.xsignal(vars.Qcircular_list,orig_face)
            end
            return vars.Qdefault
        end
        has_visited[hare]=true
    end
    return face_name
end
---@param f nelisp._frame?
---@param face_name nelisp.obj
---@param signal_p boolean
---@return nelisp.obj
local function lface_from_face_name_no_resolve(f,face_name,signal_p)
    local lface
    if f then
        error('TODO')
    else
        lface=vars.F.cdr(vars.F.gethash(face_name,vars.V.face_new_frame_defaults,vars.Qnil))
    end
    if signal_p and lisp.nilp(lface) then
        signal.signal_error('Invalid face',face_name)
    end
    check_lface(lface)
    return lface
end
---@param f nelisp._frame?
---@param face_name nelisp.obj
---@param signal_p boolean
---@return nelisp.obj
local function lface_from_face_name(f,face_name,signal_p)
    face_name=resolve_face_name(face_name,signal_p)
    return lface_from_face_name_no_resolve(f,face_name,signal_p)
end
F.internal_lisp_face_p={'internal-lisp-face-p',1,2,0,[[Return non-nil if FACE names a face.
FACE should be a symbol or string.
If optional second argument FRAME is non-nil, check for the
existence of a frame-local face with name FACE on that frame.
Otherwise check for the existence of a global face.]]}
function F.internal_lisp_face_p.f(face,frame)
    face=resolve_face_name(face,true)
    local lface
    if not lisp.nilp(frame) then
        error('TODO')
    else
        lface=lface_from_face_name(nil,face,false)
    end
    return lface
end

function M.init()
    vars.V.face_new_frame_defaults=vars.F.make_hash_table(vars.QCtest,vars.Qeq,vars.QCsize,33)
end
function M.init_syms()
    vars.defsubr(F,'internal_lisp_face_p')

    vars.defsym('Qface_alias','face-alias')
    vars.defvar_lisp('face_new_frame_defaults','face--new-frame-defaults',[[Hash table of global face definitions (for internal use only.)]])
end
return M
