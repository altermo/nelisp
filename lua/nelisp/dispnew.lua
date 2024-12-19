local vars=require'nelisp.vars'

local function defvar_kboard(name,symname,doc)
    vars.defvar_lisp(name,symname,doc)
    vars.V[name]=vars.Qnil
    if _G.nelisp_later then
        error('TODO')
    end
end

local M={}
function M.init_syms()
    defvar_kboard('window_system','window-system',[[Name of window system through which the selected frame is displayed.
The value is a symbol:
 nil for a termcap frame (a character-only terminal),
 `x' for an Emacs frame that is really an X window,
 `w32' for an Emacs frame that is a window on MS-Windows display,
 `ns' for an Emacs frame on a GNUstep or Macintosh Cocoa display,
 `pc' for a direct-write MS-DOS frame.
 `pgtk' for an Emacs frame using pure GTK facilities.
 `haiku' for an Emacs frame running in Haiku.

Use of this variable as a boolean is deprecated.  Instead,
use `display-graphic-p' or any of the other `display-*-p'
predicates which report frame's specific UI-related capabilities.]])
end

return M
