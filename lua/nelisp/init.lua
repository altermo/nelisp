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

vars.commit_qsymbols()

require'nelisp.eval'.init()
require'nelisp.buffer'.init()
require'nelisp.fns'.init()
require'nelisp.lread'.init()

if not _G.nelisp_later then
    local vars=require'nelisp.vars'
    local name,val=debug.getupvalue(getmetatable(vars.V).__newindex,2)
    assert(name=='Vsymbol_pre_values',name)
    assert(vim.tbl_isempty(val),'Vsymbol_pre_values not empty')
end

