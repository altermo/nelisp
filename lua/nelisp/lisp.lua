local types=require'nelisp.obj.types'
local cons=require'nelisp.obj.cons'
local vars=require'nelisp.vars'
local type_of=types.type
local M={}
---@overload fun(x:nelisp.obj):boolean
function M.symbolp(x)
    assert(type_of(x)~=types.symbol_with_pos,'TODO')
    return type_of(x)==types.symbol
end
---@overload fun(x:nelisp.obj):boolean
function M.subr_native_compiled_dynp(_) return false end
---@overload fun(x:nelisp.obj):boolean
function M.integerp(x) return M.fixnump(x) or M.bignump(x) end

---@overload fun(x:nelisp.obj):boolean
function M.baresymbolp(x) return type_of(x)==types.symbol end
---@overload fun(x:nelisp.obj):boolean
function M.vectorp(x) return type_of(x)==types.vec end
---@overload fun(x:nelisp.obj):boolean
function M.stringp(x) return type_of(x)==types.str end
---@overload fun(x:nelisp.obj):boolean
function M.consp(x) return type_of(x)==types.cons end
---@overload fun(x:nelisp.obj):boolean
function M.subrp(x) return type_of(x)==types.subr end
---@overload fun(x:nelisp.obj):boolean
function M.fixnump(x) return type_of(x)==types.fixnum end
---@overload fun(x:nelisp.obj):boolean
function M.bignump(x) return type_of(x)==types.bignum end
---@overload fun(x:nelisp.obj):boolean
function M.bufferp(x) return type_of(x)==types.buffer end
---@overload fun(x:nelisp.obj):boolean
function M.compiledp(x) return type_of(x)==types.compiled end
---@overload fun(x:nelisp.obj):boolean
function M.module_functionp(x) return type_of(x)==types.module_function end
---@overload fun(x:nelisp.obj):boolean
function M.chartablep(x) return type_of(x)==types.char_table end

---@overload fun(x:nelisp.obj):boolean
function M.nilp(x) return x==vars.Qnil end


---@param x nelisp.obj
---@param y nelisp.obj
---@return boolean
function M.eq(x,y)
    assert(type_of(x)~=types.symbol_with_pos,'TODO')
    assert(type_of(y)~=types.symbol_with_pos,'TODO')
    return x==y
end

local function wrong_type_argument(predicate,x)
    _={predicate,x}
    --TODO
    error('No error handling yet, error: wrong_type_argument}')
end

---@param ok boolean
---@param predicate nelisp.obj
---@param x nelisp.obj
function M.check_type(ok,predicate,x)
    if not ok then
        wrong_type_argument(predicate,x)
    end
end
---@overload fun(x:nelisp.obj)
function M.check_list(x) M.check_type(M.consp(x) or M.nilp(x),vars.Qlistp,x) end
---@overload fun(x:nelisp.obj,y:nelisp.obj)
function M.check_list_end(x,y) M.check_type(M.nilp(x),vars.Qlistp,y) end
---@overload fun(x:nelisp.obj)
function M.check_symbol(x) M.check_type(M.symbolp(x),vars.Qsymbolp,x) end
---@overload fun(x:nelisp.obj)
function M.check_integer(x) M.check_type(M.integerp(x),vars.Qintegerp,x) end
---@overload fun(x:nelisp.obj)
function M.check_string(x) M.check_type(M.stringp(x),vars.Qstringp,x) end
---@overload fun(x:nelisp.obj)
function M.check_cons(x) M.check_type(M.consp(x),vars.Qconsp,x) end

---@param x nelisp.obj
---@param fn fun(x:nelisp.cons):'continue'|'break'|nelisp.obj?
function M.for_each_tail(x,fn)
    local has_visited={}
    while M.consp(x) do
        if has_visited[x] then
            error('TODO: err')
        end
        has_visited[x]=true
        ---@cast x nelisp.cons
        local result=fn(x)
        if result=='break' then
            return nil,x
        elseif result=='continue' then
        elseif result then
            return result --[[@as nelisp.obj]],x
        end
        x=cons.cdr(x)
    end
    return nil,x
end
---@param x nelisp.obj
---@return number
function M.list_length(x)
    local i=0
    local _,list=M.for_each_tail(x,function ()
        i=i+1
    end)
    M.check_list_end(list,list)
    return i
end

---@param x nelisp.obj
function M.loadhist_attach(x)
    if not _G.nelisp_later then
        error('TODO')
    end
end

return M
