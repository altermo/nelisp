local symbol=require'nelisp.obj.symbol'
local cons=require'nelisp.obj.cons'
local str=require'nelisp.obj.str'
local subr=require'nelisp.obj.subr'
local vars={}

local Qsymbols={}
local Qsymbols_later={}
---@param name string
---@param symname string
function vars.defsym(name,symname)
    assert(name:sub(1,1)=='Q','DEV: Internal symbol must start with Q')
    assert(not Qsymbols_later[name] and not Qsymbols[name],'DEV: Internal symbol already defined: '..name)
    local lread=require'nelisp.lread'
    local sym=lread.define_symbol(symname)
    Qsymbols_later[name]=sym
end
function vars.commit_qsymbols()
    Qsymbols=setmetatable(Qsymbols_later,{__index=Qsymbols})
    Qsymbols_later={}
end

local Vsymbol_pre_values={}
local Vsymbols={}
---@param name string
---@param symname string?
---@param doc string?
function vars.defvar_lisp(name,symname,doc)
    assert(not name:match('%-'),'DEV: Internal variable names must not contain -')
    assert(not symname or not symname:match('_'),'DEV: Internal variable symbol names must should probably not contain _')
    assert(name:sub(1,1)~='V','DEV: Internal variable names must not start with V')
    assert(not Vsymbols[name],'DEV: Internal variable already defined: '..name)
    local lread=require'nelisp.lread'
    local sym
    if symname then
        sym=lread.define_symbol(symname)
    else
        sym=symbol.make_uninterned(str.make(name,false))
    end
    Vsymbols[name]=sym
    if doc then
        if not _G.nelisp_later then
            error('TODO')
        end
    end
    if Vsymbol_pre_values[name] then
        symbol.set_var(sym,Vsymbol_pre_values[name])
        Vsymbol_pre_values[name]=nil
    end
end
---@param doc string?
---@param name string
---@param symname string
function vars.defvar_bool(name,symname,doc)
    vars.defvar_lisp(name,symname,doc)
    vars.V.byte_boolean_vars=cons.make(Vsymbols[name],vars.V.byte_boolean_vars)
end
vars.V=setmetatable({},{__index=function (_,k)
    if not Vsymbols[k] and not Vsymbol_pre_values[k] then
        error('DEV: Not an internal variable or not set: '..tostring(k))
    elseif not Vsymbols[k] then
        return Vsymbol_pre_values[k]
    end
    return assert(symbol.get_var(Vsymbols[k]),'DEV: Internal variable not set: '..tostring(k))
end,__newindex=function (_,k,v)
        assert(type(v)=='table' and type(v[1])=='number')
        if not Vsymbols[k] then
            Vsymbol_pre_values[k]=v
            return
        end
        symbol.set_var(Vsymbols[k],v)
    end})

vars.F={}
---@param name string
---@param map {[string]:{[1]:string,[2]:number,[3]:number,[4]:number,[5]:string,f:fun(...:nelisp.obj):nelisp.obj}}
function vars.setsubr(map,name)
    assert(not vars.F[name],'DEV: internal function already defined: '..name)
    local d=assert(map[name])
    local f=assert(d.f)
    assert(type(d[1])=='string')
    assert(type(d[2])=='number')
    assert(type(d[3])=='number')
    assert(type(d[4])=='number')
    assert(type(d[5])=='string')
    assert(type(d.f)=='function')
    assert(#d==5)
    vars.F[name]=f
    local lread=require'nelisp.lread'
    local symname=d[1]
    if d[3]>=0 and d[3]<=8 then
        assert(debug.getinfo(f,'u').nparams==d[3])
    else
        assert(debug.getinfo(f,'u').nparams==1)
    end
    local sym=lread.lookup_or_make(symname)
    local s=subr.make(f,d[2],d[3],symname,d[4],d[5])
    symbol.set_func(sym,s)
end

return setmetatable(vars,{__index=function (_,k)
    if Qsymbols[k] then
        return Qsymbols[k]
    end
    error('DEV: try to index out of bounds: '..tostring(k))
end})
