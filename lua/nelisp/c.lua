local env={
    obj_mt={},
    memtbl=setmetatable({},{__mode='v'}),
}

local M=vim.defaulttable(function (k)
    local f=package.loadlib(NELISP_SO_PATH,k)
    assert(f,k..' not found')
    debug.setfenv(f,env)
    return f
end)

M._memtbl=env.memtbl

return M
