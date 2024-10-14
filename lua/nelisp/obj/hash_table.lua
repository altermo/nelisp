local types=require'nelisp.obj.types'

---@class nelisp.hash_table: nelisp.obj
---@field [1] nelisp.types.hash_table
---@field [2] table --content
---@field [3] nelisp.obj? --test
---@field [4] nelisp.obj? --weak
---@field [5] boolean? --not-mutable

local M={}
---@param test table?
---@param weak nelisp.obj?
---@return nelisp.hash_table
function M.make(test,weak)
    ---@type nelisp.hash_table
    return {
        types.hash_table --[[@as nelisp.types.hash_table]],
        {},
        test,
        weak,
    }
end
function M.put(t,key,value)
    t[2][key]=value
end
return M
