local types=require'nelisp.obj.types'

---@class nelisp.bignum: nelisp.obj
---@field [1] nelisp.types.fixnum
---@field [2] number[]

local M={}
---@param str string
---@return nelisp.bignum
function M.make(str)
    error('TODO')
end
return M
