local types=require'nelisp.obj.types'

---@class nelisp.fixnum: nelisp.obj
---@field [1] nelisp.types.fixnum
---@field [2] number

local M={}
---@type table<number,nelisp.fixnum>
local cache=setmetatable({},{__mode='v'})
---@param num number
---@return nelisp.fixnum
function M.make(num)
    if not cache[num] then
        ---@type nelisp.fixnum
        cache[num]={
            types.fixnum --[[@as nelisp.types.fixnum]],
            num,
        }
    end
    return cache[num]
end
M.zero=M.make(0)

---@param x nelisp.fixnum
---@return number
function M.tonumber(x)
    return x[2]
end
return M
