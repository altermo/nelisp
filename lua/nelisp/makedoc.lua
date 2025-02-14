#!/usr/bin/env -S nvim -l
---@alias nelisp.makedoc.lfn table<string,[string,[string,string][]]>
---                                name    ret    args
---@alias nelisp.makedoc.cfn table<string,number>
---                                name   maxargs
---@alias nelisp.makedoc.sym table<string,string>
---                                name   sname
---@alias nelisp.makedoc.var table<string,string>
---                                name   type
local map={
    string={'string','str'},
    number={'number','n'},
    object={'nelisp.obj','obj'},
}
---@param path string
---@return nelisp.makedoc.lfn
local function scan_lua_c(path)
    local row=0
    local function err(msg)
        error(('%s:%s: %s'):format(path,row,msg))
    end
    local f=io.lines(path)
    ---@return string
    local function readline()
        row=row+1
        return f()
    end
    local out={}
    for line in readline do
        if not line:match('^int pub') then
            goto continue
        end
        local ret,name=line:match('^int pub ret(%b()) ([a-zA-Z_1-9]*)%(lua_State %*L%) *{ *$')
        if not ret then
            err('pub function bad format')
        end
        ret=ret:sub(2,-2)
        if ret:match('/%*(.*)%*/') then
            ret=ret:match('/%*(.*)%*/')
        end
        if ret=='' then
            ret=nil
        end
        if not readline():match('^%s*global_lua_state%s*=%s*L;$') then
            err('pub function bad format')
        end
        local nargs=readline():match('^%s*check_nargs%(L,%s*(%d+)%s*%);$')
        if not nargs then
            err('pub function bad format')
        end
        local typ_map={}
        local args={}
        for _=1,tonumber(nargs) do
            local typ=readline():match('^%s*check_is(%w*)%b()')
            if not typ then
                err('pub function bad format')
            end
            if type(typ_map[typ])=='table' then
                typ_map[typ][2]=map[typ][2]..'1'
                typ_map[typ]=2
            end
            local varname=map[typ][2]..(typ_map[typ] or '')
            table.insert(args,{map[typ][1],varname})
            if typ_map[typ] then
                typ_map[typ]=typ_map[typ]+1
            else
                typ_map[typ]=args[#args]
            end
        end
        out[name]={ret,args}
        ::continue::
    end
    return out
end
---@param lfn nelisp.makedoc.lfn
---@param out string[]
local function lfn_to_meta(lfn,out)
    for name,info in vim.spairs(lfn) do
        local ret,args=unpack(info)
        table.insert(out,'')
        local arg_vars={}
        for _,v in ipairs(args) do
            table.insert(out,('---@param %s %s'):format(v[2],v[1]))
            table.insert(arg_vars,v[2])
        end
        if ret then
            table.insert(out,('---@return %s'):format(ret))
        end
        table.insert(out,('function M.%s(%s) end'):format(name,table.concat(arg_vars,',')))
    end
end
---@param path string
---@param out {f:nelisp.makedoc.cfn,s:nelisp.makedoc.sym,v:nelisp.makedoc.var}
local function scan_c(path,out)
    local s=vim.fn.readblob(path)
    for pos in s:gmatch('[\n\r]%s*()DEF[UVS]') do
        local typ
        if s:match('^DEFUN',pos) then
            pos=pos+#('DEFUN')
            typ='function'
        elseif s:match('^DEFSYM[ \t(]',pos) then
            typ='symbol'
        elseif s:match('^DEFVAR_',pos) then
            pos=pos+#('DEFVAR_')
            if s:match('^I',pos) then
                pos=pos+1
                typ='integer'
            elseif s:match('^L',pos) then
                pos=pos+1
                typ='object'
            elseif s:match('^BO',pos) then
                pos=pos+2
                typ='boolean'
            end
        end
        if not typ then goto continue end
        pos=s:match('^[^(]*%(()',pos)
        if not pos then goto continue end
        if typ~='symbol' then
            pos=s:match('^"[^"]*"()',pos)
            if not pos then goto continue end
        end
        pos=assert(s:match('^[%s,]*()',pos))
        local name
        name,pos=assert(s:match('^([^%s,]*)()',pos))
        if typ=='symbol' then
            pos=assert(s:match('^[%s,]*()',pos))
            local sname
            sname,pos=s:match('^"([^"]*)"()',pos)
            out.s[name]=sname
            goto continue
        elseif typ~='function' then
            out.v[name]=typ
            goto continue
        end
        local function get_comma()
            pos=s:match('^[^,]*,%s*()',pos)
            if not pos then error('EOF') end
            local val
            val,pos=s:match('^([^,]*)()',pos)
            return val
        end
        get_comma()
        get_comma()
        local maxargs=get_comma()
        if maxargs=='MANY' then
            maxargs=-1
        elseif maxargs=='UNEVALLED' then
            maxargs=-2
        else
            maxargs=tonumber(maxargs)
            if not maxargs then error('') end
        end
        if name~='...' then
            out.f[name]=maxargs
        end
        ::continue::
    end
end
---@param file string
---@return number
local function file_mtime(file)
    local stat=vim.uv.fs_stat(file)
    if not stat then return -math.huge end
    return stat.mtime.sec+stat.mtime.nsec/1000000000
end
local function outfile_needs_update(out,other)
    local out_mtime=file_mtime(out)
    local other_mtime
    if vim.uv.fs_stat(other).type=='directory' then
        other_mtime=-math.huge
        for file in vim.fs.dir(other) do
            other_mtime=math.max(other_mtime,file_mtime(vim.fs.joinpath(other,file)))
        end
    else
        other_mtime=file_mtime(other)
    end
    return out_mtime<other_mtime
end
---@param c {f:nelisp.makedoc.cfn,s:nelisp.makedoc.sym,v:nelisp.makedoc.var}
---@return string[]
local function c_to_globals(c)
    local out={
        '// This file is generated by makedoc.lua',
        'struct emacs_globals {',
    }
    for k,v in vim.spairs(c.v) do
        if v=='object' then
            table.insert(out,('  Lisp_Object f_%s;'):format(k))
        elseif v=='integer' then
            table.insert(out,('  intmax_t f_%s;'):format(k))
        elseif v=='boolean' then
            table.insert(out,('  bool f_%s;'):format(k))
        else
            error('')
        end
        table.insert(out,('#define %s globals.f_%s'):format(k,k))
    end
    table.insert(out,'} globals;')
    table.insert(out,'')
    c.s['Qnil']=nil
    c.s['Qt']=nil
    c.s['Qunbound']=nil
    c.s['Qerror']=nil
    c.s['Qlambda']=nil
    local syms=vim.tbl_keys(c.s)
    c.s['Qnil']='nil'
    c.s['Qt']='t'
    c.s['Qunbound']='unbound'
    c.s['Qerror']='error'
    c.s['Qlambda']='lambda'
    table.sort(syms)
    table.insert(syms,1,'Qnil')
    table.insert(syms,2,'Qt')
    table.insert(syms,3,'Qunbound')
    table.insert(syms,4,'Qerror')
    table.insert(syms,5,'Qlambda')
    table.insert(out,('struct Lisp_Symbol lispsym[%d];'):format(#syms))
    table.insert(out,'')
    for k,v in ipairs(syms) do
        table.insert(out,('#define i%s %d'):format(v,k-1))
    end
    for k,v in vim.spairs(c.f) do
        local s=tostring(v)
        if v==-2 then
            s='MANY'
        elseif v==-1 then
            s='UNEVALLED'
        end
        table.insert(out,('EXFUN(%s, %s);'):format(k,s))
    end
    table.insert(out,'static char const *const defsym_name[] = {')
    for _,v in ipairs(syms) do
        table.insert(out,('  "%s",'):format(c.s[v]))
    end
    table.insert(out,'};')
    for k,v in ipairs(syms) do
        table.insert(out,('#define %s builtin_lisp_symbol (%s)'):format(v,k-1))
    end
    return out
end
---@param cfn nelisp.makedoc.cfn
---@param out string[]
local function cfn_to_meta(cfn,out)
    local varnames='abcdefghijklmnopqrstuvwxyz'
    for name,maxargs in vim.spairs(cfn) do
        table.insert(out,'')
        local arg_vars={}
        for i=1,maxargs do
            assert(varnames:sub(i,i)~='')
            table.insert(arg_vars,varnames:sub(i,i))
            table.insert(out,('---@param %s nelisp.obj'):format(varnames:sub(i,i)))
        end
        table.insert(out,'---@return nelisp.obj')
        table.insert(out,('function M.l%s(%s) end'):format(name,table.concat(arg_vars,',')))
    end
end
if #arg<4 then
    error(('require 4 arguments, got %d'):format(#arg))
end
local lua_c_file,lua_out,c_dir,c_out=unpack(arg --[[@as string[] ]])
local c
if outfile_needs_update(c_out,c_dir) then
    c={f={},s={},v={}}
    local files={}
    for file in vim.fs.dir(c_dir) do
        if vim.endswith(file,'.c') then
            table.insert(files,vim.fs.normalize(vim.fs.abspath(vim.fs.joinpath(c_dir,file))))
        end
    end
    for _,path in ipairs(files) do
        scan_c(path,c)
    end
    vim.fn.writefile(c_to_globals(c),c_out)
end
if outfile_needs_update(lua_out,lua_c_file) or outfile_needs_update(lua_out,c_dir) then
    if not c then
        c={f={},s={},v={}}
        local files={}
        for file in vim.fs.dir(c_dir) do
            if vim.endswith(file,'.c') then
                table.insert(files,vim.fs.normalize(vim.fs.abspath(vim.fs.joinpath(c_dir,file))))
            end
        end
        for _,path in ipairs(files) do
            scan_c(path,c)
        end
    end
    local lfn=scan_lua_c(lua_c_file)
    local out={
        '---@meta nelisp.c',
        '-- This file is generated by makedoc.lua',
        '',
        '---@class nelisp.obj',
        '',
        'local M={}',
    }
    lfn_to_meta(lfn,out)
    cfn_to_meta(c.f,out)
    table.insert(out,'')
    table.insert(out,'return M')
    vim.fn.writefile(out,lua_out)
end
