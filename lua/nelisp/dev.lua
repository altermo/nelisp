local vars=require'nelisp.vars'
local specpdl=require'nelisp.specpdl'
local lisp=require'nelisp.lisp'
local print_=require'nelisp.print'
local M={}

local inspect=function (x)
    local printcharfun=print_.make_printcharfun()
    print_.print_obj(x,true,printcharfun)
    return printcharfun.out()
end

---@type nelisp.F
local F={}
F.Xprint={'!print',1,1,0,[[internal function]]}
function F.Xprint.f(x)
    if type(lisp.xtype(x))=='table' then
        for k,v in ipairs(x) do
            print(k..' : '..inspect(v))
        end
    elseif #x==0 then
        print('(!print): empty table')
    else
        print(inspect(x))
    end
    return vars.Qt
end
F.Xbacktrace={'!backtrace',0,0,0,[[internal function]]}
function F.Xbacktrace.f()
    print'Backtrace:'
    for entry in specpdl.riter() do
        if entry.type==specpdl.type.backtrace then
            local str_args={}
            for _,arg in ipairs(entry.args) do
                if not pcall(function ()
                    table.insert(str_args,inspect(arg))
                end) then
                    table.insert(str_args,'...')
                end
            end
            ---@cast entry nelisp.specpdl.backtrace_entry
            print('  ('..inspect(entry.func)..' '..table.concat(str_args,' ')..')')
        end
    end
    return vars.Qt
end
F.Xerror={'!error',0,1,0,[[internal function]]}
function F.Xerror.f(x)
    error(inspect(x))
    return vars.Qt
end
F.Xlua_exec={'!lua-exec',1,-2,0,[[internal function]]}
function F.Xlua_exec.fa(args)
    local x=args[1]
    lisp.check_string(x)
    local s=lisp.sdata(x)
    loadstring(s)(unpack(args,2))
    return vars.Qnil
end

function M.init_syms()
    vars.defsubr(F,'Xprint')
    vars.defsubr(F,'Xbacktrace')
    vars.defsubr(F,'Xerror')
    vars.defsubr(F,'Xlua_exec')
end
return M
