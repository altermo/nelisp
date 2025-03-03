local query=vim.treesitter.query.parse('c',[[
(preproc_def name: (identifier) @name) @definition
(preproc_function_def name: (identifier) @name) @definition

(function_definition declarator: (_ declarator: (identifier) @name . (parameter_list))) @definition
(function_definition declarator: (pointer_declarator declarator: (_ declarator: (identifier) @name))) @definition
(function_definition declarator: (pointer_declarator declarator: (pointer_declarator declarator: (_ declarator: (identifier) @name)))) @definition
(function_definition declarator: (parenthesized_declarator (identifier) @name)) @definition
(function_definition declarator: (_ declarator: (parenthesized_declarator (identifier) @name))) @definition
(function_definition declarator: (function_declarator declarator: (_ parameters: (_ (parameter_declaration) @name)))) @definition
(function_definition declarator: (_ (ERROR (identifier) @name) . (parameter_list))) @definition

(enumerator name: (identifier) @name) @definition

((expression_statement (call_expression (call_expression function: (_) @_defun (#eq? @_defun "DEFUN") arguments: (_ (string_literal) . (identifier) @name)))) . (compound_statement) @definition) @definition2
]])
local query_check=vim.treesitter.query.parse('c',[[
(preproc_def) @1
(preproc_function_def) @1
(function_definition) @1
(enumerator) @1
; (enum_specifier) @1
; (struct_specifier) @1
; (type_definition) @1

((expression_statement (call_expression (call_expression function: (_) @1 (#eq? @1 "DEFUN") arguments: (_ (string_literal) . (identifier))))) . (compound_statement))
]])
local function get_nodes_with_injections(parser)
    ---Taken from runtime/lua/vim/treesitter/dev.lua
    local injections = {}
    parser:for_each_tree(function(parent_tree, parent_ltree)
        local parent = parent_tree:root()
        for _, child in pairs(parent_ltree:children()) do
            for _, tree in pairs(child:trees()) do
                local r = tree:root()
                local node = assert(parent:named_descendant_for_range(r:range()))
                -- HACK: `node:id` may return the same id if different trees
                local id = node:id()
                if not injections[id] or r:byte_length() > injections[id]:byte_length() then
                    injections[id] = r
                end
            end
        end
    end)
    return injections
end
local function get_definitions(source)
    local parser=vim.treesitter.get_string_parser(source,'c')
    local tree=parser:parse(true)[1]
    local out=vim.defaulttable(function () return {} end)
    out._nameless={}
    local n=0
    for _,match in query:iter_matches(tree:root(),source,0,-1,{}) do
        n=n+1
        local name,definition,definition2
        for id,nodes in pairs(match) do
            local fname=query.captures[id]
            assert(#nodes==1)
            if fname=='name' then
                name=vim.treesitter.get_node_text(nodes[1],source)
            elseif fname=='definition' then
                definition=nodes[1]
            elseif fname=='definition2' then
                definition2=nodes[1]
            elseif fname:sub(1,1)=='_' then
            else
                error('unexpected capture')
            end
        end
        assert(name and definition)
        table.insert(out[name],{definition,definition2})
    end
    local n_check=0
    local check_nodes={}
    for _,match in query_check:iter_matches(tree:root(),source,0,-1,{}) do
        n_check=n_check+1
        assert(#match==1 and #match[1]==1)
        check_nodes[match[1][1]:id()]=match[1][1]
    end
    if n_check~=n then
        for _,nodes_s in pairs(out) do
            for _,nodes in ipairs(nodes_s) do
                for _,node in ipairs(nodes) do
                    check_nodes[node:id()]=nil
                end
            end
        end
        local err={}
        for _,v in pairs(check_nodes) do
            table.insert(err,'```\n'..vim.treesitter.get_node_text(v,source)..'\n```')
        end
        error(('miscount: %d-%d \n'):format(n,n_check)..table.concat(err,'\n*AND*\n'))
    end
    return out,get_nodes_with_injections(parser)
end
local function diff_node_ignore_comments(a,b,a_source,b_source,a_injections,b_injections)
    if type(a)=='table' and type(b)=='table' then
        if #a~=#b then
            return true
        end
        for i=1,#a do
            if diff_node_ignore_comments(a[i],b[i],a_source,b_source,a_injections,b_injections) then
                return true
            end
        end
        return false
    end
    if a:type()=='comment' then
        assert(b:type()=='comment')
        return false
    end
    if a:type()~=b:type() then
        return true
    end
    local filter_a={}
    for node in a:iter_children() do
        if node:type()~='comment' then
            table.insert(filter_a,node)
        end
    end
    local filter_b={}
    for node in b:iter_children() do
        if node:type()~='comment' then
            table.insert(filter_b,node)
        end
    end
    if #filter_a~=#filter_b then
        return true
    end
    if a_injections[a:id()] then
        if not b_injections[b:id()] then
            return true
        end
        return diff_node_ignore_comments(a_injections[a:id()],b_injections[b:id()],a_source,b_source,a_injections,b_injections)
    end
    if #filter_a==0 then
        if a:named() and b:named() then --HACK: needed for e.x. `# define`/`#define`
            if vim.treesitter.get_node_text(a,a_source)~=vim.treesitter.get_node_text(b,b_source) then
                return true
            end
        end
    end
    for i=1,#filter_a do
        if diff_node_ignore_comments(filter_a[i],filter_b[i],a_source,b_source,a_injections,b_injections) then
            return true
        end
    end
    return false
end
local function diff(a,b)
    -- These break the parser, so remove them
    local remove={
        'DEFINE_GDB_SYMBOL_BEGIN%s*%b()',
        'DEFINE_GDB_SYMBOL_END%s*%b()',
        'ATTRIBUTE_NO_SANITIZE_UNDEFINED',
    }
    for _,r in ipairs(remove) do
        a=a:gsub(r,'')
        b=b:gsub(r,'')
    end
    local a_def,a_injections=get_definitions(a)
    local b_def,b_injections=get_definitions(b)
    local new={}
    local missing={}
    local both={}
    for name,_ in pairs(a_def) do
        if #b_def[name]==0 then
            table.insert(new,name)
        else
            table.insert(both,name)
        end
    end
    for name,_ in pairs(b_def) do
        if #a_def[name]==0 then
            table.insert(missing,name)
        end
    end
    local differ={}
    for _,name in pairs(both) do
        local a_nodes=a_def[name]
        local b_nodes=b_def[name]
        for j=1,#a_nodes do
            local a_node=a_nodes[j]
            for i=1,#b_nodes do
                local b_node=b_nodes[i]
                if not diff_node_ignore_comments(a_node,b_node,a,b,a_injections,b_injections) then
                    goto continue
                end
            end
            table.insert(differ,name)
            ::continue::
        end
    end
    return new,missing,both,differ,a_def,b_def,a,b
end
local state={cache={}}
local function buf_set_lines(buf,start,end_,lines)
    vim.bo[buf].modifiable=true
    vim.api.nvim_buf_set_lines(buf,start,end_,false,lines)
    vim.bo[buf].modifiable=false
end
local function create_get_buff(name)
    if vim.fn.bufexists(name)==1 then
        return vim.fn.bufnr(name)
    end
    local b=vim.api.nvim_create_buf(false,true)
    vim.api.nvim_buf_set_name(b,name)
    vim.bo[b].bufhidden='unload'
    vim.bo[b].modifiable=false
    return b
end
local function buf_get_line(buf)
    if vim.fn.bufwinid(buf)==-1 then
        return nil
    end
    local win=vim.fn.bufwinid(buf)
    local line=vim.fn.line('.',win)
    return vim.fn.getbufoneline(buf,line)
end
local function ui(path_nelisp,path_emacs)
    local files=vim.fn.readdir(path_nelisp)
    local e_files=vim.fn.readdir(path_emacs)
    for i=#files,1,-1 do
        local f=files[i]
        if not vim.tbl_contains(e_files,f) then
            table.remove(files,i)
        end
    end
    local b_file_selector=create_get_buff('nelisp://file_selector')
    local width=math.max(unpack(vim.tbl_map(string.len,files)))+9
    buf_set_lines(b_file_selector,0,-1,files)
    local b_option_selector=create_get_buff('nelisp://option_selector')
    buf_set_lines(b_option_selector,0,-1,{'diff','new','missing','both'})
    local b_function_selector=create_get_buff('nelisp://function_selector')
    local b_diff_old=create_get_buff('nelisp://diff_old')
    vim.bo[b_diff_old].filetype='c'
    local b_diff_new=create_get_buff('nelisp://diff_new')
    vim.bo[b_diff_new].filetype='c'
    local function render()
        local do_render_function
        do
            local file=buf_get_line(b_file_selector)
            if not file then return end
            if state.file~=file then
                do_render_function=true
            end
            if file then
                state.file=file
            end
        end
        do
            local option=buf_get_line(b_option_selector)
            if state.option~=option then
                do_render_function=true
            end
            if option then
                state.option=option
            end
        end
        local new,missing,both,differ,new_def,old_def,new_source,old_source
        if not state.cache[state.file] then
            local a_file=vim.fs.joinpath(path_nelisp,state.file)
            local b_file=vim.fs.joinpath(path_emacs,state.file)
            new,missing,both,differ,new_def,old_def,new_source,old_source=diff(vim.fn.readblob(a_file),vim.fn.readblob(b_file))
            state.cache[state.file]={new,missing,both,differ,new_def,old_def,new_source,old_source}
        else
            new,missing,both,differ,new_def,old_def,new_source,old_source=unpack(state.cache[state.file])
        end
        if do_render_function then
            local funcs={}
            if state.option=='diff' then
                funcs=differ
            elseif state.option=='new' then
                funcs=new
            elseif state.option=='missing' then
                funcs=missing
            elseif state.option=='both' then
                funcs=both
            end
            buf_set_lines(b_function_selector,0,-1,funcs)
        end
        local func=buf_get_line(b_function_selector)
        if func=='' then
            buf_set_lines(b_diff_old,0,-1,{})
            buf_set_lines(b_diff_new,0,-1,{})
            return
        end
        if not new_def[func] then
            buf_set_lines(b_diff_new,0,-1,{})
        else
            local lines={}
            for _,nodes in ipairs(new_def[func]) do
                for _,node in ipairs(nodes) do
                    for _,line in ipairs(vim.split(vim.treesitter.get_node_text(node,new_source),'\n')) do
                        table.insert(lines,line)
                    end
                end
                table.insert(lines,'/*'..('-'):rep(30)..'*/')
            end
            buf_set_lines(b_diff_new,0,-1,lines)
        end
        if not old_def[func] then
            buf_set_lines(b_diff_old,0,-1,{})
        else
            local lines={}
            for _,nodes in ipairs(old_def[func]) do
                for _,node in ipairs(nodes) do
                    for _,line in ipairs(vim.split(vim.treesitter.get_node_text(node,old_source),'\n')) do
                        table.insert(lines,line)
                    end
                end
                table.insert(lines,'/*'..('-'):rep(30)..'*/')
            end
            buf_set_lines(b_diff_old,0,-1,lines)
        end
    end
    vim.cmd.split{mods={tab=vim.fn.tabpagenr()}}
    vim.api.nvim_set_current_buf(b_file_selector)
    local w_file_selector=vim.api.nvim_get_current_win()
    vim.cmd.vsplit()
    vim.api.nvim_set_current_buf(b_function_selector)
    local w_function_selector=vim.api.nvim_get_current_win()
    vim.cmd.split()
    vim.api.nvim_set_current_buf(b_diff_old)
    local w_diff_old=vim.api.nvim_get_current_win()
    vim.cmd.vsplit()
    vim.api.nvim_set_current_buf(b_diff_new)
    local w_diff_new=vim.api.nvim_get_current_win()
    vim.cmd.wincmd('t')
    vim.cmd.split()
    vim.api.nvim_set_current_buf(b_option_selector)
    local w_option_selector=vim.api.nvim_get_current_win()
    vim.cmd.wincmd{'|',count=width}
    vim.wo[w_option_selector].cursorline=true
    vim.wo[w_function_selector].cursorline=true
    vim.wo[w_file_selector].cursorline=true
    vim.wo[w_diff_old].diff=true
    vim.wo[w_diff_new].diff=true
    vim.api.nvim_create_autocmd('CursorMoved',{buffer=b_file_selector,callback=render})
    vim.api.nvim_create_autocmd('CursorMoved',{buffer=b_option_selector,callback=render})
    vim.api.nvim_create_autocmd('CursorMoved',{buffer=b_function_selector,callback=render})
    render()
end
return ui
