local home=vim.env.HOME
_G.nelisp_emacs=home..'/.nelisp/emacs'
_G.nelisp_root=home..'/.nelisp/nelisp'

if not vim.tbl_contains(vim.opt.runtimepath:get(),_G.nelisp_root) then
    vim.opt.runtimepath:append(_G.nelisp_root)
end

for k,_ in pairs(package.loaded) do
    if k:match('^nelisp%.') then
        package.loaded[k]=nil
    end
end

_G.nelisp_later=true

local api=require'nelisp.api'
api.init()

--vim.print(('%.1f%% of internal functions implemented (estimate)'):format(#vim.tbl_keys(require'nelisp.vars'.F)/1775*100))

local t=vim.api.nvim_get_current_tabpage()
vim.cmd.tabnew()
local t2=vim.api.nvim_get_current_tabpage()
vim.schedule(function ()
    vim.api.nvim_set_current_tabpage(t2)
    vim.cmd.tabclose()
    vim.api.nvim_set_current_tabpage(t)
end)

if jit.status() then vim.schedule(jit.on) end
jit.off() --luajit makes the code slower (why?)
api.load(_G.nelisp_emacs..'/lisp/loadup.el')
