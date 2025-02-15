local vars=require'nelisp.vars'


require'nelisp.lread'.init_once()
vars.commit_qsymbols()
require'nelisp.eval'.init_once()

vars.commit_qsymbols()

require'nelisp.charset'.init_syms()
require'nelisp.emacs'.init_syms()
require'nelisp.fns'.init_syms()
require'nelisp.lread'.init_syms()
require'nelisp.data'.init_syms()
require'nelisp.eval'.init_syms()
require'nelisp.editfns'.init_syms()
require'nelisp.buffer'.init_syms()
require'nelisp.alloc'.init_syms()
require'nelisp.dev'.init_syms()
require'nelisp.doc'.init_syms()
require'nelisp.keymap'.init_syms()
require'nelisp.chartab'.init_syms()
require'nelisp.print'.init_syms()
require'nelisp.search'.init_syms()
require'nelisp.fileio'.init_syms()
require'nelisp.timefns'.init_syms()
require'nelisp.callint'.init_syms()
require'nelisp.casetab'.init_syms()
require'nelisp.chars'.init_syms()
require'nelisp.bytecode'.init_syms()
require'nelisp.casefiddle'.init_syms()
require'nelisp.coding'.init_syms()
require'nelisp.textprop'.init_syms()
require'nelisp.xfaces'.init_syms()
require'nelisp.frame'.init_syms()
require'nelisp.font'.init_syms()
require'nelisp.minibuf'.init_syms()
require'nelisp.callproc'.init_syms()
require'nelisp.macros'.init_syms()
require'nelisp.syntax'.init_syms()
require'nelisp.category'.init_syms()
require'nelisp.composite'.init_syms()
require'nelisp.xdisp'.init_syms()
require'nelisp.dispnew'.init_syms()
require'nelisp.ccl'.init_syms()
require'nelisp.terminal'.init_syms()
require'nelisp.floatfns'.init_syms()

vars.commit_qsymbols()

require'nelisp.eval'.init()
require'nelisp.category'.init() --Needs to be loaded before buffer
require'nelisp.syntax'.init() --Needs to be loaded before buffer
require'nelisp.buffer'.init()
require'nelisp.fns'.init()
require'nelisp.lread'.init()
require'nelisp.keymap'.init()
require'nelisp.emacs'.init()
require'nelisp.data'.init()
require'nelisp.casetab'.init()
require'nelisp.charset'.init()
require'nelisp.coding'.init()
require'nelisp.xfaces'.init()
require'nelisp.font'.init()
require'nelisp.minibuf'.init()
require'nelisp.frame'.init()
require'nelisp.chars'.init()
require'nelisp.composite'.init()
require'nelisp.xdisp'.init()
require'nelisp.ccl'.init()
require'nelisp.dispnew'.init()
require'nelisp.textprop'.init()

local name,val=debug.getupvalue(getmetatable(vars.V).__newindex,1)
assert(name=='Vsymbols',name)
for k,_ in pairs(val) do
    _=vars.V[k]
end
