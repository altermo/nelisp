local vars=require'nelisp.vars'
local specpdl=require'nelisp.specpdl'
local lisp=require'nelisp.lisp'
local print_=require'nelisp.print'
local M={}

local function convert_from_lua(x)
    local fixnum=require'nelisp.obj.fixnum'
    local cons=require'nelisp.obj.cons'
    local str=require'nelisp.obj.str'
    if type(x)=='number' then
        return fixnum.make(x)
    elseif type(x)=='nil' then
        return vars.Qnil
    elseif type(x)=='boolean' then
        return x and vars.Qt or vars.Qnil
    elseif type(x)=='table' then
        if not vim.islist(x) then
            error('Non list tables can\'t be converted')
        end
        local ret=vars.Qnil
        for _,v in ipairs(x) do
            ret=cons.make(convert_from_lua(v) --[[TODO: infinite recursion]],ret)
        end
        return ret
    elseif type(x)=='string' then
        return str.make(x,'auto')
    elseif type(x)=='function' then
        error('Conversion from type not supported: '..type(x))
        error('TODO')
    else
        error('Conversion from type not supported: '..type(x))
    end
end

local inspect=function (x)
    local printcharfun=print_.make_printcharfun()
    print_.print_obj(x,true,printcharfun)
    return printcharfun.out()
end

local F={}
F.Xprint={'!print',1,1,0,[[internal function]]}
function F.Xprint.f(x)
    print(inspect(x))
    return vars.Qt
end
F.Xbacktrace={'!backtrace',0,0,0,[[internal function]]}
function F.Xbacktrace.f()
    print'Backtrace:'
    for entry in specpdl.riter() do
        if entry.type==specpdl.type.backtrace then
            ---@cast entry nelisp.specpdl.backtrace_entry
            print('  ('..inspect(entry.func)..' ...)')
        end
    end
    return vars.Qt
end
F.Xerror={'!error',0,1,0,[[internal function]]}
function F.Xerror.f(x)
    error(inspect(x))
    return vars.Qt
end
F.Xlua_eval={'!lua-eval',1,1,0,[[internal function]]}
function F.Xlua_eval.f(x)
    --TODO: maybe support arguments which get converted from nelisp to lua
    lisp.check_string(x)
    local s=lisp.sdata(x)
    return convert_from_lua(loadstring('return '..s)())
end

function M.init_syms()
    vars.setsubr(F,'Xprint')
    vars.setsubr(F,'Xbacktrace')
    vars.setsubr(F,'Xerror')
    vars.setsubr(F,'Xlua_eval')
end
return M
