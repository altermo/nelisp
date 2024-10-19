local types=require'nelisp.obj.types'

---@class nelisp.compiled: nelisp.obj
---@field [1] nelisp.types.compiled
---@field [2] nelisp.obj[]

local M={}
M.idx={
    arglist=1,
    bytecode=2,
    constants=3,
    stack_depth=4,
    doc_string=5,
    interactive=6
}
---@param elems nelisp.obj[]
---@return nelisp.compiled
function M.make(elems)
    ---@type nelisp.compiled
    return {
        types.compiled --[[@as nelisp.types.compiled]],
        elems,
    }
end
return M
