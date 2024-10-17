local vars=require'nelisp.vars'

require'nelisp.lread'.obarray_init_once()

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

vars.commit_qsymbols()

require'nelisp.eval'.init()
require'nelisp.buffer'.init()
require'nelisp.fns'.init()
require'nelisp.lread'.init()
require'nelisp.keymap'.init()
require'nelisp.emacs'.init()

if not _G.nelisp_later then
    local name,val=debug.getupvalue(getmetatable(vars.V).__newindex,2)
    assert(name=='Vsymbols',name)
    for k,_ in pairs(val) do
        _=vars.V[k]
    end
end

