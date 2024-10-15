local vars=require'nelisp.vars'
local str=require'nelisp.obj.str'
local lisp=require'nelisp.lisp'
local buffer=require'nelisp.obj.buffer'

---@class nelisp.buffer_obj_id: function

local M={}

local values=setmetatable({},{__mode='k'})
local function _buf_set_var(id,key,value)
    if not vim.b[id].nelisp_buflocal_id then
        local new_id_object=function () end
        vim.b[id].nelisp_buflocal_id=new_id_object
        values[new_id_object]={}
    end
    local id_object=vim.b[id].nelisp_buflocal_id
    if not values[id_object] then
        values[id_object]={}
    end
    values[id_object][key]=value
end
local function _buf_get_var(id,key)
    if not vim.b[id].nelisp_buflocal_id then
        return nil
    end
    local id_object=vim.b[id].nelisp_buflocal_id
    if not values[id_object] then
        values[id_object]={}
    end
    return values[id_object][key]
end

local vimbuf_vals;vimbuf_vals=setmetatable({},{__index=function (_,vimbuf_id)
    if type(vimbuf_id)=='string' then
        return vimbuf_vals[0][vimbuf_id]
    end
    return setmetatable({},{__index=function (_,key)
        return _buf_get_var(vimbuf_id,key)
    end,__newindex=function (_,key,value)
        _buf_set_var(vimbuf_id,key,value)
    end})
end})

local function vimbuf_get_buf_obj(vimbuf_id)
    if not vimbuf_vals[vimbuf_id].buf_obj then
        vimbuf_vals[vimbuf_id].buf_obj=buffer.make(vimbuf_id)
    end
    return vimbuf_vals[vimbuf_id].buf_obj
end
---@param buf nelisp.buffer
---@return number?
local function buf_obj_get_vimbuf_id(buf)
    if vim.api.nvim_buf_is_valid(buf[2]) then
        return buf[2]
    end
end
---@param str_obj nelisp.str
---@return nelisp.buffer|nelisp.nil
function M.get_buffer_by_name(str_obj)
    local name=lisp.sdata(str_obj)
    local vimbuf_id=vim.fn.bufnr(name)
    if vimbuf_id==-1 then
        return vars.Qnil
    end
    return vimbuf_get_buf_obj(vimbuf_id)
end
---@param str_obj nelisp.str
---@return nelisp.buffer
function M.create_buffer(str_obj)
    local name=lisp.sdata(str_obj)
    local vimbuf_id=vim.api.nvim_create_buf(true,false)
    vim.api.nvim_buf_set_name(vimbuf_id,name)
    if not _G.nelisp_later then
        error('TODO: what if str_obj is force unibyte? maybe set vim.b[id].nvim_buf_name_force_unibyte=true')
    end
    return vimbuf_get_buf_obj(vimbuf_id)
end
---@param buf nelisp.buffer
---@return nelisp.str|nelisp.nil
function M.buffer_name(buf)
    local vimbuf_id=buf_obj_get_vimbuf_id(buf)
    if vimbuf_id==nil then return vars.Qnil end
    local name=vim.api.nvim_buf_get_name(vimbuf_id)
    return name=='' and vars.Qnil or str.make(name,'auto')
end
---@param buf nelisp.buffer
function M.set_current_buffer(buf)
    if not _G.nelisp_later then
        error('TODO: what about unsetting buffer local variables, and other things?')
    end
    local vimbuf_id=buf_obj_get_vimbuf_id(buf)
    vim.api.nvim_set_current_buf(assert(vimbuf_id))
end
return M
