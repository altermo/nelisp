local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local nvim=require'nelisp.nvim'
local signal=require'nelisp.signal'
local alloc=require'nelisp.alloc'

local M={}
---@param x nelisp.obj
---@return nelisp.obj
---@nodiscard
function M.check_fixnum_coerce_marker(x)
    if lisp.markerp(x) then
        error('TODO')
    elseif lisp.bignump(x) then
        error('TODO')
    end
    lisp.check_fixnum(x)
    return x
end
local function defvar_per_buffer(name,symname,defval,doc)
    ---@type nelisp.buffer_local_value
    local blv={
        local_if_set=true,
        default_value=defval,
    }
    vars.defvar_localized(name,symname,doc,blv)
end

---@param buffer nelisp._buffer
---@return boolean
function M.BUFFERLIVEP(buffer)
    return not lisp.nilp(nvim.buffer_name(buffer))
end

---@type nelisp.F
local F={}
F.get_buffer={'get-buffer',1,1,0,[[Return the buffer named BUFFER-OR-NAME.
BUFFER-OR-NAME must be either a string or a buffer.  If BUFFER-OR-NAME
is a string and there is no buffer with that name, return nil.  If
BUFFER-OR-NAME is a buffer, return it as given.]]}
function F.get_buffer.f(buffer_or_name)
    if lisp.bufferp(buffer_or_name) then
        return buffer_or_name
    end
    lisp.check_string(buffer_or_name)
    return nvim.get_buffer_by_name(buffer_or_name)
end
local function nsberror(spec)
    if lisp.stringp(spec) then
        signal.error('No buffer named %s',lisp.sdata(spec))
    end
    signal.error('Invalid buffer argument')
end
F.set_buffer={'set-buffer',1,1,0,[[Make buffer BUFFER-OR-NAME current for editing operations.
BUFFER-OR-NAME may be a buffer or the name of an existing buffer.
See also `with-current-buffer' when you want to make a buffer current
temporarily.  This function does not display the buffer, so its effect
ends when the current command terminates.  Use `switch-to-buffer' or
`pop-to-buffer' to switch buffers permanently.
The return value is the buffer made current.]]}
function F.set_buffer.f(buf)
    local buffer=vars.F.get_buffer(buf)
    if lisp.nilp(buffer) then
        nsberror(buf)
    end
    if not M.BUFFERLIVEP(buffer --[[@as nelisp._buffer]]) then
        signal.error('Selecting deleted buffer')
    end
    nvim.set_current_buffer(buffer --[[@as nelisp._buffer]])
    return buffer
end
F.get_buffer_create={'get-buffer-create',1,2,0,[[Return the buffer specified by BUFFER-OR-NAME, creating a new one if needed.
If BUFFER-OR-NAME is a string and a live buffer with that name exists,
return that buffer.  If no such buffer exists, create a new buffer with
that name and return it.

If BUFFER-OR-NAME starts with a space, the new buffer does not keep undo
information.  If optional argument INHIBIT-BUFFER-HOOKS is non-nil, the
new buffer does not run the hooks `kill-buffer-hook',
`kill-buffer-query-functions', and `buffer-list-update-hook'.  This
avoids slowing down internal or temporary buffers that are never
presented to users or passed on to other applications.

If BUFFER-OR-NAME is a buffer instead of a string, return it as given,
even if it is dead.  The return value is never nil.]]}
function F.get_buffer_create.f(buffer_or_name,inhibit_buffer_hooks)
    local buffer=vars.F.get_buffer(buffer_or_name)
    if not lisp.nilp(buffer) then
        return buffer
    end
    if lisp.schars(buffer_or_name)==0 then
        signal.error('Empty string for buffer name is not allowed')
    end
    return nvim.create_buffer(buffer_or_name)
end
F.force_mode_line_update={'force-mode-line-update',0,1,0,[[Force redisplay of the current buffer's mode line and header line.
With optional non-nil ALL, force redisplay of all mode lines, tab lines and
header lines.  This function also forces recomputation of the
menu bar menus and the frame title.]]}
function F.force_mode_line_update.f(all)
    if lisp.nilp(all) then
        vim.cmd.redrawstatus()
        vim.cmd.redrawtabline()
    else
        vim.cmd.redrawstatus{bang=true}
        vim.cmd.redrawtabline()
    end
    return all
end
F.buffer_list={'buffer-list',0,1,0,[[Return a list of all live buffers.
If the optional arg FRAME is a frame, return the buffer list in the
proper order for that frame: the buffers shown in FRAME come first,
followed by the rest of the buffers.]]}
function F.buffer_list.f(frame)
    if _G.nelisp_later then
        error('TODO')
    end
    return nvim.buffer_list()
end
F.current_buffer={'current-buffer',0,0,0,[[Return the current buffer as a Lisp object.]]}
function F.current_buffer.f()
    return nvim.get_current_buffer()
end
F.make_overlay={'make-overlay',2,5,0,[[Create a new overlay with range BEG to END in BUFFER and return it.
If omitted, BUFFER defaults to the current buffer.
BEG and END may be integers or markers.
The fourth arg FRONT-ADVANCE, if non-nil, makes the marker
for the front of the overlay advance when text is inserted there
\(which means the text *is not* included in the overlay).
The fifth arg REAR-ADVANCE, if non-nil, makes the marker
for the rear of the overlay advance when text is inserted there
\(which means the text *is* included in the overlay).]]}
F.make_overlay.f=function (beg,end_,buffer,front_advance,rear_advance)
    if lisp.nilp(buffer) then
        buffer=nvim.get_current_buffer()
    else
        lisp.check_buffer(buffer)
    end
    local b=(buffer --[[@as nelisp._buffer]])
    if not M.BUFFERLIVEP(b) then
        signal.error('Attempt to create overlay in a dead buffer')
    end
    if lisp.markerp(beg) and error('TODO') then
        error('TODO')
    end
    if lisp.markerp(end_) and error('TODO') then
        error('TODO')
    end
    beg=M.check_fixnum_coerce_marker(beg)
    end_=M.check_fixnum_coerce_marker(end_)
    if lisp.fixnum(beg)>lisp.fixnum(end_) then
        beg,end_=end_,beg
    end
    local obeg=lisp.clip_to_bounds(1,lisp.fixnum(beg),nvim.get_buffer_z(b))
    local oend=lisp.clip_to_bounds(obeg,lisp.fixnum(end_),nvim.get_buffer_z(b))
    return nvim.make_overlay(b,obeg,oend,not lisp.nilp(front_advance),not lisp.nilp(rear_advance))
end
F.overlay_put={'overlay-put',3,3,0,[[Set one property of overlay OVERLAY: give property PROP value VALUE.
VALUE will be returned.]]}
function F.overlay_put.f(overlay,prop,value)
    if _G.nelisp_later then
        error('TODO: some of the prop are special and should change the underlying extmark')
    end
    lisp.check_overlay(overlay)
    local ov=overlay --[[@as nelisp._overlay]]
    local b=nvim.overlay_buffer(ov)
    local plist=nvim.overlay_plist(ov)
    local tail=plist
    local changed
    while lisp.consp(tail) and lisp.consp(lisp.xcdr(tail)) do
        if lisp.eq(lisp.xcar(tail),prop) then
            changed=not lisp.eq(lisp.xcar(lisp.xcdr(tail)),value)
            lisp.xsetcar(lisp.xcdr(tail),value)
            goto found
        end
        tail=lisp.xcdr(lisp.xcdr(tail))
    end
    changed=not lisp.nilp(value)
    nvim.overlay_set_plist(ov,vars.F.cons(prop,vars.F.cons(value,plist)))
    ::found::
    if b then
        if _G.nelisp_later then
            error('TODO')
        end
    end
    return value
end
F.delete_overlay={'delete-overlay',1,1,0,[[Delete the overlay OVERLAY from its buffer.]]}
function F.delete_overlay.f(overlay)
    lisp.check_overlay(overlay)
    nvim.drop_overlay(overlay)
    return vars.Qnil
end

function M.init()
    defvar_per_buffer('enable_multibyte_characters','enable-multibyte-characters',vars.Qnil,[[Non-nil means the buffer contents are regarded as multi-byte characters.
Otherwise they are regarded as unibyte.  This affects the display,
file I/O and the behavior of various editing commands.

This variable is buffer-local but you cannot set it directly;
use the function `set-buffer-multibyte' to change a buffer's representation.
To prevent any attempts to set it or make it buffer-local, Emacs will
signal an error in those cases.
See also Info node `(elisp)Text Representations'.]])

    --This should be last, after all per buffer variables are defined
    vars.F.get_buffer_create(alloc.make_unibyte_string('*scratch*'),vars.Qnil)
end
function M.init_syms()
    vars.defsubr(F,'get_buffer_create')
    vars.defsubr(F,'get_buffer')
    vars.defsubr(F,'set_buffer')
    vars.defsubr(F,'force_mode_line_update')
    vars.defsubr(F,'buffer_list')
    vars.defsubr(F,'current_buffer')
    vars.defsubr(F,'make_overlay')
    vars.defsubr(F,'overlay_put')
    vars.defsubr(F,'delete_overlay')
end
return M
