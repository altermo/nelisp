local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local M={}
function M.init()
    vars.F.put(vars.Qglyphless_char_display,vars.Qchar_table_extra_slots,lisp.make_fixnum(1))
    vars.V.glyphless_char_display=vars.F.make_char_table(vars.Qglyphless_char_display,vars.Qnil)
    vars.F.set_char_table_extra_slot(vars.V.glyphless_char_display,lisp.make_fixnum(0),vars.Qempty_box)
end
function M.init_syms()
    vars.defsym('Qempty_box','empty-box')

    vars.defsym('Qglyphless_char_display','glyphless-char-display')
    vars.defvar_lisp('glyphless_char_display','glyphless-char-display',[[Char-table defining glyphless characters.
Each element, if non-nil, should be one of the following:
  an ASCII acronym string: display this string in a box
  `hex-code':   display the hexadecimal code of a character in a box
  `empty-box':  display as an empty box
  `thin-space': display as 1-pixel width space
  `zero-width': don't display
Any other value is interpreted as `empty-box'.
An element may also be a cons cell (GRAPHICAL . TEXT), which specifies the
display method for graphical terminals and text terminals respectively.
GRAPHICAL and TEXT should each have one of the values listed above.

The char-table has one extra slot to control the display of characters
for which no font is found on graphical terminals, and characters that
cannot be displayed by text-mode terminals.  Its value should be an
ASCII acronym string, `hex-code', `empty-box', or `thin-space'.  It
could also be a cons cell of any two of these, to specify separate
values for graphical and text terminals.  The default is `empty-box'.

With the obvious exception of `zero-width', all the other representations
are displayed using the face `glyphless-char'.

If a character has a non-nil entry in an active display table, the
display table takes effect; in this case, Emacs does not consult
`glyphless-char-display' at all.]])
end
return M