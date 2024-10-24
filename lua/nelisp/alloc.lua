local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'

local M={}
---@param len number
---@param init nelisp.obj|'zero'|'nil'
function M.make_vector(len,init)
    local v={}
    ---@cast v nelisp._normal_vector
    v.size=len
    v.contents={}
    ---@cast init nelisp.obj
    if init=='zero' then
        ---Needed because make_vector is used before Qnil is inited
        local zero=lisp.make_fixnum(0)
        for i=1,len do
            v.contents[i]=zero
        end
    elseif init~='nil' and not lisp.nilp(init) then
        for i=1,len do
            v.contents[i]=init
        end
    end
    return lisp.make_vectorlike_ptr(v,lisp.pvec.normal_vector)
end
---@param num number (float)
---@return nelisp.obj
function M.make_float(num)
    local new=lisp.make_empty_ptr(lisp.type.float)
    lisp.xfloat_init(new,num)
    return new
end
---@param data string
---@return nelisp.obj
function M.make_pure_c_string(data)
    local s={}
    ---@cast s nelisp._string
    s.size_chars=nil
    s[2]=data
    s.intervals=nil
    return lisp.make_ptr(s,lisp.type.string)
end
---@param c string
---@return nelisp.obj
function M.make_string(c)
    if not _G.nelisp_later then
        error('TODO: multibyte')
    end
    return M.make_unibyte_string(c)
end
---@param c string
---@return nelisp.obj
function M.make_unibyte_string(c)
    local s={}
    ---@cast s nelisp._string
    s[2]=c
    s.size_chars=nil
    s.intervals=nil
    return lisp.make_ptr(s,lisp.type.string)
end
---@param c string
---@param nchars number|-1
---@return nelisp.obj
function M.make_multibyte_string(c,nchars)
    local s={}
    ---@cast s nelisp._string
    s[2]=c
    s.size_chars=vim.str_byteindex(c,#c)
    assert(nchars==s.size_chars or nchars==-1)
    s.intervals=nil
    return lisp.make_ptr(s,lisp.type.string)
end
---@param data string
---@param nchars number|-1
---@param multibyte boolean
function M.make_specified_string(data,nchars,multibyte)
    if multibyte then
        if nchars<0 then
            error('TODO')
        end
        error('TODO')
    end
    return M.make_unibyte_string(data)
end

---@param sym nelisp.obj
---@param name nelisp.obj
function M.set_symbol_name(sym,name)
    (sym --[[@as nelisp._symbol]]).name=name
end
---@param val nelisp.obj
---@param name nelisp.obj
function M.init_symbol(val,name)
    assert(lisp.baresymbolp(val))
    local p=(val --[[@as nelisp._symbol]])
    M.set_symbol_name(val,name)
    lisp.set_symbol_plist(val,vars.Qnil)
    p.redirect=lisp.symbol_redirect.plainval
    lisp.set_symbol_val(p,nil)
    lisp.set_symbol_function(val,vars.Qnil)
    p.interned=lisp.symbol_interned.uninterned
    p.trapped_write=lisp.symbol_trapped_write.untrapped
    p.declared_special=nil --false
end

local F={}
F.list={'list',0,-2,0,[[Return a newly created list with specified arguments as elements.
Allows any number of arguments, including zero.
usage: (list &rest OBJECTS)]]}
function F.list.f(args)
    local val=vars.Qnil
    for i=#args,1,-1 do
        val=vars.F.cons(args[i],val)
    end
    return val
end
---@param car nelisp.obj
---@param cdr nelisp.obj
---@return nelisp.obj
M.cons=function (car,cdr)
    local val=lisp.make_empty_ptr(lisp.type.cons)
    lisp.xsetcar(val,car)
    lisp.xsetcdr(val,cdr)
    return val
end
F.cons={'cons',2,2,0,[[Create a new cons, give it CAR and CDR as components, and return it.]]}
function F.cons.f(car,cdr)
    return M.cons(car,cdr)
end
F.purecopy={'purecopy',1,1,0,[[Make a copy of object OBJ in pure storage.
Recursively copies contents of vectors and cons cells.
Does not copy symbols.  Copies strings without text properties.]]}
function F.purecopy.f(obj)
    return obj
end
F.make_vector={'make-vector',2,2,0,[[Return a newly created vector of length LENGTH, with each element being INIT.
See also the function `vector'.]]}
function F.make_vector.f(length,init)
    lisp.check_type(lisp.fixnatp(length),vars.Qwholenump,length)
    return M.make_vector(lisp.fixnum(length),init)
end
---@param name nelisp.obj
---@return nelisp.obj
function M.make_symbol(name)
    local val=lisp.make_empty_ptr(lisp.type.symbol)
    M.init_symbol(val,name)
    return val
end
F.make_symbol={'make-symbol',1,1,0,[[Return a newly allocated uninterned symbol whose name is NAME.
Its value is void, and its function definition and property list are nil.]]}
function F.make_symbol.f(name)
    lisp.check_string(name)
    return M.make_symbol(name)
end
F.vector={'vector',0,-2,0,[[Return a newly created vector with specified arguments as elements.
Allows any number of arguments, including zero.
usage: (vector &rest OBJECTS)]]}
function F.vector.f(args)
    local vec=M.make_vector(#args,'nil')
    for i=1,#args do
        (vec --[[@as nelisp._normal_vector]]).contents[i]=args[i]
    end
    return vec
end

function M.init_syms()
    vars.defsubr(F,'list')
    vars.defsubr(F,'cons')
    vars.defsubr(F,'purecopy')
    vars.defsubr(F,'make_vector')
    vars.defsubr(F,'make_symbol')
    vars.defsubr(F,'vector')

    vars.defsym('Qchar_table_extra_slots','char-table-extra-slots')
end
return M
