local types=require'nelisp.obj.types'

---@class nelisp.buffer: nelisp.obj
---@field [1] nelisp.types.buffer
---@field [number] any

local M={}
---@return nelisp.buffer
function M.make(...)
    ---@type nelisp.buffer
    return {
        types.buffer --[[@as nelisp.types.buffer]],
        ...
    }
end
return M
