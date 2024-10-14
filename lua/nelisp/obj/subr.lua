local types=require'nelisp.obj.types'

---@class nelisp.subr: nelisp.obj
---@field [1] nelisp.types.subr
---@field [2] fun(...:nelisp.obj):nelisp.obj
---@field [3] number minargs
---@field [4] number maxargs
---@field [5] string symbol_name
---@field [6] string? --intspec
---@field [7] number? --docs

local M={}
---@param fun fun(...:nelisp.obj):nelisp.obj
---@param minargs number
---@param maxargs number
---@param symbol_name string
---@param intspec string|0
---@param docs string
---@return nelisp.subr
function M.make(fun,minargs,maxargs,symbol_name,intspec,docs)
    if not _G.nelisp_later then
        _=docs
        error('TODO')
    end
    ---@type nelisp.subr
    return {
        types.subr --[[@as nelisp.types.subr]],
        fun,
        minargs,
        maxargs,
        symbol_name,
        intspec~=0 and intspec or nil --[[@as string?]],
    }
end
---@param s nelisp.subr
---@return {fun:(fun(...:nelisp.obj):nelisp.obj),minargs:number,maxargs:number,symbol_name:string,intspec:string?,doc:number?}
function M.totable(s)
    return {
        fun=s[2],
        minargs=s[3],
        maxargs=s[4],
        symbol_name=s[5],
        intspec=s[6],
        docs=s[7],
    }
end
return M
