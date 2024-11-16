--- Lua-ls doesn't handle vararg typing well
---@alias nelisp.F_fun_normal
---| fun():nelisp.obj
---| fun(a:nelisp.obj):nelisp.obj
---| fun(a:nelisp.obj,b:nelisp.obj):nelisp.obj
---| fun(a:nelisp.obj,b:nelisp.obj,c:nelisp.obj):nelisp.obj
---| fun(a:nelisp.obj,b:nelisp.obj,c:nelisp.obj,d:nelisp.obj):nelisp.obj
---| fun(a:nelisp.obj,b:nelisp.obj,c:nelisp.obj,d:nelisp.obj,e:nelisp.obj):nelisp.obj
---| fun(a:nelisp.obj,b:nelisp.obj,c:nelisp.obj,d:nelisp.obj,e:nelisp.obj,f:nelisp.obj):nelisp.obj
---| fun(a:nelisp.obj,b:nelisp.obj,c:nelisp.obj,d:nelisp.obj,e:nelisp.obj,f:nelisp.obj,g:nelisp.obj):nelisp.obj
---@alias nelisp.F_fun_args
---| fun(args:nelisp.obj[]):nelisp.obj
---@alias nelisp.F_fun nelisp.F_fun_normal|nelisp.F_fun_args

---@class nelisp.F.entry
---@field [1] string name
---@field [2] number minargs
---@field [3] number maxargs
---@field [4] string|0|nil intspec
---@field [5] string docs
---@field f nelisp.F_fun_normal?
---@field fa nelisp.F_fun_args?
---@alias nelisp.F table<string,nelisp.F.entry>

---@class nelisp.vars
---@field defsym fun(name:string,symname:string)
---@field commit_qsymbols fun()
---@field defvar_lisp fun(name:string,symname:string?,doc:string?):nelisp.obj
---@field defvar_forward fun(name:string,symname:string?,doc:string,get:(fun():nelisp.obj),set:(fun(v:nelisp.obj)))
---@field defvar_bool fun(name:string,symname:string,doc:string?)
---@field defvar_localized fun(name:string,symname:string,doc:string,blv:nelisp.buffer_local_value)
---@field defsubr fun(map:nelisp.F,name:string)
---@field F table<string,nelisp.F_fun>
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
        if _G.nelisp_later then
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
---@param doc string
---@param name string
---@param symname string
---@param blv nelisp.buffer_local_value
vars.defvar_localized=function (name,symname,doc,blv)
    local sym=vars.defvar_lisp(name,symname,doc)
    local p=(sym --[[@as nelisp._symbol]])
    local lisp=require'nelisp.lisp'
    p.redirect=lisp.symbol_redirect.localized
    assert(blv.default_value)
    p.value=blv
end
vars.V=setmetatable({},{__index=function (_,k)
    local sym=assert(Vsymbols[k],'DEV: Not an internal variable: '..tostring(k)) --[[@as nelisp._symbol]]
    local lisp=require'nelisp.lisp'
    if sym.redirect==lisp.symbol_redirect.plainval then
        return assert(lisp.symbol_val(sym),'DEV: Internal variable not set: '..tostring(k))
    elseif sym.redirect==lisp.symbol_redirect.forwarded then
        return sym.value[1]()
    elseif sym.redirect==lisp.symbol_redirect.localized then
        local data=require'nelisp.data'
        return data.find_symbol_value(sym)
    else
        error('TODO')
    end
end,__newindex=function (_,k,v)
        assert(type(v)=='table' and type(v[1])=='number')
        local lisp=require'nelisp.lisp'
        local sym=assert(Vsymbols[k],'DEV: Not an internal variable: '..tostring(k)) --[[@as nelisp._symbol]]
        if sym.redirect==lisp.symbol_redirect.plainval then
            lisp.set_symbol_val(sym,v)
        elseif sym.redirect==lisp.symbol_redirect.forwarded then
            return sym.value[2](v)
        else
            error('TODO')
        end
    end})

vars.F={}
---@param map nelisp.F
---@param name string
function vars.defsubr(map,name)
    assert(not vars.F[name],'DEV: internal function already defined: '..name)
    local d=assert(map[name])
    assert(type(d[1])=='string')
    assert(type(d[2])=='number')
    assert(type(d[3])=='number')
    assert(type(d[4])=='string' or d[4]==0 or d[4]==nil)
    assert(type(d[5])=='string')
    local f
    if d[3]==-2 then
        assert(type(d.fa)=='function',d[1])
        f=assert(d.fa)
    else
        assert(type(d.f)=='function')
        f=assert(d.f)
    end
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
