local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local alloc=require'nelisp.alloc'

local M={}

--- ;; Buffer
---@class nelisp.vim.buffer:nelisp._buffer
---@field bufid number
---@field vars table<nelisp._symbol,nelisp.obj>
---@field category_table nelisp.obj
---@field syntax_table nelisp.obj

local ref_to_buf=setmetatable({},{__mode='k'})
---@param bufid number
---@return nelisp.obj
local function get_or_create_buf_obj(bufid)
    if not vim.b[bufid].nelisp_reference then
        vim.b[bufid].nelisp_reference=function() end
    end
    if ref_to_buf[vim.b[bufid].nelisp_reference] then
        return ref_to_buf[vim.b[bufid].nelisp_reference]
    end
    ---@type nelisp.vim.buffer
    local b={
        bufid=bufid,
        vars={},
        category_table=vars.standard_category_table,
        syntax_table=vars.standard_syntax_table,
    }
    ref_to_buf[vim.b[bufid].nelisp_reference]=b
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
    if _G.nelisp_later then
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
    if _G.nelisp_later then
        error('TODO: what about unsetting buffer local variables, and other things?')
    end
    ---@cast buffer nelisp.vim.buffer
    vim.api.nvim_set_current_buf(buffer.bufid)
end
---@return nelisp.obj
function M.get_current_buffer()
    return get_or_create_buf_obj(vim.api.nvim_get_current_buf())
end
---@param buffer nelisp._buffer
---@param sym nelisp._symbol
---@return nelisp.obj?
function M.get_buffer_var(buffer,sym)
    ---@cast buffer nelisp.vim.buffer
    return buffer.vars[sym]
end
---@param buffer nelisp._buffer
---@param sym nelisp._symbol
---@param val nelisp.obj|nil
function M.set_buffer_var(buffer,sym,val)
    ---@cast buffer nelisp.vim.buffer
    buffer.vars[sym]=val
end
--TODO: instead implement a general BVAR like function
---@param buffer nelisp._buffer
---@return nelisp.obj
function M.buffer_category_table(buffer)
    ---@cast buffer nelisp.vim.buffer
    return buffer.category_table
end
---@param buffer nelisp._buffer
---@return nelisp.obj
function M.buffer_syntax_table(buffer)
    ---@cast buffer nelisp.vim.buffer
    return buffer.syntax_table
end

--- ;; Terminal (UI)

---@class nelisp.vim.terminal: nelisp._terminal
---@field chan_id number
----- No more fields, gets cleaned up if no references remain.

local ref_to_terminal=setmetatable({},{__mode='v'})
---@param chan_id integer
---@return nelisp.obj
function M.get_or_create_terminal(chan_id)
    if ref_to_terminal[chan_id] then
        return ref_to_terminal[chan_id]
    end
    local chan_info=vim.api.nvim_get_chan_info(chan_id)
    assert(next(chan_info))
    assert(not vim.tbl_get(chan_info,'client','type') or chan_info.client.type=='ui')
    ---@type nelisp.vim.terminal
    local t={
        chan_id=chan_id,
    }
    ref_to_terminal[chan_id]=t
    return lisp.make_vectorlike_ptr(t,lisp.pvec.terminal)
end
---@return nelisp.obj[]
function M.list_live_terminals()
    local terminals={}
    for _,chan_info in ipairs(vim.api.nvim_list_uis()) do
        table.insert(terminals,M.get_or_create_terminal(chan_info.chan))
    end
    return terminals
end

--- ;; Frame
---@class nelisp.vim.frame: nelisp._frame
---@field tabpage_id integer
---@field face_hash_table nelisp.obj
---@field param_alist nelisp.obj
---@field face_cache nelisp.face_cache

local ref_to_frame=setmetatable({},{__mode='k'})
---@param tab_id number
---@return nelisp.obj
local function get_or_create_frame(tab_id)
    if not vim.t[tab_id].nelisp_reference then
        vim.t[tab_id].nelisp_reference=function() end
    end
    if ref_to_frame[vim.t[tab_id].nelisp_reference] then
        return ref_to_frame[vim.t[tab_id].nelisp_reference]
    end
    ---@type nelisp.vim.frame
    local t={
        tabpage_id=tab_id,
        face_hash_table=vars.F.make_hash_table(vars.QCtest,vars.Qeq),
        param_alist=vars.Qnil,
        face_cache={faces_by_id={},buckets={}},
    }
    ref_to_frame[vim.t[tab_id].nelisp_reference]=t
    local obj=lisp.make_vectorlike_ptr(t,lisp.pvec.frame)
    require'nelisp.xfaces'.init_frame_faces(t)
    return obj
end
---@return nelisp.obj
function M.get_current_frame()
    return get_or_create_frame(vim.api.nvim_get_current_tabpage())
end
function M.get_all_frames()
    local frames={}
    for _,tab_id in ipairs(vim.api.nvim_list_tabpages()) do
        table.insert(frames,get_or_create_frame(tab_id))
    end
    return frames
end
---@param f nelisp._frame
function M.frame_hash_table(f)
    return (f --[[@as nelisp.vim.frame]]).face_hash_table
end
---@param f nelisp._frame
---@return nelisp.obj
function M.frame_param_alist(f)
    return (f --[[@as nelisp.vim.frame]]).param_alist
end
---@param f nelisp._frame
---@return nelisp.face_cache
function M.frame_face_cache(f)
    return (f --[[@as nelisp.vim.frame]]).face_cache
end
---@param f nelisp._frame
---@return boolean
function M.frame_live_p(f)
    return vim.api.nvim_tabpage_is_valid((f --[[@as nelisp.vim.frame]]).tabpage_id)
end
---@param f nelisp._frame
---@return number
function M.frame_foreground_pixel(f)
    return -2
end
---@param f nelisp._frame
---@return number
function M.frame_background_pixel(f)
    return -3
end
---@param f nelisp._frame
---@return nelisp.obj
function M.frame_name(f)
    return alloc.make_string('F'..(f --[[@as nelisp.vim.frame]]).tabpage_id)
end
---@param f nelisp._frame
---@return number
function M.frame_height(f)
    return vim.o.lines
end
---@param f nelisp._frame
---@return number
function M.frame_width(f)
    return vim.o.columns
end
---@param f nelisp._frame
---@return boolean
function M.frame_wants_modeline_p(f)
    return vim.o.laststatus==0 or vim.o.laststatus==1
end
---@param f nelisp._frame
---@return nelisp.obj
function M.frame_buffer_list(f)
    local buffers={}
    for _,win_id in ipairs(vim.api.nvim_tabpage_list_wins((f --[[@as nelisp.vim.frame]]).tabpage_id)) do
        buffers[vim.api.nvim_win_get_buf(win_id)]=true
    end
    return lisp.list(unpack(vim.tbl_keys(buffers)))
end
---@param f nelisp._frame
---@return nelisp.obj
function M.frame_buried_buffer_list(f)
    return vars.Qnil
end
---@param f nelisp._frame
---@return number
function M.frame_menu_bar_lines(f)
    return 0
end
---@param f nelisp._frame
---@return number
function M.frame_tab_bar_lines(f)
    if vim.o.showtabline==1 and #vim.api.nvim_list_tabpages()>1 then
        return 1
    end
    return vim.o.showtabline==2 and 1 or 0
end

--- ;; Marker

---@class nelisp.vim.marker:nelisp._marker
---@field bufid number?
---@field id number?

---@return nelisp.obj
function M.make_marker()
    ---@type nelisp.vim.marker
    local m={}
    return lisp.make_vectorlike_ptr(m,lisp.pvec.marker)
end
return M
