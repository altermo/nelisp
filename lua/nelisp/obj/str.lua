local types=require'nelisp.obj.types'
local b=require'nelisp.bytes'

---@class nelisp.str: nelisp.obj
---@field [1] nelisp.types.str
---@field [2] string
---@field [3] any --text properties
---@field [4]? boolean --multibyte
---@class nelisp.str.multibyte: nelisp.str
---@field [2] number[]
---@field [4] true
---@class nelisp.str.unibyte: nelisp.str
---@field [2] string
---@field [4] false|nil

local M={}
---@param str string
---@param multibyte boolean|'auto'
---@return nelisp.str
function M.make(str,multibyte)
    if not _G.nelisp_later then
        assert(not multibyte,'TODO')
    else
        multibyte=false
    end
    ---@type nelisp.str
    return {
        types.str --[[@as nelisp.types.str]],
        str,
    }
end

---@param obj nelisp.str
---@param idx number
---@return number
function M.index1(obj,idx)
    --assert(idx>=1 and idx<=#obj[2])
    return assert(b(obj[2] --[[@as string]] :sub(idx,idx)))
end
---@param obj nelisp.str
---@param idx number
---@return number
function M.index1_neg(obj,idx)
    local err,res=pcall(M.index1,obj,idx)
    return err and res or -1
end
---@param obj nelisp.str
---@return string
function M.to_lua_string(obj)
    return obj[2]
end

---@param obj nelisp.str
---@return boolean
function M.is_multibyte(obj)
    return obj[3]==true
end
return M
