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

--- The (29.4) emacs runtime dir
local emacs_path='/usr/share/emacs/29.4/'

--- Assert that at least some elisp code has been bytecompiled
--- It is STRONGLY recommended that you use byte-compiled elisp code:
--- Using byte-compiled elisp code is around 10x faster than using standard elisp code
local assert_elisp_bytecode_found = true

--- Disable the jit optimization in specific functions
--- It is between 1.5-2x faster (for unknown reasons)
_G.nelisp_optimize_jit = true

--- Path to use for lua compiled elisp source code
--- It is around 2x faster (after the files are generated)
--- Set to `false` to disable
_G.nelisp_compile_lisp_to_lua_path = '/tmp/nelisp'


--- CODE

assert(vim.fn.has('nvim-0.10')==1,'Requires at least version nvim-0.10')

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

local data_path=vim.fs.joinpath(emacs_path,'etc')
assert(vim.fn.isdirectory(elisp_path)==1,
    '`emacs_path` directory does not contain `/etc` subdirectory')
_G.nelisp_data_path=data_path


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

## Goals
+ Being able to run emacs plugins such as magit and org-mode
+ Performant(fast) loading of elisp-stdlib (100-200ms)

## Roadmap
+ Implement all functions(/other features) to be able to load all of `loadup.el` without errors
+ Implement dumping
+ Rewrite in C (if lua is too slow)
+ Implement a bridge between nelisp and neovim
+ Implement the rest of the functions(/other features); make all emacs test pass
