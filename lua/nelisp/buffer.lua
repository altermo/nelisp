local vars=require'nelisp.vars'
local str=require'nelisp.obj.str'
local lisp=require'nelisp.lisp'
local nvim=require'nelisp.nvim'
local signal=require'nelisp.signal'
local M={}

function M.init()
    vars.F.get_buffer_create(str.make('*scratch*',false),vars.Qnil)
end
---@param buffer nelisp.buffer
function M.bufferlivep(buffer)
    return not lisp.nilp(nvim.buffer_name(buffer))
end

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
    return nvim.get_buffer(buffer_or_name)
end
local function nsberror(spec)
    if lisp.stringp(spec) then
        signal.error('No buffer named %s',str.to_lua_string(spec))
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
    if not M.bufferlivep(buffer) then
        signal.error('Selecting deleted buffer')
    end
    nvim.set_current_buffer(buffer)
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
    if str.index1_neg(buffer_or_name,1)==-1 then
        signal.error('Empty string for buffer name is not allowed')
    end
    return nvim.create_buffer(buffer_or_name)
end

function M.init_syms()
    vars.setsubr(F,'get_buffer_create')
    vars.setsubr(F,'get_buffer')
    vars.setsubr(F,'set_buffer')
end
return M
