local types=require'nelisp.obj.types'

---@class nelisp.float: nelisp.obj
---@field [1] nelisp.types.float
---@field [2] number

local M={}
---@param num number
---@return nelisp.float
function M.make(num)
    ---@type nelisp.float
    return {
        types.float --[[@as nelisp.types.float]],
        num,
    }
end
return M
