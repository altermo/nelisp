---@class nelisp.vars
---@field defsym fun(name:string,symname:string)
---@field commit_qsymbols fun()
---@field defvar_lisp fun(name:string,symname:string?,doc:string?):nelisp.obj
---@field defvar_forward fun(name:string,symname:string?,doc:string,get:(fun():nelisp.obj),set:(fun(v:nelisp.obj)))
---@field defvar_bool fun(name:string,symname:string,doc:string?)
---@field defsubr fun(map:nelisp.defsubr_map,name:string)
---@field F table<string,fun(...:(nelisp.obj|nelisp.obj[])):nelisp.obj>
---@field V table<string,nelisp.obj>
---@field modifier_symbols (0|nelisp.obj)[]
---@field lisp_eval_depth number
---@field charset_ascii number
---@field charset_iso_8859_1 number
---@field charset_unicode number
---@field charset_emacs number
---@field charset_eight_bit number
---@field charset_table nelisp.charset[]
---@field iso_charset_table number[][][]
---@field emacs_mule_charset number[]
---@field emacs_mule_bytes number[]
---@field charset_ordered_list_tick number
---@field charset_jisx0201_roman number
---@field charset_jisx0208_1978 number
---@field charset_jisx0208 number
---@field charset_ksc5601 number
---@field safe_terminal_coding nelisp.coding_system
---@field [string] nelisp.obj
local vars={}

local Qsymbols={}
local Qsymbols_later={}
---@param name string
---@param symname string
function vars.defsym(name,symname)
    assert(name:sub(1,1)=='Q','DEV: Internal symbol must start with Q')
    assert(not Qsymbols_later[name] and not Qsymbols[name],'DEV: Internal symbol already defined: '..name)
    local lread=require'nelisp.lread'
    local lisp=require'nelisp.lisp'
    local sym
    local found=lread.lookup(vars.initial_obarray,symname)
    if type(found)=='number' then
        sym=lisp.make_empty_ptr(lisp.type.symbol)
        if symname=='nil' then
            Qsymbols[name]=sym
        else
            Qsymbols_later[name]=sym
        end
        lread.define_symbol(sym,symname)
    else
        Qsymbols_later[name]=found
    end
end
function vars.commit_qsymbols()
    Qsymbols=setmetatable(Qsymbols_later,{__index=Qsymbols})
    Qsymbols_later={}
end

---@type table<string,nelisp.obj>
local Vsymbols={}
---@param name string
---@param symname string?
---@param doc string?
---@return nelisp.obj
function vars.defvar_lisp(name,symname,doc)
    assert(not name:match('%-'),'DEV: Internal variable names must not contain -')
    assert(not symname or not symname:match('_'),'DEV: Internal variable symbol names must should probably not contain _')
    assert(name:sub(1,1)~='V','DEV: Internal variable names must not start with V')
    assert(not Vsymbols[name],'DEV: Internal variable already defined: '..name)
    local lread=require'nelisp.lread'
    local lisp=require'nelisp.lisp'
    local sym
    if symname then
        local found=lread.lookup(vars.initial_obarray,symname)
        if type(found)=='number' then
            sym=lisp.make_empty_ptr(lisp.type.symbol)
            lread.define_symbol(sym,symname)
        else
            sym=found
        end
    else
        sym=lisp.make_empty_ptr(lisp.type.symbol)
        local alloc=require'nelisp.alloc'
        alloc.init_symbol(sym,alloc.make_pure_c_string(name))
    end
    Vsymbols[name]=sym
    if doc then
        if not _G.nelisp_later then
            error('TODO')
        end
    end
    return sym
end
---@param doc string?
---@param name string
---@param symname string
function vars.defvar_bool(name,symname,doc)
    vars.defvar_lisp(name,symname,doc)
    local alloc=require'nelisp.alloc'
    local lisp=require'nelisp.lisp'
    lisp.set_symbol_val(vars.Qbyte_boolean_vars --[[@as nelisp._symbol]],
        alloc.cons(Vsymbols[name],lisp.symbol_val(vars.Qbyte_boolean_vars --[[@as nelisp._symbol]])))
end
---@param doc string
---@param name string
---@param symname string
---@param get fun():nelisp.obj
---@param set fun(v:nelisp.obj)
vars.defvar_forward=function (name,symname,doc,get,set)
    local sym=vars.defvar_lisp(name,symname,doc)
    local p=(sym --[[@as nelisp._symbol]])
    local lisp=require'nelisp.lisp'
    p.redirect=lisp.symbol_redirect.forwarded
    p.value={get,set} --[[@as nelisp.forward]]
end
vars.V=setmetatable({},{__index=function (_,k)
    local sym=assert(Vsymbols[k],'DEV: Not an internal variable: '..tostring(k)) --[[@as nelisp._symbol]]
    local lisp=require'nelisp.lisp'
    if sym.redirect==lisp.symbol_redirect.forwarded then
        return sym.value[1]()
    end
    return assert(lisp.symbol_val(sym),'DEV: Internal variable not set: '..tostring(k))
end,__newindex=function (_,k,v)
        assert(type(v)=='table' and type(v[1])=='number')
        local lisp=require'nelisp.lisp'
        local sym=assert(Vsymbols[k],'DEV: Not an internal variable: '..tostring(k)) --[[@as nelisp._symbol]]
        if sym.redirect==lisp.symbol_redirect.forwarded then
            return sym.value[2](v)
        end
        lisp.set_symbol_val(sym,v)
    end})

vars.F={}
---@alias nelisp.defsubr_map {[string]:{[1]:string,[2]:number,[3]:number,[4]:string|0,[5]:string,f:fun(...:nelisp.obj):nelisp.obj}}
---@param map nelisp.defsubr_map
---@param name string
function vars.defsubr(map,name)
    assert(not vars.F[name],'DEV: internal function already defined: '..name)
    local d=assert(map[name])
    local f=assert(d.f)
    assert(type(d[1])=='string')
    assert(type(d[2])=='number')
    assert(type(d[3])=='number')
    assert(type(d[4])=='string' or d[4]==0)
    assert(type(d[5])=='string')
    assert(type(d.f)=='function')
    assert(#d==5)
    vars.F[name]=f
    local symname=d[1]
    if d[3]>=0 and d[3]<=8 then
        assert(debug.getinfo(f,'u').nparams==d[3])
    else
        assert(debug.getinfo(f,'u').nparams==1)
    end
    local lread=require'nelisp.lread'
    local sym=lread.intern_c_string(symname)
    local lisp=require'nelisp.lisp'
    local subr=lisp.make_vectorlike_ptr(
        {
            fn=f,
            minargs=d[2],
            maxargs=d[3],
            symbol_name=symname,
            intspec=d[4]~=0 and d[4] --[[@as string]] or nil,
            docs=d[5],
        } --[[@as nelisp._subr]],lisp.pvec.subr)
    lisp.set_symbol_function(sym,subr)
end

return setmetatable(vars,{__index=function (_,k)
    if Qsymbols[k] then
        return Qsymbols[k]
    end
    error('DEV: try to index out of bounds: '..tostring(k))
end})
