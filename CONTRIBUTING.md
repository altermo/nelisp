# Commits
Do [Conventional Commits](https://www.conventionalcommits.org)

# Coding
## Comments
All comments have to use `//`, and not `/* */`, with the exception of when a `/* */` is required/used by external tools to generate stuff. This makes it easier to know what's a comment from the emacs source code and what's a nelisp comment.

## Supported os
Only the tartets which nvim has tier 1 support for are designed to be supported (see `:h support).

## Lint
The linter is run with `make check` (uses `clang-check`).
A stronger version of the linter is run with `make analyze` (which also uses `clang-check`).
Only the weaker version is needs to pass.

## Style
The formatter is run with `make format` (uses `clang-format`).
There's not yet any style guide for lua code, so do whatever you want.
