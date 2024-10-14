# Nelisp
The **N**eovim **E**macs **LISP** interpreter.
> [!NOTE]
> This is a work in progress.
## The dev environment
### Setup
Run in bash:
```bash
mkdir -p ~/.nelisp
git clone https://github.com/emacs-mirror/emacs --depth 1 --branch emacs-29.4 ~/.nelisp/emacs
git clone https://github.com/altermo/nelisp ~/.nelisp/nelisp
```
### The `_test.lua` file
This file does the following things (this might be outdated):
+ Add path to runtimepath (if needed)
+ Reloads every nelisp module
+ Set up `_G.p` and `_G.ins` to print and inspect nelisp objects
+ Initializes the interpreter
+ Creates a temp tabpage (because emacs changes buffers)
+ Tries to run `emacs/lisp/loadup.el`

It is recommended to use this file when running the interpreter.
### `error('TODO')`, `error('TODO: err')` and `_G.nelisp_later`
+ `error('TODO')` should be used for paths that are not yet implemented
+ `error('TODO: err')` should be used when emacs raises an error (but because error handling isn't implemented yet, this is used)
+ `_G.nelisp_later` is (mostly) for when implementing a feature half way, and if this is false then error about needing to implement it full way.

<!--
+ [#992](https://github.com/neovim/neovim/issues/992) and [#16313](https://github.com/neovim/neovim/issues/16313): major/minor modes
+ [#1435](https://github.com/neovim/neovim/issues/1435): custom placement of statusline, tabline, winbar, cmdline
-->
