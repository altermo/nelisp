local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local alloc=require'nelisp.alloc'

---@class nelisp.vim.buffer:nelisp._buffer
---@field bufid number

local M={}

local ref_to_buf_obj=setmetatable({},{__mode='k'})
---@param bufid number
---@return nelisp.obj
local function get_or_create_buf_obj(bufid)
    if not vim.b[bufid].nelisp_reference then
        vim.b[bufid].nelisp_reference=function() end
    end
    if ref_to_buf_obj[vim.b[bufid].nelisp_reference] then
        return ref_to_buf_obj[vim.b[bufid].nelisp_reference]
    end
    ---@type nelisp.vim.buffer
    local b={
        bufid=bufid,
    }
    ref_to_buf_obj[vim.b[bufid].nelisp_reference]=b
    return lisp.make_vectorlike_ptr(b,lisp.pvec.buffer)
end
---@param name nelisp.obj
---@return nelisp.obj
function M.get_buffer_by_name(name)
    local vname=lisp.sdata(name)
    local bufid=vim.fn.bufnr(vname)
    if bufid==-1 then
        return vars.Qnil
    end
    return get_or_create_buf_obj(bufid)
end
---@param name nelisp.obj
---@return nelisp.obj
function M.create_buffer(name)
    local vname=lisp.sdata(name)
    local bufid=vim.api.nvim_create_buf(true,false)
    vim.api.nvim_buf_set_name(bufid,vname)
    if not _G.nelisp_later then
        error('TODO: what if name is multibyte which looks like unibyte or vice versa?')
    end
    return get_or_create_buf_obj(bufid)
end
---@param buffer nelisp._buffer
---@return nelisp.obj
function M.buffer_name(buffer)
    ---@cast buffer nelisp.vim.buffer
    local id=buffer.bufid
    if not vim.api.nvim_buf_is_valid(id) then
        return vars.Qnil
    end
    local name=vim.api.nvim_buf_get_name(id)
    assert(name~='','TODO: what should be the name of a nameless buffers')
    return alloc.make_string(name)
end
---@param buffer nelisp._buffer
function M.set_current_buffer(buffer)
    if not _G.nelisp_later then
        error('TODO: what about unsetting buffer local variables, and other things?')
    end
    ---@cast buffer nelisp.vim.buffer
    vim.api.nvim_set_current_buf(buffer.bufid)
end
return M
