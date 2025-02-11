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

---@param str nelisp.obj
---@return string
function M.unibyte_lstring_to_string(str) end

---@param cons nelisp.obj
---@return [nelisp.obj,nelisp.obj]
function M.cons_to_table(cons) end

function M.collectgarbage() end

return M
