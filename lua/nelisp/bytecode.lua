local lisp=require'nelisp.lisp'
local signal=require'nelisp.signal'
local vars=require'nelisp.vars'
local specpdl=require'nelisp.specpdl'
local eval=require'nelisp.eval'
local fns=require'nelisp.fns'
local data=require'nelisp.data'
local handler=require'nelisp.handler'

local ins={
    stack_ref=0,
    stack_ref1=1,
    stack_ref2=2,
    stack_ref3=3,
    stack_ref4=4,
    stack_ref5=5,
    stack_ref6=6,
    stack_ref7=7,
    varref=8,
    varref1=9,
    varref2=10,
    varref3=11,
    varref4=12,
    varref5=13,
    varref6=14,
    varref7=15,
    varset=16,
    varset1=17,
    varset2=18,
    varset3=19,
    varset4=20,
    varset5=21,
    varset6=22,
    varset7=23,
    varbind=24,
    varbind1=25,
    varbind2=26,
    varbind3=27,
    varbind4=28,
    varbind5=29,
    varbind6=30,
    varbind7=31,
    call=32,
    call1=33,
    call2=34,
    call3=35,
    call4=36,
    call5=37,
    call6=38,
    call7=39,
    unbind=40,
    unbind1=41,
    unbind2=42,
    unbind3=43,
    unbind4=44,
    unbind5=45,
    unbind6=46,
    unbind7=47,
    pophandler=48,
    pushconditioncase=49,
    pushcatch=50,
    nth=56,
    symbolp=57,
    consp=58,
    stringp=59,
    listp=60,
    eq=61,
    memq=62,
    ['not']=63,
    car=64,
    cdr=65,
    cons=66,
    list1=67,
    list2=68,
    list3=69,
    list4=70,
    length=71,
    aref=72,
    aset=73,
    symbol_value=74,
    symbol_function=75,
    set=76,
    fset=77,
    get=78,
    substring=79,
    concat2=80,
    concat3=81,
    concat4=82,
    sub1=83,
    add1=84,
    eqlsign=85,
    gtr=86,
    lss=87,
    leq=88,
    geq=89,
    diff=90,
    negate=91,
    plus=92,
    max=93,
    min=94,
    mult=95,
    point=96,
    --save_current_buffer_OBSOLETE=97,
    goto_char=98,
    insert=99,
    point_max=100,
    point_min=101,
    char_after=102,
    following_char=103,
    preceding_char=104,
    current_column=105,
    indent_to=106,
    eolp=108,
    eobp=109,
    bolp=110,
    bobp=111,
    current_buffer=112,
    set_buffer=113,
    save_current_buffer=114,
    --interactive_p=116,
    forward_char=117,
    forward_word=118,
    skip_chars_forward=119,
    skip_chars_backward=120,
    forward_line=121,
    char_syntax=122,
    buffer_substring=123,
    delete_region=124,
    narrow_to_region=125,
    widen=126,
    end_of_line=127,
    constant2=129,
    goto=130,
    gotoifnil=131,
    gotoifnonnil=132,
    gotoifnilelsepop=133,
    gotoifnonnilelsepop=134,
    ['return']=135,
    discard=136,
    dup=137,
    save_excursion=138,
    --save_window_excursion=139,
    save_restriction=140,
    --catch=141,
    unwind_protect=142,
    --condition_case=143,
    --temp_output_buffer_setup=144,
    --temp_output_buffer_show=145,
    set_marker=147,
    match_beginning=148,
    match_end=149,
    upcase=150,
    downcase=151,
    stringeqlsign=152,
    stringlss=153,
    equal=154,
    nthcdr=155,
    elt=156,
    member=157,
    assq=158,
    nreverse=159,
    setcar=160,
    setcdr=161,
    car_safe=162,
    cdr_safe=163,
    nconc=164,
    quo=165,
    rem=166,
    numberp=167,
    integerp=168,
    listN=175,
    concatN=176,
    insertN=177,
    stack_set=178,
    stack_set2=179,
    discardN=182,
    switch=183,
    constant=192,
}

local M={}
local rev_ins={}
for k,v in pairs(ins) do
    rev_ins[v]=k
end
local function debug_print(stack,op)
    vars.F.Xprint(stack)
    print(vim.inspect(op<=ins.constant and rev_ins[op] or 'constant0'..op-ins.constant))
end

---@param fun nelisp.obj
---@param args_template number
---@param args nelisp.obj[]
---@return nelisp.obj
function M.exec_byte_code(fun,args_template,args)
    if not _G.nelisp_later then
        error('TODO: speed inprovement')
    end
    local cidx=lisp.compiled_idx

    local bc={}

    local stack
    local pc
    local bytestr=(fun --[[@as nelisp._compiled]]).contents[cidx.bytecode]
    local function push(x)
        table.insert(stack,x)
    end
    local function discard(n)
        if n==0 then return end
        local tem={}
        table.move(stack,1,#stack-n,1,tem)
        stack=tem
    end
    local function discard_get(n)
        assert(#stack>=n)
        if n==0 then return {} end
        local ret={}
        table.move(stack,#stack-n+1,#stack,1,ret)
        discard(n)
        assert(#ret==n)
        return ret
    end
    local function top()
        return assert(stack[#stack])
    end
    local function set_top(x)
        stack[#stack]=x
    end
    local function pop()
        return assert(table.remove(stack))
    end

    local function setup_frame()
        local tfun=(fun --[[@as nelisp._compiled]]).contents --[[@as (nelisp.obj[])]]
        assert(not lisp.string_multibyte(bytestr))
        local vector=tfun[cidx.constants]
        local vectorp=(vector --[[@as nelisp._compiled]]).contents --[[@as (nelisp.obj[])]]

        local bytestr_data=lisp.sdata(bytestr)
        table.insert(bc,{
            vectorp=vectorp,
            bytestr_data=bytestr_data,
            saved_top=stack,
            saved_pc=pc,
        })

        stack={}
        local function fetch()
            pc=pc+1
            return assert(string.byte(bytestr_data,pc))
        end
        local function fetch2()
            local o=fetch()
            return o+bit.lshift(fetch(),8)
        end
        pc=0

        local rest=bit.band(args_template,128)~=0
        local mandatory=bit.band(args_template,127)
        local nonreset=bit.rshift(args_template,8)
        local nargs=#args
        if not (mandatory<=nargs and (rest or nargs<=nonreset)) then
            vars.F.signal(vars.Qwrong_number_of_arguments,lisp.list(
                vars.F.cons(lisp.make_fixnum(mandatory),lisp.make_fixnum(nonreset)),
                lisp.make_fixnum(nargs)))
        end
        local pushedargs=math.min(nonreset,nargs)
        for _=1,pushedargs do
            push(table.remove(args,1))
        end
        if nonreset<nargs then
            push(vars.F.list(args))
        else
            for _=nargs-(rest and 1 or 0),nonreset-1 do
                push(vars.Qnil)
            end
        end

        while true do
            local op
            local function op_branch()
                pc=op
            end
            local function next_()
                ::next::
                op=fetch()
                if op>=ins.stack_ref1 and op<=ins.stack_ref5 then
                    local v1=stack[#stack-op+ins.stack_ref]
                    push(v1)
                    goto next
                elseif op==ins.stack_ref6 then
                    local v1=stack[#stack-fetch()]
                    push(v1)
                    goto next
                elseif op>=ins.varref and op<=ins.varref7 then
                    if op==ins.varref6 then
                        op=fetch()
                    elseif op==ins.varref7 then
                        op=fetch2()
                    else
                        op=op-ins.varref
                    end
                    local v1=vectorp[op+1]
                    local v2=vars.F.symbol_value(v1)
                    push(v2)
                    goto next
                elseif op>=ins.varset and op<=ins.varset7 then
                    if op==ins.varset6 then
                        op=fetch()
                    elseif op==ins.varset7 then
                        op=fetch2()
                    else
                        op=op-ins.varset
                    end
                    local sym=vectorp[op+1]
                    local val=pop()
                    data.set_internal(sym,val,vars.Qnil,'SET')
                    goto next
                elseif op>=ins.varbind and op<=ins.varbind7 then
                    if op==ins.varbind6 then
                        op=fetch()
                    elseif op==ins.varbind7 then
                        op=fetch2()
                    else
                        op=op-ins.varbind
                    end
                    specpdl.bind(vectorp[op+1],pop())
                    goto next
                elseif op>=ins.call and op<=ins.call7 then
                    if op==ins.call6 then
                        op=fetch()
                    elseif op==ins.call7 then
                        op=fetch2()
                    else
                        op=op-ins.call
                    end
                    vars.lisp_eval_depth=vars.lisp_eval_depth+1
                    if vars.lisp_eval_depth>lisp.fixnum(vars.V.max_lisp_eval_depth) then
                        if lisp.fixnum(vars.V.max_lisp_eval_depth)<100 then
                            vars.V.max_lisp_eval_depth=lisp.make_fixnum(100)
                        end
                        if vars.lisp_eval_depth>lisp.fixnum(vars.V.max_lisp_eval_depth) then
                            signal.error("Lisp nesting exceeds `max-lisp-eval-depth'")
                        end
                    end

                    local call_args=discard_get(op)
                    local call_fun=top()
                    local count1=specpdl.record_in_backtrace(call_fun,call_args,op)
                    if not lisp.nilp(vars.V.debug_on_next_call) then
                        error('TODO')
                    end
                    local original_fun=call_fun
                    if lisp.symbolp(call_fun) then
                        call_fun=call_fun.fn
                    end
                    if lisp.compiledp(call_fun) then
                        local template=(call_fun --[[@as nelisp._compiled]]).contents[cidx.arglist]
                        if lisp.fixnump(template) then
                            local bytecode=(call_fun --[[@as nelisp._compiled]]).contents[cidx.bytecode]
                            if not lisp.consp(bytecode) then
                                fun=call_fun
                                bytestr=bytecode
                                args_template=lisp.fixnum(template)
                                args=call_args
                                assert(setup_frame()==nil)
                                goto next
                            end
                        end
                    end
                    local val
                    if lisp.subrp(call_fun) and not lisp.subr_native_compiled_dynp(call_fun) then
                        val=eval.funcall_subr(call_fun,call_args)
                    else
                        val=eval.funcall_general(original_fun,call_args)
                    end
                    vars.lisp_eval_depth=vars.lisp_eval_depth-1
                    if specpdl.backtrace_debug_on_exit(specpdl.index()-1) then
                        error('TODO')
                    end
                    specpdl.unbind_to(specpdl.index()-1,nil)
                    set_top(val)
                    goto next
                elseif op>=ins.unbind and op<=ins.unbind7 then
                    if op==ins.unbind6 then
                        error('TODO')
                    elseif op==ins.unbind7 then
                        error('TODO')
                    else
                        op=op-ins.unbind
                    end
                    specpdl.unbind_to(specpdl.index()-op,nil)
                    goto next
                elseif op==ins.pophandler then
                    return false
                elseif op==ins.pushconditioncase then
                    local typ='CONDITION_CASE'
                    local tag_ch_val=pop()
                    local bytecode_dest=fetch2()
                    local bytecode_top=#stack
                    local noerr,msg=handler.with_handler(tag_ch_val,typ,function ()
                        return assert(next_()==false)
                    end)
                    if noerr then
                        goto next
                    else
                        discard(#stack-bytecode_top)
                        op=bytecode_dest
                        bytestr_data=bc[#bc].bytestr_data
                        vectorp=bc[#bc].vectorp
                        pc=0
                        push(msg.val)
                        op_branch()
                        goto next
                    end
                elseif op==ins.symbolp then
                    set_top(lisp.symbolp(top()) and vars.Qt or vars.Qnil)
                    goto next
                elseif op==ins.consp then
                    set_top(lisp.consp(top()) and vars.Qt or vars.Qnil)
                    goto next
                elseif op==ins.stringp then
                    set_top(lisp.stringp(top()) and vars.Qt or vars.Qnil)
                    goto next
                elseif op==ins.listp then
                    set_top((lisp.consp(top()) or lisp.nilp(top())) and vars.Qt or vars.Qnil)
                    goto next
                elseif op==ins.eq then
                    local v1=pop()
                    set_top(lisp.eq(top(),v1) and vars.Qt or vars.Qnil)
                    goto next
                elseif op==ins.memq then
                    local v1=pop()
                    set_top(vars.F.memq(top(),v1))
                    goto next
                elseif op==ins['not'] then
                    set_top(lisp.nilp(top()) and vars.Qt or vars.Qnil)
                    goto next
                elseif op==ins.car then
                    if lisp.consp(top()) then
                        set_top(lisp.xcar(top()))
                    elseif not lisp.nilp(top()) then
                        signal.wrong_type_argument(vars.Qlistp,top())
                    end
                    goto next
                elseif op==ins.cdr then
                    if lisp.consp(top()) then
                        set_top(lisp.xcdr(top()))
                    elseif not lisp.nilp(top()) then
                        signal.wrong_type_argument(vars.Qlistp,top())
                    end
                    goto next
                elseif op==ins.cons then
                    local v1=pop()
                    set_top(vars.F.cons(top(),v1))
                    goto next
                elseif op>=ins.list1 and op<=ins.list4 then
                    local d=discard_get(op-ins.list1)
                    set_top(lisp.list(top(),unpack(d)))
                    goto next
                elseif op==ins.length then
                    set_top(vars.F.length(top()))
                    goto next
                elseif op==ins.gtr then
                    local v2=pop()
                    local v1=top()
                    if lisp.fixnump(v1) and lisp.fixnump(v2) then
                        set_top(lisp.fixnum(v1)>lisp.fixnum(v2) and vars.Qt or vars.Qnil)
                    else
                        error('TODO')
                    end
                    goto next
                elseif op==ins.lss then
                    local v2=pop()
                    local v1=top()
                    if lisp.fixnump(v1) and lisp.fixnump(v2) then
                        set_top(lisp.fixnum(v1)<lisp.fixnum(v2) and vars.Qt or vars.Qnil)
                    else
                        error('TODO')
                    end
                    goto next
                elseif op==ins.geq then
                    local v2=pop()
                    local v1=top()
                    if lisp.fixnump(v1) and lisp.fixnump(v2) then
                        set_top(lisp.fixnum(v1)>=lisp.fixnum(v2) and vars.Qt or vars.Qnil)
                    else
                        error('TODO')
                    end
                    goto next
                elseif op==ins.symbol_value then
                    set_top(vars.F.symbol_value(top()))
                    goto next
                elseif op==ins.symbol_function then
                    set_top(vars.F.symbol_function(top()))
                    goto next
                elseif op==ins.aset then
                    local newelt=pop()
                    local idxval=pop()
                    local arrayval=top()
                    set_top(vars.F.aset(arrayval,idxval,newelt))
                    goto next
                elseif op==ins.set then
                    local v1=pop()
                    set_top(vars.F.set(top(),v1))
                    goto next
                elseif op==ins.fset then
                    local v1=pop()
                    set_top(vars.F.fset(top(),v1))
                    goto next
                elseif op==ins.get then
                    local v1=pop()
                    set_top(vars.F.get(v1,top()))
                    goto next
                elseif op==ins.substring then
                    local v2=pop()
                    local v1=pop()
                    set_top(vars.F.substring(top(),v1,v2))
                    goto next
                elseif op==ins.add1 then
                    set_top(vars.F.add1(top()))
                    goto next
                elseif op==ins.eqlsign then
                    local v2=pop()
                    local v1=top()
                    if lisp.fixnump(v1) and lisp.fixnump(v2) then
                        set_top(v1==v2 and vars.Qt or vars.Qnil)
                    else
                        error('TODO')
                    end
                    goto next
                elseif op==ins['goto'] then
                    op=fetch2()
                    op_branch()
                    goto next
                elseif op==ins.gotoifnil then
                    local v1=pop()
                    op=fetch2()
                    if lisp.nilp(v1) then
                        op_branch()
                    end
                    goto next
                elseif op==ins.gotoifnonnil then
                    op=fetch2()
                    if not lisp.nilp(pop()) then
                        op_branch()
                    end
                    goto next
                elseif op==ins.gotoifnilelsepop then
                    op=fetch2()
                    if lisp.nilp(top()) then
                        op_branch()
                        goto next
                    end
                    discard(1)
                    goto next
                elseif op==ins.gotoifnonnilelsepop then
                    op=fetch2()
                    if not lisp.nilp(top()) then
                        op_branch()
                        goto next
                    end
                    discard(1)
                    goto next
                elseif op==ins['return'] then
                    local saved_top=bc[#bc].saved_top
                    if saved_top then
                        local val=top()
                        vars.lisp_eval_depth=vars.lisp_eval_depth-1
                        if specpdl.backtrace_debug_on_exit(specpdl.index()-1) then
                            error('TODO')
                        end
                        specpdl.unbind_to(specpdl.index()-1,nil)
                        stack=saved_top
                        pc=table.remove(bc).saved_pc
                        local fp=bc[#bc]
                        vectorp=fp.vectorp
                        bytestr_data=fp.bytestr_data
                        set_top(val)
                        return nil
                    end
                    return top()
                elseif op==ins.discard then
                    discard(1)
                    goto next
                elseif op==ins.dup then
                    push(top())
                    goto next
                elseif op==ins.unwind_protect then
                    local handler=pop()
                    specpdl.record_unwind_protect(function ()
                        if lisp.functionp(handler) then
                            vars.F.funcall({handler})
                        else
                            vars.F.progn(handler)
                        end
                    end)
                    goto next
                elseif op==ins.match_beginning then
                    set_top(vars.F.match_beginning(top()))
                    goto next
                elseif op==ins.match_end then
                    set_top(vars.F.match_end(top()))
                    goto next
                elseif op==ins.stringeqlsign then
                    local v1=pop()
                    set_top(vars.F.string_equal(top(),v1))
                    goto next
                elseif op==ins.member then
                    local v1=pop()
                    set_top(vars.F.member(top(),v1))
                    goto next
                elseif op==ins.assq then
                    local v1=pop()
                    set_top(vars.F.assq(top(),v1))
                    goto next
                elseif op==ins.nreverse then
                    set_top(vars.F.nreverse(top()))
                    goto next
                elseif op==ins.car_safe then
                    set_top(vars.F.car_safe(top()))
                    goto next
                elseif op==ins.cdr_safe then
                    set_top(vars.F.cdr_safe(top()))
                    goto next
                elseif op==ins.nconc then
                    local a=discard_get(2)
                    push(vars.F.nconc(a))
                    goto next
                elseif op==ins.numberp then
                    set_top(lisp.numberp(top()) and vars.Qt or vars.Qnil)
                    goto next
                elseif op==ins.listN then
                    op=fetch()
                    set_top(lisp.list(top(),unpack(discard_get(op-1))))
                    goto next
                elseif op==ins.stack_set then
                    stack[#stack-fetch()]=pop()
                    goto next
                elseif op==ins.discardN then
                    op=fetch()
                    if bit.band(op,0x80)~=0 then
                        op=bit.band(op,0x7f)
                        stack[#stack-op]=top()
                    end
                    discard(op)
                    goto next
                elseif op==ins.switch then
                    local jmp_table=pop()
                    local v1=pop()
                    local h=(jmp_table --[[@as nelisp._hash_table]])
                    local i=fns.hash_lookup(h,v1)
                    if i>=0 then
                        local val=lisp.aref(h.key_and_value,2*i+1)
                        op=lisp.fixnum(val)
                        op_branch()
                    end
                    goto next
                elseif op>=ins.constant then
                    push(vectorp[op-ins.constant+1])
                    goto next
                else
                    error('TODO: byte-code '..op)
                end
                error('unreachable')
            end
            return next_()
        end
    end
    return assert(setup_frame()) --[[@as nelisp.obj]]
end
local F={}
F.byte_code={'byte-code',3,3,0,[[Function used internally in byte-compiled code.
The first argument, BYTESTR, is a string of byte code;
the second, VECTOR, a vector of constants;
the third, MAXDEPTH, the maximum stack depth used in this function.
If the third argument is incorrect, Emacs may crash.]]}
function F.byte_code.f(bytestr,vector,maxdepth)
    if not (lisp.stringp(bytestr) and lisp.vectorp(vector) and lisp.fixnatp(maxdepth)) then
        signal.error('Invalid byte-code')
    end
    if lisp.string_multibyte(bytestr) then
        bytestr=vars.F.string_as_unibyte(bytestr)
    end
    local fun=vars.F.make_byte_code({vars.Qnil,bytestr,vector,maxdepth})
    return M.exec_byte_code(fun,0,{})
end
function M.init_syms()
    vars.defsubr(F,'byte_code')
end
return M
