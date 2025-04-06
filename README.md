# Nelisp
The **N**eovim **E**macs **LISP** interpreter.

> [!NOTE]
> This is a work in progress.

## Init
```lua
-- Remember to run `make` once.

-- The `nelisp.c` module is a wrapper around `nelisp.so` using `package.loadlib`.
-- Run `make` to generate a meta file for completions.
local c = require'nelisp.c'

-- Emacs lisp files is managed by `EMACSLOADPATH` environment variable, you need use
-- it let nelisp can find emacs lisp file correctly.
c.init({runtime_path = os.getenv("EMACSLOADPATH")})

c.eval[[
(message "Hello World!")
]]
```

## Goals
+ Being able to run emacs plugins such as magit and org-mode
+ Performant(fast) loading of elisp-stdlib (100-200ms)

## Roadmap
+ [x] Rewrite in C ([old-lua-branch](https://github.com/altermo/nelisp/tree/lua))
+ Implement all functions(/other features) to be able to load all of `loadup.el` without errors
+ Implement dumping
+ Implement a bridge between nelisp and neovim
+ Implement the rest of the functions(/other features); make all emacs test pass
