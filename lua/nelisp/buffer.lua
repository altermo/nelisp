local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local nvim=require'nelisp.nvim'
local signal=require'nelisp.signal'
local alloc=require'nelisp.alloc'

local M={}
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
end
return M
