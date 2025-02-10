local env={
    obj_mt={},
    memtbl=setmetatable({},{__mode='v'}),
}

local path

local M=vim.defaulttable(function (k)
    if not path then
        path=vim.api.nvim_get_runtime_file('nelisp.so',false)[1]
    end
    assert(path,'nelisp.so not found in runtimepath')
    local f=package.loadlib(path,k)
    assert(f,k..' not found')
    debug.setfenv(f,env)
    return f
end)

M._memtbl=env.memtbl

return M
