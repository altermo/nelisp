local vars=require'nelisp.vars'
local cons=require'nelisp.obj.cons'
local M={}

local F={}
F.list={'list',0,-2,0,[[Return a newly created list with specified arguments as elements.
Allows any number of arguments, including zero.
usage: (list &rest OBJECTS)]]}
function F.list.f(args)
    local val=vars.Qnil
    for _,arg in ipairs(args) do
        val=vars.F.cons(arg,val)
    end
    return val
end
F.cons={'cons',2,2,0,[[Create a new cons, give it CAR and CDR as components, and return it.]]}
function F.cons.f(car,cdr)
    return cons.make(car,cdr)
end

function M.init_syms()
    vars.setsubr(F,'list')
    vars.setsubr(F,'cons')
end
return M
