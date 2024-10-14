local types=require'nelisp.obj.types'

---@class nelisp.vec: nelisp.obj
---@field [1] nelisp.types.vec
---@field [2] nelisp.obj[]

local M={}
---@param elems nelisp.obj[]
function M.make(elems)
    ---@type nelisp.vec
    return {
        types.vec --[[@as nelisp.types.vec]],
        elems
    }
end
---@param vec nelisp.vec
---@return number
function M.length(vec)
    return #vec[2]
end
---@param vec nelisp.vec
---@param idx number
---@return nelisp.obj?
function M.index0(vec,idx)
    assert(idx>=0 and idx<#vec[2])
    return vec[2][idx+1]
end
---@param vec nelisp.vec
---@param idx number
---@param val nelisp.obj
function M.set_index0(vec,idx,val)
    assert(idx>=0 and idx<#vec[2])
    vec[2][idx+1]=val
end
return M
