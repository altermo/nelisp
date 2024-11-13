# Nelisp
The **N**eovim **E**macs **LISP** interpreter.
> [!NOTE]
> This is a work in progress.
## Initialization-script
~~~lua
--- !IMPORTANT
--- This script does not run any elisp files, it only initializes
---
--- If you want to run emacs stdlib, use:
---  `require'nelisp.api'.load('loadup')`
--- note: this will most likely error in some way
---
--- If you want to run elisp code, use:
---  `require'nelisp.api'.eval('(message "Hello world!")')`
--- note: most elisp functions (such as `defun`) are defined in the emacs stdlib
---   If you want the basic features (such as `defun`) it is recommended to load the following:
---   ```lua
---   require'nelisp.api'.load('emacs-lisp/byte-run')
---   require'nelisp.api'.load('emacs-lisp/backquote')
---   require'nelisp.api'.load('subr')
---   ```


--- OPTIONS

--- The emacs lisp runtime dir
--- Defaults to `/usr/share/emacs/29.4/`
local emacs_path -- = '/SOME/EMACS/PATH'

--- Assert that at least some elisp code has been bytecompiled
--- It is STRONGLY recommended that you use byte-compiled elisp code:
--- Using byte-compiled elisp code is around 10x faster than using standard elisp code
local assert_elisp_bytecode_found = true

--- Disable the jit optimization in specific functions
--- It is between 2-4x faster (for unknown reasons)
_G.nelisp_optimize_jit = true

--- Path to use for lua compiled elisp source code
--- It is around 2x faster (after the files are generated)
--- Set to `false` to disable
_G.nelisp_compile_lisp_to_lua_path = '/tmp/nelisp'


--- CODE

assert(vim.fn.has('nvim-0.10')==1,'Requires at least version nvim-0.10')

if emacs_path then
elseif vim.fn.isdirectory('/usr/share/emacs/29.4/')==1 then
    emacs_path='/usr/share/emacs/29.4/'
end
assert(vim.fn.isdirectory(emacs_path)==1,'`emacs_path` is not a directory')

local elisp_path=vim.fs.joinpath(emacs_path,'lisp')
assert(vim.fn.isdirectory(elisp_path)==1,
    '`emacs_path` directory does not contain `/lisp` subdirectory')

if assert_elisp_bytecode_found then
    assert(vim.fn.glob(vim.fs.joinpath(elisp_path,'*.elc'))~='',
        [[`assert_elisp_bytecode_found` set but no bytecode found]])
end

_G.nelisp_load_path={elisp_path}
for name,t in vim.fs.dir(elisp_path) do
    if t=='directory' then
        table.insert(_G.nelisp_load_path,vim.fs.joinpath(elisp_path,name))
    end
end

----- For developers: Reload all nelisp modules
--for k,_ in pairs(package.loaded) do
--    if k:match('^nelisp%.') then
--        package.loaded[k]=nil
--    end
--end

----- For developers: Discard all buffer changes
--local t=vim.api.nvim_get_current_tabpage()
--vim.cmd.tabnew()
--local t2=vim.api.nvim_get_current_tabpage()
--vim.schedule(function ()
    --vim.api.nvim_set_current_tabpage(t2)
    --vim.cmd.tabclose()
    --vim.api.nvim_set_current_tabpage(t)
--end)

require'nelisp.api'.init()
~~~

<!--
+ [#992](https://github.com/neovim/neovim/issues/992) and [#16313](https://github.com/neovim/neovim/issues/16313): major/minor modes
+ [#1435](https://github.com/neovim/neovim/issues/1435): custom placement of statusline, tabline, winbar, cmdline
+ [#2161](https://github.com/neovim/neovim/issues/2161): basically emacs frames
-->
