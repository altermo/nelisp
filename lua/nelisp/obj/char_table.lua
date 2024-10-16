local types=require'nelisp.obj.types'
local vars=require'nelisp.vars'

---@class nelisp.char_table: nelisp.obj
---@field [1] nelisp.types.char_table
---@field [2] table<number,nelisp.obj> --content
---@field [3] nelisp.obj --purpose
---@field [4] nelisp.obj? --default
---@field [5] nelisp.obj? --parent
---@field [6] number? extras

local M={}
---@param purpose nelisp.obj
---@param extras number
---@param default nelisp.obj?
---@param parent nelisp.obj?
---@return nelisp.char_table
function M.make(purpose,extras,default,parent)
    assert(extras==nil or (0<=extras and extras<10))
    ---@type nelisp.char_table
    return {
        types.char_table --[[@as nelisp.types.char_table]],
        {},
        purpose,
        default~=vars.Qnil and default or nil,
        parent,
        extras~=0 and extras or nil,
    }
end
---@param t nelisp.char_table
---@param c number
---@param val nelisp.obj
function M.set(t,c,val)
    if not _G.nelisp_later then
        error('TODO')
    end
    t[2][c]=val
end
---@param t nelisp.char_table
---@param c number
---@return nelisp.obj
function M.get(t,c)
    if not _G.nelisp_later then
        error('TODO')
    end
    return t[2][c] or t[4] or vars.Qnil
end
return M
