local types=require'nelisp.obj.types'
local function inspect(obj,has_visited)
    has_visited=has_visited or {}
    if has_visited[obj] then
        return '<cycle>'
    end
    local out=''
    if obj[1]==types.cons then
        has_visited[obj]=true
        out=out..'('
        local has_visit={}
        while obj[1]==types.cons do
            local i=obj[2]
            obj=obj[3]
            out=out..inspect(i,has_visited)..' '
            if has_visit[obj] then
                out=out..'...)'
                break
            end
            has_visit[obj]=true
        end
        out=out:sub(1,-2)
        if obj[2]~='nil' then
            out=out..' . '..inspect(obj,has_visited)
        end
        out=out..')'
    elseif obj[1]==types.vec then
        out=out..'['
        for _,i in ipairs(obj[2]) do
            out=out..inspect(i)..' '
        end
        out=out:sub(1,-2)
        out=out..']'
    elseif obj[1]==types.str then
        out=out..'"'
        out=out..obj[2]
        out=out..'"'
    elseif obj[1]==types.symbol then
        out=out..obj[2][2]
    elseif obj[1]==types.fixnum then
        out=out..tostring(obj[2])
    else
        out=out..'{'
        for k,v in vim.spairs(obj) do
            if k==1 then
                out=out..'type='..types.reverse[v]..',content='
            else
                out=out..vim.inspect(v)..', '
            end
        end
        out=out:sub(1,-3)
        out=out..'}'
    end
    return out
end
local function p(objs)
    if type(objs)=='number' then
        print(string.char(objs))
        return
    end
    if type(objs[1])=='number' then
        objs={objs}
    end
    for _,v in ipairs(objs) do
        print(inspect(v))
    end
end
return {p=p,inspect=inspect}
