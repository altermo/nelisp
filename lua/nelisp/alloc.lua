local vars=require'nelisp.vars'
local cons=require'nelisp.obj.cons'
local M={}

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
F.cons={'cons',2,2,0,[[Create a new cons, give it CAR and CDR as components, and return it.]]}
function F.cons.f(car,cdr)
    return cons.make(car,cdr)
end
F.purecopy={'purecopy',1,1,0,[[Make a copy of object OBJ in pure storage.
Recursively copies contents of vectors and cons cells.
Does not copy symbols.  Copies strings without text properties.]]}
function F.purecopy.f(obj)
    return obj
end

function M.init_syms()
    vars.setsubr(F,'list')
    vars.setsubr(F,'cons')
    vars.setsubr(F,'purecopy')

    vars.defsym('Qchar_table_extra_slots','char-table-extra-slots')
end
return M
