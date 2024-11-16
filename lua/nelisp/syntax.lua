local alloc=require'nelisp.alloc'
local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local chartab=require'nelisp.chartab'
local b=require'nelisp.bytes'

---@enum nelisp.syntax_code
local syntax_code={
    whitespace=0,
    punct=1,
    word=2,
    symbol=3,
    open=4,
    close=5,
    quote=6,
    string=7,
    math=8,
    escape=9,
    charquote=10,
    comment=11,
    endcomment=12,
    inherit=13,
    comment_fence=14,
    string_fence=15,
    max=16,
}
local syntax_code_object
local standard_syntax_table

---@type nelisp.F
local F={}
F.standard_syntax_table={'standard-syntax-table',0,0,0,[[Return the standard syntax table.
This is the one used for new buffers.]]}
function F.standard_syntax_table.f()
    return standard_syntax_table
end

local M={}
function M.init()
    syntax_code_object=alloc.make_vector(syntax_code.max,'nil')
    for i=0,syntax_code.max-1 do
        lisp.aset(syntax_code_object,i,lisp.list(lisp.make_fixnum(i)))
    end

    vars.F.put(vars.Qsyntax_table,vars.Qchar_table_extra_slots,lisp.make_fixnum(0))
    local temp=lisp.aref(syntax_code_object,syntax_code.whitespace)
    standard_syntax_table=vars.F.make_char_table(vars.Qsyntax_table,temp)

    temp=lisp.aref(syntax_code_object,syntax_code.punct)
    for i=0,(b' '-1) do
        chartab.set(standard_syntax_table,i,temp)
    end
    chartab.set(standard_syntax_table,b.no_break_space,temp)

    temp=lisp.aref(syntax_code_object,syntax_code.whitespace)
    chartab.set(standard_syntax_table,b' ',temp)
    chartab.set(standard_syntax_table,b'\t',temp)
    chartab.set(standard_syntax_table,b'\n',temp)
    chartab.set(standard_syntax_table,b'\r',temp)
    chartab.set(standard_syntax_table,b'\f',temp)

    temp=lisp.aref(syntax_code_object,syntax_code.word)
    for i=b'a',b'z' do
        chartab.set(standard_syntax_table,i,temp)
    end
    for i=b'A',b'Z' do
        chartab.set(standard_syntax_table,i,temp)
    end
    for i=b'0',b'9' do
        chartab.set(standard_syntax_table,i,temp)
    end
    chartab.set(standard_syntax_table,b'$',temp)
    chartab.set(standard_syntax_table,b'%',temp)

    chartab.set(standard_syntax_table,b'(',vars.F.cons(lisp.make_fixnum(syntax_code.open),lisp.make_fixnum(b')')))
    chartab.set(standard_syntax_table,b')',vars.F.cons(lisp.make_fixnum(syntax_code.close),lisp.make_fixnum(b'(')))
    chartab.set(standard_syntax_table,b'[',vars.F.cons(lisp.make_fixnum(syntax_code.open),lisp.make_fixnum(b']')))
    chartab.set(standard_syntax_table,b']',vars.F.cons(lisp.make_fixnum(syntax_code.close),lisp.make_fixnum(b'[')))
    chartab.set(standard_syntax_table,b'{',vars.F.cons(lisp.make_fixnum(syntax_code.open),lisp.make_fixnum(b'}')))
    chartab.set(standard_syntax_table,b'}',vars.F.cons(lisp.make_fixnum(syntax_code.close),lisp.make_fixnum(b'{')))
    chartab.set(standard_syntax_table,b'"',vars.F.cons(lisp.make_fixnum(syntax_code.string),vars.Qnil))
    chartab.set(standard_syntax_table,b'\\',vars.F.cons(lisp.make_fixnum(syntax_code.escape),vars.Qnil))

    temp=lisp.aref(syntax_code_object,syntax_code.symbol)
    for c in ('_-+*/&|<>='):gmatch'.' do
        chartab.set(standard_syntax_table,b[c],temp)
    end

    temp=lisp.aref(syntax_code_object,syntax_code.punct)
    for c in (".,;:?!#@~^'`"):gmatch'.' do
        chartab.set(standard_syntax_table,b[c],temp)
    end

    temp=lisp.aref(syntax_code_object,syntax_code.word)
    chartab.set_range(standard_syntax_table,0x80,b.MAX_CHAR,temp)
end
function M.init_syms()
    vars.defsym('Qsyntax_table','syntax-table')

    vars.defsubr(F,'standard_syntax_table')
end
return M
