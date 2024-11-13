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
# Build .elc files
cd ~/.nelisp/emacs
make
```
### The `_test.lua` file
This file does the following things (this might be outdated):
+ Add path to runtimepath (if needed)
+ Reloads every nelisp module
+ Initializes the interpreter
+ Creates a temp tabpage (because emacs changes buffers)
+ Tries to run `emacs/lisp/loadup.el`

It is recommended to use this file when running the interpreter.
### `error('TODO')` and `_G.nelisp_later`
+ `error('TODO')` should be used for paths that are not yet implemented
+ `_G.nelisp_later` is (mostly) for when implementing a feature half way, and if this is `true` then error about needing to implement it full way.

<!--
+ [#992](https://github.com/neovim/neovim/issues/992) and [#16313](https://github.com/neovim/neovim/issues/16313): major/minor modes
+ [#1435](https://github.com/neovim/neovim/issues/1435): custom placement of statusline, tabline, winbar, cmdline
+ [#2161](https://github.com/neovim/neovim/issues/2161): basically emacs frames
-->
