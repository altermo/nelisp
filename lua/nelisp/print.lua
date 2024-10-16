local vars=require'nelisp.vars'
local lisp=require'nelisp.lisp'
local specpdl=require'nelisp.specpdl'
local types=require'nelisp.obj.types'
local hash_table=require'nelisp.obj.hash_table'
local str=require'nelisp.obj.str'
local fixnum=require'nelisp.obj.fixnum'
local M={}

---@param obj nelisp.obj
---@param escapeflag boolean
---@param out string[]
local function obj_to_string(obj,escapeflag,out)
    if not _G.nelisp_later then
        error('TODO: implement print_depth')
    end
    local print_depth=0
    local base_depth=print_depth
    local stack={}
    local typ=types.type(obj)
    if typ==types.hash_table then
        local size=0
        local t=hash_table.to_table(obj)
        for _ in pairs(t.content) do
            size=size+1
        end
        table.insert(out,('#s(hash-table size %d)'):format(size))
        if t.test then
            error('TODO')
        end
        if t.weak then
            error('TODO')
        end
        table.insert(out,' rehash-size 1 rehash-threshold 1 data(')
        table.insert(stack,{
            t='hash',
            obj=obj,
            content=t.content,
            idx=0,
            printed=0,
            nobjs=size,
        })
        goto next_obj
    else
        error('TODO')
    end
    ::next_obj::
    while #stack>0 do
        local e=stack[#stack]
        if e.t=='hash' then
            if e.printed>=e.nobjs then
                table.insert(out,')')
                table.remove(stack)
                goto next_obj
            end
            error('TODO')
        else
            error('TODO')
        end
    end
    assert(print_depth==base_depth)
end

local F={}
F.prin1_to_string={'prin1-to-string',1,3,0,[[Return a string containing the printed representation of OBJECT.
OBJECT can be any Lisp object.  This function outputs quoting characters
when necessary to make output that `read' can handle, whenever possible,
unless the optional second argument NOESCAPE is non-nil.  For complex objects,
the behavior is controlled by `print-level' and `print-length', which see.

OBJECT is any of the Lisp data types: a number, a string, a symbol,
a list, a buffer, a window, a frame, etc.

See `prin1' for the meaning of OVERRIDES.

A printed representation of an object is text which describes that object.]]}
function F.prin1_to_string.f(obj,noescape,overrides)
    if not _G.nelisp_later then
        error('TODO')
    end
    --local count=specpdl.index()
    if not lisp.nilp(overrides) then
        error('TODO')
    end
    local out={}
    obj_to_string(obj,noescape,out)
    obj=str.make(table.concat(out),'auto')
    return obj
    --return specpdl.unbind_to(count,obj)
end

function M.init_syms()
    vars.setsubr(F,'prin1_to_string')
end
return M
