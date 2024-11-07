local lisp=require'nelisp.lisp'
local vars=require'nelisp.vars'
local alloc=require'nelisp.alloc'
local lread=require'nelisp.lread'
---@param obj nelisp.obj
---@return string
local function escapestr(obj)
    local s=lisp.sdata(obj)
    return ('%q'):format(s)
end
---@param obj nelisp.obj
---@param printcharfun nelisp.print.printcharfun
---@return "DONT RETURN"
local function compile(obj,printcharfun)
    printcharfun.print_depth=printcharfun.print_depth+1
    if not _G.nelisp_later then
        error('TODO: recursive/circular check')
        error('TODO: reuse reused objects')
    end
    local typ=lisp.xtype(obj)
    if typ==lisp.type.symbol then
        if obj==vars.Qnil then
            printcharfun.write('NIL')
        elseif obj==vars.Qt then
            printcharfun.write('T')
        else
            if lisp.symbolinternedininitialobarrayp(obj) then
                printcharfun.write('S('..escapestr(lisp.symbol_name(obj))..')')
            else
                error('TODO')
            end
        end
    elseif typ==lisp.type.int0 then
        printcharfun.write('INT('..('%d'):format(lisp.fixnum(obj))..')')
    elseif typ==lisp.type.float then
        printcharfun.write('FLOAT('..('%f'):format(lisp.xfloat_data(obj))..')')
    elseif typ==lisp.type.string then
        assert(lisp.string_intervals(obj)==nil)
        printcharfun.write('STR('..escapestr(obj))
        if lisp.string_multibyte(obj) then
            printcharfun.write(','..lisp.schars(obj))
        end
        printcharfun.write(')')
    elseif typ==lisp.type.cons then
        local elems={}
        local tail=obj
        while lisp.consp(tail) do
            table.insert(elems,lisp.xcar(tail))
            tail=lisp.xcdr(tail)
        end
        printcharfun.write('C(')
        for _,elem in ipairs(elems) do
            compile(elem,printcharfun)
            printcharfun.write(',')
        end
        compile(tail,printcharfun)
        printcharfun.write(')')
    elseif typ==lisp.type.vectorlike then
        if lisp.compiledp(obj) then
            printcharfun.write('COMP(')
            for i=0,lisp.asize(obj)-1 do
                if i~=0 then
                    printcharfun.write(',')
                end
                compile(lisp.aref(obj,i),printcharfun)
            end
            printcharfun.write(')')
        elseif lisp.vectorp(obj) then
            printcharfun.write('V(')
            for i=0,lisp.asize(obj)-1 do
                if i~=0 then
                    printcharfun.write(',')
                end
                compile(lisp.aref(obj,i),printcharfun)
            end
            printcharfun.write(')')
        elseif lisp.hashtablep(obj) then
            local ht=(obj --[[@as nelisp._hash_table]])
            printcharfun.write('HASHTABEL{')
            printcharfun.write('SIZE=')
            printcharfun.write(('%d'):format(lisp.asize(ht.next)))
            printcharfun.write(',')
            if not lisp.nilp(ht.test.name) then
                printcharfun.write('TEST=')
                compile(ht.test.name,printcharfun)
                printcharfun.write(',')
            end
            if not lisp.nilp(ht.weak) then
                printcharfun.write('WEAK=')
                compile(ht.weak,printcharfun)
                printcharfun.write(',')
            end
            printcharfun.write('REHASH_SIZE=')
            compile(vars.F.hash_table_rehash_size(obj),printcharfun)
            printcharfun.write(',')
            printcharfun.write('REHASH_THRESHOLD=')
            compile(vars.F.hash_table_rehash_threshold(obj),printcharfun)
            printcharfun.write(',')
            printcharfun.write('DATA={')
            local idx=0
            for _=0,ht.count-1 do
                while lisp.aref(ht.key_and_value,idx*2)==vars.Qunique do
                    idx=idx+1
                end
                compile(lisp.aref(ht.key_and_value,idx*2),printcharfun)
                printcharfun.write(',')
                compile(lisp.aref(ht.key_and_value,idx*2+1),printcharfun)
                printcharfun.write(',')
                idx=idx+1
            end
            assert(idx==ht.count)
            printcharfun.write('}}')
        else
            error('TODO')
        end
    end
    printcharfun.print_depth=printcharfun.print_depth-1
    return 'DONT RETURN'
end
local M={}
---@param obj nelisp.obj
---@return string
local function compile_obj(obj)
    local printcharfun=require'nelisp.print'.make_printcharfun()
    compile(obj,printcharfun)
    return printcharfun.out()
end
---@param objs nelisp.obj[]
---@return string[]
function M.compiles(objs)
    local out={}
    table.insert(out,'return {')
    for _,v in ipairs(objs) do
        table.insert(out,compile_obj(v)..',')
    end
    table.insert(out,'}')
    return out
end
local globals={
    C=function (...)
        local args={...}
        local val=args[#args]
        for i=#args-1,1,-1 do
            val=alloc.cons(args[i],val)
        end
        return val
    end,
    S=function (s)
        return lread.intern(s)
    end,
    STR=function (s,nchars)
        if nchars then
            return alloc.make_multibyte_string(s,nchars)
        end
        return alloc.make_unibyte_string(s)
    end,
    INT=function (n)
        return lisp.make_fixnum(n)
    end,
    V=function (...)
        local args={...}
        local vec=alloc.make_vector(#args,'nil')
        for i=0,#args-1 do
            lisp.aset(vec,i,args[i+1])
        end
        return vec
    end,
    COMP=function (...)
        return lread.bytecode_from_list({...},{})
    end,
    T=vars.Qt,
    NIL=vars.Qnil,
    HASHTABEL=function (opts)
        local args={}
        assert(opts.SIZE)
        table.insert(args,vars.Qsize)
        table.insert(args,lisp.make_fixnum(opts.SIZE))
        if opts.TEST then
            table.insert(args,vars.Qtest)
            table.insert(args,opts.TEST)
        end
        if opts.WEAK then
            table.insert(args,vars.Qweakness)
            table.insert(args,opts.WEAK)
        end
        assert(opts.REHASH_SIZE)
        table.insert(args,vars.Qrehash_size)
        table.insert(args,opts.REHASH_SIZE)
        assert(opts.REHASH_THRESHOLD)
        table.insert(args,vars.Qrehash_threshold)
        table.insert(args,opts.REHASH_THRESHOLD)
        assert(opts.DATA)
        table.insert(args,vars.Qdata)
        table.insert(args,lisp.list(unpack(opts.DATA)))
        return lread.hash_table_from_plist(lisp.list(unpack(args)))
    end,
    FLOAT=function (a)
        return alloc.make_float(a)
    end,
}
---@param code string
function M.read(code)
    local f=assert(loadstring(code))
    debug.setfenv(f,globals)
    return f()
end
return M
