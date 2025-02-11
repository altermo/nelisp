---@meta nelisp.c

---@class nelisp.obj

local M={}

---@type table<string,nelisp.obj>
M._memtbl={}

---@param n number
---@return nelisp.obj
function M.number_to_fixnum(n) end

---@param n number
---@return nelisp.obj
function M.number_to_float(n) end

---@param n nelisp.obj
---@return number
function M.fixnum_to_number(n) end

---@param n nelisp.obj
---@return number
function M.float_to_number(n) end

---@param str string
---@return nelisp.obj
function M.string_to_unibyte_lstring(str) end

function M.collectgarbage() end

return M
