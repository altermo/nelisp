local vars=require'nelisp.vars'
local str=require'nelisp.obj.str'
local lisp=require'nelisp.lisp'
local buffer=require'nelisp.obj.buffer'
local M={}
local buffer_to_id=setmetatable({},{__mode='k'})
local id_to_buffer;id_to_buffer=vim.defaulttable(function (id)
    assert(vim.api.nvim_buf_is_valid(id))
    vim.api.nvim_buf_attach(id,false,{on_detach=function (_,id_)
        --TODO: do something like https://github.com/neovim/neovim/pull/28748#issuecomment-2407090007 instead
        buffer_to_id[id_to_buffer[id_]]=nil
        id_to_buffer[id_]=nil
    end})
    local buf=buffer.make(id)
    buffer_to_id[buf]=id
    return buf
end)
---@param str_obj nelisp.str
---@return nelisp.buffer|nelisp.nil
function M.get_buffer(str_obj)
    local name=lisp.sdata(str_obj)
    local id=vim.fn.bufnr(name)
    if id==-1 then
        return vars.Qnil
    end
    return id_to_buffer[id]
end
---@param str_obj nelisp.str
---@return nelisp.buffer
function M.create_buffer(str_obj)
    local name=lisp.sdata(str_obj)
    local id=vim.api.nvim_create_buf(true,false)
    vim.api.nvim_buf_set_name(id,name)
    if not _G.nelisp_later then
        error('TODO: what if str_obj is force unibyte? maybe set vim.b[id].nvim_buf_name_force_unibyte=true')
    end
    return id_to_buffer[id]
end
---@param buf nelisp.buffer
---@return nelisp.str|nelisp.nil
function M.buffer_name(buf)
    local id=buffer_to_id[buf]
    local name=vim.api.nvim_buf_get_name(id)
    return name=='' and vars.Qnil or str.make(name,'auto')
end
---@param buf nelisp.buffer
function M.set_current_buffer(buf)
    if not _G.nelisp_later then
        error('TODO: what about unsetting buffer local variables, and other things?')
    end
    local id=buffer_to_id[buf]
    vim.api.nvim_set_current_buf(id)
end
return M
