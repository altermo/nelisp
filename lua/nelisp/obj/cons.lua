local types=require'nelisp.obj.types'

---@class nelisp.cons: nelisp.obj
---@field [1] nelisp.types.cons
---@field [2] nelisp.obj
---@field [3] nelisp.obj

local M={}
---@param car nelisp.obj
---@param cdr nelisp.obj
---@return nelisp.cons
function M.make(car,cdr)
    ---@type nelisp.cons
    return {
        types.cons --[[@as nelisp.types.cons]],
        car,
        cdr,
    }
end
---@param cons nelisp.cons
---@param cdr nelisp.obj
function M.setcdr(cons,cdr)
    cons[3]=cdr
end
---@param cons nelisp.cons
---@param car nelisp.obj
function M.setcar(cons,car)
    cons[2]=car
end
---@param cons nelisp.cons
---@return nelisp.obj
function M.cdr(cons)
    return cons[3]
end
---@param cons nelisp.cons
---@return nelisp.obj
function M.car(cons)
    return cons[2]
end
return M
