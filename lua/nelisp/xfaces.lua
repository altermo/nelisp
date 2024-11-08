local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local signal=require'nelisp.signal'
local nvim=require'nelisp.nvim'
local alloc=require'nelisp.alloc'
local frame_=require'nelisp.frame'

---@class nelisp.face
---@field lface nelisp.obj
---@field id number
---@field ascii_face nelisp.face

---@class nelisp.face_cache
---@field faces_by_id nelisp.face[]
---@field buckets table<nelisp.face,nelisp.face>

local M={}

local lface_id_to_name={}

local FRAME_WINDOW_P=false --no frames are graphical
local FRAME_TERMCAP_P=true --all frames are in a terminal

---@enum nelisp.lface_index
local lface_index={
    _symbol_face=0,
    family=1,
    foundry=2,
    swidth=3,
    height=4,
    weight=5,
    slant=6,
    underline=7,
    inverse=8,
    foreground=9,
    background=10,
    stipple=11,
    overline=12,
    strike_through=13,
    box=14,
    font=15,
    inherit=16,
    fontset=17,
    distant_foreground=18,
    extend=19,
    size=20,
}
---@enum nelisp.face_id
local face_id={
    default=0,
    mode_line_active=1,
    mode_line_inactive=2,
    tool_bar=3,
    fringe=4,
    header_line=5,
    scroll_bar=6,
    border=7,
    cursor=8,
    mouse=9,
    menu=10,
    vertical_border=11,
    window_divider=12,
    window_divider_first_pixel=13,
    window_divider_last_pixel=14,
    internal_border=15,
    child_frame_border=16,
    tab_bar=17,
    tab_line=18,
}

local function check_lface(_) end
local function check_lface_attrs(_) end
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
        lface=vars.F.gethash(face_name,nvim.frame_hash_table(f),vars.Qnil)
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
local function unspecifiedp(obj)
    return lisp.eq(obj,vars.Qunspecified)
end
local function lface_fully_specified_p(lface)
    for i=1,lisp.asize(lface)-1 do
        if (i~=lface_index.font and i~=lface_index.inherit and i~=lface_index.distant_foreground) and
            (unspecifiedp(lisp.aref(lface,i)) or lisp.eq(vars.QCignore_defface,lisp.aref(lface,i))) then
            return false
        end
    end
    return lisp.asize(lface)==lface_index.size
end
---@param lface nelisp.obj
---@return nelisp.face
local function make_realized_face(lface)
    ---@type nelisp.face
    local face={} --[[@as unknown]]
    face.lface=lface
    face.ascii_face=face
    if not _G.nelisp_later then
        error('TODO: maybe need zero out all other options (emacs does it)')
    end
    return face
end
---@param f nelisp._frame
---@param lface nelisp.obj
---@return nelisp.face
local function realize_tty_face(f,lface)
    local face=make_realized_face(lface)
    if not _G.nelisp_later then
        error('TODO: implement realize_tty_face')
    end
    return face
end
---@param f nelisp._frame
---@param face nelisp.face
local function cache_face(f,face)
    local cache=nvim.frame_face_cache(f)
    face.id=#cache.faces_by_id+1
    cache.faces_by_id[face.id]=face
    cache.buckets[face]=face
end
local function uncache_face(f,face)
    local cache=nvim.frame_face_cache(f)
    cache.buckets[face]=nil
    cache.faces_by_id[face.id]=nil
end
---@param f nelisp._frame
---@param lface nelisp.obj
---@param former_face_id number
local function realize_face(f,lface,former_face_id)
    local face
    check_lface_attrs(lface)
    local cache=nvim.frame_face_cache(f)
    if former_face_id>=0 and cache.faces_by_id[former_face_id] then
        uncache_face(f,cache.faces_by_id[former_face_id])
        if not _G.nelisp_later then
            error('TODO: redraw')
        end
    end
    if FRAME_WINDOW_P then
        error('TODO')
    elseif FRAME_TERMCAP_P then
        face=realize_tty_face(f,lface)
    else
        error('TODO')
    end
    cache_face(f,face)
    return face
end
---@param f nelisp._frame
---@return boolean
local function realize_default_face(f)
    local lface=lface_from_face_name(f,vars.Qdefault,false)
    if lisp.nilp(lface) then
        local frame=f --[[@as nelisp.obj]]
        lface=vars.F.internal_make_lisp_face(vars.Qdefault,frame)
    end
    if FRAME_WINDOW_P then
        error('TODO')
    end
    if not FRAME_WINDOW_P then
        lisp.aset(lface,lface_index.family,alloc.make_string('default'))
        lisp.aset(lface,lface_index.foundry,lisp.aref(lface,lface_index.family))
        lisp.aset(lface,lface_index.swidth,vars.Qnormal)
        lisp.aset(lface,lface_index.height,lisp.make_fixnum(1))
        if unspecifiedp(lisp.aref(lface,lface_index.weight)) then
            lisp.aset(lface,lface_index.weight,vars.Qnormal)
        end
        if unspecifiedp(lisp.aref(lface,lface_index.slant)) then
            lisp.aset(lface,lface_index.slant,vars.Qnormal)
        end
        if unspecifiedp(lisp.aref(lface,lface_index.fontset)) then
            lisp.aset(lface,lface_index.fontset,vars.Qnil)
        end
    end
    if unspecifiedp(lisp.aref(lface,lface_index.extend)) then
        lisp.aset(lface,lface_index.extend,vars.Qnil)
    end
    if unspecifiedp(lisp.aref(lface,lface_index.underline)) then
        lisp.aset(lface,lface_index.underline,vars.Qnil)
    end
    if unspecifiedp(lisp.aref(lface,lface_index.overline)) then
        lisp.aset(lface,lface_index.overline,vars.Qnil)
    end
    if unspecifiedp(lisp.aref(lface,lface_index.strike_through)) then
        lisp.aset(lface,lface_index.strike_through,vars.Qnil)
    end
    if unspecifiedp(lisp.aref(lface,lface_index.box)) then
        lisp.aset(lface,lface_index.box,vars.Qnil)
    end
    if unspecifiedp(lisp.aref(lface,lface_index.inverse)) then
        lisp.aset(lface,lface_index.inverse,vars.Qnil)
    end
    if unspecifiedp(lisp.aref(lface,lface_index.foreground)) then
        local color=vars.F.assq(vars.Qforeground_color,nvim.frame_param_alist(f))
        if lisp.consp(color) and lisp.stringp(lisp.xcdr(color)) then
            lisp.aset(lface,lface_index.foreground,lisp.xcdr(color))
        elseif FRAME_WINDOW_P then
            return false
        elseif FRAME_TERMCAP_P then
            lisp.aset(lface,lface_index.foreground,alloc.make_string('unspecified-fg'))
        end
    end
    if unspecifiedp(lisp.aref(lface,lface_index.background)) then
        local color=vars.F.assq(vars.Qbackground_color,nvim.frame_param_alist(f))
        if lisp.consp(color) and lisp.stringp(lisp.xcdr(color)) then
            lisp.aset(lface,lface_index.background,lisp.xcdr(color))
        elseif FRAME_WINDOW_P then
            return false
        elseif FRAME_TERMCAP_P then
            lisp.aset(lface,lface_index.background,alloc.make_string('unspecified-bg'))
        end
    end
    if unspecifiedp(lisp.aref(lface,lface_index.stipple)) then
        lisp.aset(lface,lface_index.stipple,vars.Qnil)
    end
    assert(lface_fully_specified_p(lface))
    check_lface(lface)
    realize_face(f,lface,face_id.default)
    if FRAME_WINDOW_P then
        error('TODO')
    end
    return true
end
local function get_lface_attributes_no_remap(f,sym,signal_p)
    local lface=lface_from_face_name_no_resolve(f,sym,signal_p)
    if not lisp.nilp(lface) then
        return vars.F.copy_sequence(lface)
    end
    return vars.Qnil
end
local function merge_face_vectors(w,f,from,to,named_merge_points)
    if not unspecifiedp(lisp.aref(from,lface_index.inherit))
        and not lisp.nilp(lisp.aref(from,lface_index.inherit)) then
        error('TODO')
    end
    local font=vars.Qnil
    if lisp.fontp(lisp.aref(from,lface_index.font)) then
        error('TODO')
    end
    for i=1,lface_index.size-1 do
        if not unspecifiedp(lisp.aref(from,i)) then
            if i==lface_index.height and not lisp.fixnump(lisp.aref(from,i)) then
                error('TODO')
            elseif i~=lface_index.font and not lisp.eq(lisp.aref(from,i),lisp.aref(to,i)) then
                error('TODO')
            end
        end
    end
    if not lisp.nilp(font) then
        error('TODO')
    end
    lisp.aset(to,lface_index.inherit,vars.Qnil)
end
---@param f nelisp._frame
---@param sym nelisp.obj
---@param id number
local function realize_named_face(f,sym,id)
    local c=nvim.frame_face_cache(f)
    local lface=lface_from_face_name(f,sym,false)
    local attrs=get_lface_attributes_no_remap(f,vars.Qdefault,true)
    check_lface_attrs(attrs)
    assert(lface_fully_specified_p(attrs))
    if lisp.nilp(lface) then
        lface=vars.F.internal_make_lisp_face(sym,f)
    end
    local symbol_attrs=get_lface_attributes_no_remap(f,sym,true)
    for i=1,lface_index.size-1 do
        if lisp.eq(lisp.aref(symbol_attrs,i),vars.Qreset) then
            lisp.aset(symbol_attrs,i,lisp.aref(attrs,i))
        end
    end
    merge_face_vectors(nil,f,symbol_attrs,attrs,nil)
    realize_face(f,attrs,id)
end
---@param f nelisp._frame
---@return boolean
local function realize_basic_faces(f)
    local success_p=false
    if realize_default_face(f) then
        realize_named_face(f,vars.Qmode_line_active,face_id.mode_line_active)
        realize_named_face(f,vars.Qmode_line_inactive,face_id.mode_line_inactive)
        realize_named_face(f,vars.Qtool_bar,face_id.tool_bar)
        realize_named_face(f,vars.Qfringe,face_id.fringe)
        realize_named_face(f,vars.Qheader_line,face_id.header_line)
        realize_named_face(f,vars.Qscroll_bar,face_id.scroll_bar)
        realize_named_face(f,vars.Qborder,face_id.border)
        realize_named_face(f,vars.Qcursor,face_id.cursor)
        realize_named_face(f,vars.Qmouse,face_id.mouse)
        realize_named_face(f,vars.Qmenu,face_id.menu)
        realize_named_face(f,vars.Qvertical_border,face_id.vertical_border)
        realize_named_face(f,vars.Qwindow_divider,face_id.window_divider)
        realize_named_face(f,vars.Qwindow_divider_first_pixel,face_id.window_divider_first_pixel)
        realize_named_face(f,vars.Qwindow_divider_last_pixel,face_id.window_divider_last_pixel)
        realize_named_face(f,vars.Qinternal_border,face_id.internal_border)
        realize_named_face(f,vars.Qchild_frame_border,face_id.child_frame_border)
        realize_named_face(f,vars.Qtab_bar,face_id.tab_bar)
        realize_named_face(f,vars.Qtab_line,face_id.tab_line)
        success_p=true
    end
    return success_p
end
---@param f nelisp._frame
function M.init_frame_faces(f)
    assert(realize_basic_faces(f))
end

local F={}
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
local function lfacep(lface)
    return lisp.vectorp(lface) and lisp.eq(lisp.aref(lface,0),vars.Qface) and lisp.asize(lface)==lface_index.size
end
F.internal_make_lisp_face={'internal-make-lisp-face',1,2,0,[[Make FACE, a symbol, a Lisp face with all attributes nil.
If FACE was not known as a face before, create a new one.
If optional argument FRAME is specified, make a frame-local face
for that frame.  Otherwise operate on the global face definition.
Value is a vector of face attributes.]]}
function F.internal_make_lisp_face.f(face,frame)
    lisp.check_symbol(face)
    local global_lface=lface_from_face_name(nil,face,false)
    local f,lface
    if not lisp.nilp(frame) then
        frame_.check_live_frame(frame)
        f=(frame --[[@as nelisp._frame]])
        lface=lface_from_face_name(f,face,false)
    else
        lface=vars.Qnil
    end
    if lisp.nilp(global_lface) then
        table.insert(lface_id_to_name,face)
        local face_id_=lisp.make_fixnum(#lface_id_to_name)
        vars.F.put(face,vars.Qface,face_id_)
        global_lface=alloc.make_vector(lface_index.size,vars.Qunspecified)
        lisp.aset(global_lface,0,vars.Qface)
        vars.F.puthash(face,vars.F.cons(face_id_,global_lface),vars.V.face_new_frame_defaults)
    elseif f==nil then
        for i=1,lface_index.size-1 do
            lisp.aset(global_lface,i,vars.Qunspecified)
        end
    end
    if f then
        if lisp.nilp(lface) then
            lface=alloc.make_vector(lface_index.size,vars.Qunspecified)
            lisp.aset(lface,0,vars.Qface)
            vars.F.puthash(face,lface,nvim.frame_hash_table(f))
        else
            for i=1,lface_index.size-1 do
                lisp.aset(lface,i,vars.Qunspecified)
            end
        end
    else
        lface=global_lface
    end
    if lisp.nilp(vars.F.get(face,vars.Qface_no_inherit)) then
        if not _G.nelisp_later then
            error('TODO: redraw')
        end
    end
    assert(lfacep(lface))
    check_lface(lface)
    return lface
end

function M.init()
    vars.V.face_new_frame_defaults=vars.F.make_hash_table(vars.QCtest,vars.Qeq,vars.QCsize,33)
end
function M.init_syms()
    vars.defsubr(F,'internal_lisp_face_p')
    vars.defsubr(F,'internal_make_lisp_face')

    vars.defsym('Qface','face')
    vars.defsym('Qface_no_inherit','face-no-inherit')
    vars.defsym('Qunspecified','unspecified')
    vars.defsym('Qface_alias','face-alias')
    vars.defsym('Qdefault','default')
    vars.defsym('Qnormal','normal')
    vars.defsym('Qforeground_color','foreground-color')
    vars.defsym('Qbackground_color','background-color')
    vars.defsym('QCignore_defface',':ignore-defface')

    vars.defsym('Qmode_line_active','mode-line-active')
    vars.defsym('Qmode_line_inactive','mode-line-inactive')
    vars.defsym('Qtool_bar','tool-bar')
    vars.defsym('Qfringe','fringe')
    vars.defsym('Qscroll_bar','scroll-bar')
    vars.defsym('Qborder','border')
    vars.defsym('Qcursor','cursor')
    vars.defsym('Qmouse','mouse')
    vars.defsym('Qmenu','menu')
    vars.defsym('Qvertical_border','vertical-border')
    vars.defsym('Qwindow_divider','window-divider')
    vars.defsym('Qwindow_divider_first_pixel','window-divider-first-pixel')
    vars.defsym('Qwindow_divider_last_pixel','window-divider-last-pixel')
    vars.defsym('Qinternal_border','internal-border')
    vars.defsym('Qchild_frame_border','child-frame-border')
    vars.defsym('Qtab_bar','tab-bar')
    vars.defsym('Qreset','reset')

    vars.defvar_lisp('face_new_frame_defaults','face--new-frame-defaults',[[Hash table of global face definitions (for internal use only.)]])
end
return M
