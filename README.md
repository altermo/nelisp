# Nelisp
The **N**eovim **E**macs **LISP** interpreter.

> [!NOTE]
> This is a work in progress.

## Goals
+ Being able to run emacs plugins such as magit and org-mode
+ Performant(fast) loading of elisp-stdlib (100-200ms)

## Roadmap
+ [x] Rewrite in C ([old-lua-branch](https://github.com/altermo/nelisp/tree/lua))
+ Implement all functions(/other features) to be able to load all of `loadup.el` without errors
+ Implement dumping
+ Implement a bridge between nelisp and neovim
+ Implement the rest of the functions(/other features); make all emacs test pass
