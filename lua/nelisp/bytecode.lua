local lisp=require'nelisp.lisp'
local signal=require'nelisp.signal'
local vars=require'nelisp.vars'
local specpdl=require'nelisp.specpdl'
local eval=require'nelisp.eval'

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

---@param fun nelisp.obj
---@param args_template number
---@param args nelisp.obj[]
function M.exec_byte_code(fun,args_template,args)
    if not _G.nelisp_later then
        error('TODO: speed inprovement')
    end
    local cidx=lisp.compiled_idx

    local quitcounter=0
    local bc={}

    local stack
    local pc
    local bytestr=(fun --[[@as nelisp._compiled]]).contents[cidx.bytecode]
    local function push(x)
        table.insert(stack,x)
    end
    local function discard(n)
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

    ::setup_frame::
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
                error('TODO')
            elseif op==ins.varref7 then
                error('TODO')
            else
                op=op-ins.varref
            end
            local v1=vectorp[op+1]
            local v2=vars.F.symbol_value(v1)
            push(v2)
            goto next
        elseif op>=ins.call and op<=ins.call7 then
            if op==ins.call6 then
                error('TODO')
            elseif op==ins.call7 then
                error('TODO')
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
                        goto setup_frame
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
        elseif op==ins.cons then
            local v1=pop()
            set_top(vars.F.cons(top(),v1))
            goto next
        elseif op>=ins.list1 and op<=ins.list4 then
            set_top(lisp.list(top(),unpack(discard_get(op-ins.list1))))
            goto next
        elseif op==ins.listN then
            op=fetch()
            set_top(lisp.list(top(),unpack(discard_get(op-1))))
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
                goto next
            end
            return top()
        elseif op==ins.discard then
            discard(1)
            goto next
        elseif op>=ins.constant then
            push(vectorp[op-ins.constant+1])
            goto next
        else
            error('TODO: byte-code '..op)
        end
        error('unreachable')
    end
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
