---@meta

---@class nelisp.obj
---@field [1] nelisp.types

---@enum nelisp.types
local types={
    symbol=0,
    fixnum=1,
    str=2,
    vec=3,
    cons=4,
    float=5,
    _normal_vector=100,
    _free=101,
    bignum=102,
    marker=103,
    overlay=104,
    finalizer=105,
    symbol_with_pos=106,
    _misc_ptr=107,
    user_ptr=108,
    process=109,
    frame=110,
    window=111,
    bool_vector=112,
    buffer=113,
    hash_table=114,
    obarray=115,
    terminal=116,
    window_configuration=117,
    subr=118,
    _other=119,
    xwidget=120,
    xwidget_view=121,
    thread=122,
    mutex=123,
    condvar=124,
    module_function=125,
    native_comp_unit=126,
    ts_parser=127,
    ts_node=128,
    ts_compiled_query=129,
    sqlite=130,
    closure=131,
    char_table=132,
    sub_char_table=133,
    compiled=134,
    record=135,
    font=136,
}
local M=setmetatable({},{__index=types})
M.reverse={}
for k,v in pairs(types) do
    M.reverse[v]=k
end

---@param obj nelisp.obj
---@return nelisp.types
function M.type(obj)
    return obj[1]
end
---@param t nelisp.types
function M.vectorlike_p(t)
    return t>=types._normal_vector or t==types.vec
end

---@alias nelisp.nil nelisp.symbol
---@alias nelisp.list nelisp.nil|nelisp.cons

return M
