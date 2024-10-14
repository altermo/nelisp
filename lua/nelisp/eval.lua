local lisp=require'nelisp.lisp'
local fixnum=require'nelisp.obj.fixnum'
local vars=require'nelisp.vars'
local cons=require'nelisp.obj.cons'
local specpdl=require'nelisp.specpdl'
local symbol=require'nelisp.obj.symbol'
local subr=require'nelisp.obj.subr'
local M={}

---@param obj nelisp.symbol
---@return nelisp.obj
function M.indirect_function(obj)
    local has_visited={}
    ---@type nelisp.obj
    local hare=obj
    while true do
        if not lisp.symbolp(hare) or lisp.nilp(hare) then
            return hare
        end
        hare=symbol.get_func(hare --[[@as nelisp.symbol]])
        if has_visited[hare] then
            error('TODO: err')
        end
        has_visited[hare]=true
    end
end
local function funcall_lambda(fun,args)
    local lexenv,syms_left
    local count=specpdl.index()
    if lisp.consp(fun) then
        if lisp.eq(cons.car(fun),vars.Qclosure) then
            error('TODO')
        else
            lexenv=vars.Qnil
        end
        syms_left=cons.cdr(fun)
        if lisp.consp(syms_left) then
            syms_left=cons.car(syms_left --[[@as nelisp.cons]])
        else
            error('TODO: err')
        end
    else
        error('TODO')
    end
    local idx=1
    local rest=false
    local optional=false
    local previous_rest=false
    while lisp.consp(syms_left) do
        ---@cast syms_left nelisp.cons
        local next_=cons.car(syms_left)
        if not lisp.symbolp(next_) then
            error('TODO: err')
        end
        if lisp.eq(next_,vars.Qand_rest) then
            if rest or previous_rest then
                error('TODO: err')
            end
            rest=true
            previous_rest=true
        elseif lisp.eq(next_,vars.Qand_optional) then
            error('TODO')
        else
            local arg
            if rest then
                arg=vars.F.list({unpack(args,idx)})
                idx=#args+1
            elseif idx<=#args then
                arg=args[idx]
                idx=idx+1
            elseif not optional then
                error('TODO: err')
            else
                error('TODO')
            end
            if not lisp.nilp(lexenv) and lisp.symbolp(next_) then
                lexenv=vars.cons(vars.cons(next_,arg),lexenv)
            else
                specpdl.bind(next_,arg)
            end
            previous_rest=false
        end
        syms_left=cons.cdr(syms_left)
    end
    if not lisp.nilp(syms_left) or previous_rest then
        error('TODO: err')
    elseif idx<=#args then
        error('TODO: err')
    end
    if not lisp.eq(lexenv,vars.V.internal_interpreter_environment) then
        error('TODO')
    end
    local val
    if lisp.consp(fun) then
        val=vars.F.progn(cons.cdr(cons.cdr(fun) --[[@as nelisp.cons]]))
    else
        error('TODO')
    end
    return specpdl.unbind_to(count,val)
end
local function apply_lambda(fun,args,count)
    local arg_vector={}
    local args_left=args
    for _=1,lisp.list_length(args) do
        local tem=vars.F.car(args_left)
        args_left=vars.F.cdr(args_left)
        tem=M.eval_sub(tem)
        table.insert(arg_vector,tem)
    end
    specpdl.set_backtrace_args(count,arg_vector)
    local val=funcall_lambda(fun,arg_vector)
    vars.lisp_eval_depth=vars.lisp_eval_depth-1
    if specpdl.backtrace_debug_on_exit(count) then
        error('TODO')
    end
    return specpdl.unbind_to(specpdl.index()-1,val)
end
local function apply1(fn,args)
    return lisp.nilp(arg) and vars.F.funcall({fn}) or vars.F.apply({fn,args})
end
---@param form nelisp.obj
---@return nelisp.obj
function M.eval_sub(form)
    if lisp.symbolp(form) then
        local lex_binding=vars.F.assq(form,vars.V.internal_interpreter_environment)
        return not lisp.nilp(lex_binding) and cons.cdr(lex_binding) or vars.F.symbol_value(form)
    end
    if not lisp.consp(form) then
        return form
    end

    ---@cast form nelisp.cons

    if not _G.nelisp_later then
        error('TODO: vars.lisp_eval_depth may not be decreased if error, how to solve this? how does emacs do it?\nAlso how is specpdl reset to a previous state')
        --Note that vars.lisp_eval_depth is being incremented/decremented in other places, so when changing how this works, remember to change those places too (if needed)
    end
    vars.lisp_eval_depth=vars.lisp_eval_depth+1
    if vars.lisp_eval_depth>fixnum.tonumber(vars.V.max_lisp_eval_depth) then
        if fixnum.tonumber(vars.V.max_lisp_eval_depth)<100 then
            vars.V.max_lisp_eval_depth=fixnum.make(100)
        end
        if vars.lisp_eval_depth>fixnum.tonumber(vars.V.max_lisp_eval_depth) then
            error('TODO: err')
        end
    end
    local original_fun=cons.car(form)
    local original_args=cons.cdr(form)
    local val
    lisp.check_list(original_args)
    local count=specpdl.record_in_backtrace(original_fun,original_args,'UNEVALLED')
    if not lisp.nilp(vars.V.debug_on_next_call) then
        error('TODO')
    end
    local fun=original_fun
    ---@cast fun nelisp.symbol
    if (not lisp.symbolp(fun)) then
        error('TODO')
    elseif (not lisp.nilp(fun)) then
        fun=symbol.get_func(fun)
        if lisp.symbolp(fun) then
            fun=M.indirect_function(fun --[[@as nelisp.symbol]])
        end
    end
    if lisp.subrp(fun) and not lisp.subr_native_compiled_dynp(fun) then
        local args_left=original_args
        local numargs=lisp.list_length(args_left)
        local t=subr.totable(fun --[[@as nelisp.subr]])
        if numargs<t.minargs or (t.maxargs>=0 and numargs>t.maxargs) then
            error('TODO: err')
        elseif t.maxargs==-1 then
            val=t.fun(args_left)
        elseif t.maxargs==-2 or t.maxargs>8 then
            local vals={}
            while (lisp.consp(args_left) and #vals<numargs) do
                ---@cast args_left nelisp.cons
                local arg=cons.car(args_left)
                args_left=cons.cdr(args_left)
                table.insert(vals,M.eval_sub(arg))
            end

            specpdl.set_backtrace_args(count,vals)
            val=t.fun(vals)
        else
            local argvals={}
            for _=1,t.maxargs do
                table.insert(argvals,M.eval_sub(vars.F.car(args_left)))
                args_left=vars.F.cdr(args_left)
            end
            specpdl.set_backtrace_args(count,argvals)
            val=t.fun(unpack(argvals))
        end
    elseif lisp.compiledp(fun) or lisp.subr_native_compiled_dynp(fun) or lisp.module_functionp(fun) then
        error('TODO')
    else
        if lisp.nilp(fun) then
            error('TODO: MAYBE need to implement: '..symbol.get_name(original_fun --[[@as nelisp.symbol]])[2])
            error('TODO: err')
        elseif not lisp.consp(fun) then
            error('TODO: err')
        end
        ---@cast fun nelisp.cons
        local funcar=cons.car(fun)
        if not lisp.symbolp(funcar) then
            error('TODO: err')
        elseif lisp.eq(funcar,vars.Qautoload) then
            error('TODO')
        elseif lisp.eq(funcar,vars.Qmacro) then
            local count1=specpdl.index()
            specpdl.bind(vars.Qlexical_binding,lisp.nilp(vars.V.internal_interpreter_environment) and vars.Qnil or vars.Qt)
            local p=vars.V.internal_interpreter_environment
            local dynvars=vars.V.macroexp__dynvars
            while not lisp.nilp(p) do
                local e=cons.car(p)
                if lisp.symbolp(e) then
                    dynvars=vars.F.cons(e, dynvars)
                end
                p=cons.cdr(p)
            end
            if lisp.eq(dynvars,vars.V.macroexp__dynvars) then
                specpdl.bind(vars.Qmacroexp__dynvars,dynvars)
            end
            local exp=apply1(vars.F.cdr(fun),original_args)
            exp=specpdl.unbind_to(count1,exp)
            val=M.eval_sub(exp)
        elseif lisp.eq(funcar,vars.Qlambda) or lisp.eq(funcar,vars.Qclosure) then
            return apply_lambda(fun,original_args,count)
        else
            error('TODO: err')
        end
    end
    vars.lisp_eval_depth=vars.lisp_eval_depth-1
    if specpdl.backtrace_debug_on_exit(count) then
        error('TODO')
    end
    return specpdl.unbind_to(specpdl.index()-1,assert(val))
end
function M.init()
    vars.lisp_eval_depth=0
    if not _G.nelisp_later then
        vars.F.make_variable_buffer_local(vars.Qlexical_binding)
    end
end

local F={}
F.setq={'setq',0,-1,0,[[Set each SYM to the value of its VAL.
The symbols SYM are variables; they are literal (not evaluated).
The values VAL are expressions; they are evaluated.
Thus, (setq x (1+ y)) sets `x' to the value of `(1+ y)'.
The second VAL is not computed until after the first SYM is set, and so on;
each VAL can use the new value of variables set earlier in the `setq'.
The return value of the `setq' form is the value of the last VAL.
usage: (setq [SYM VAL]...)]]}
function F.setq.f(args)
    local val=args
    local tail=args
    local nargs=0
    while lisp.consp(tail) do
        local sym=cons.car(tail)
        tail=cons.cdr(tail)
        if not lisp.consp(tail) then
            error('TODO: err')
            ---@cast tail nelisp.cons
        end
        local arg=cons.car(tail)
        tail=cons.cdr(tail)
        val=M.eval_sub(arg)
        local lex_binding=lisp.symbolp(sym) and vars.F.assq(sym,vars.V.internal_interpreter_environment) or vars.Qnil
        if not lisp.nilp(lex_binding) then
            error('TODO')
        else
            vars.F.set(sym,val)
        end
        nargs=nargs+2
    end
    return val
end
F.let={'let',1,-1,0,[[Bind variables according to VARLIST then eval BODY.
The value of the last form in BODY is returned.
Each element of VARLIST is a symbol (which is bound to nil)
or a list (SYMBOL VALUEFORM) (which binds SYMBOL to the value of VALUEFORM).
All the VALUEFORMs are evalled before any symbols are bound.
usage: (let VARLIST BODY...)]]}
function F.let.f(args)
    local count=specpdl.index()
    local varlist=cons.car(args)
    local varlist_len=lisp.list_length(varlist)
    local elt
    local temps={}
    local argnum=0
    while argnum<varlist_len and lisp.consp(varlist) do
        ---@cast varlist nelisp.cons
        elt=cons.car(varlist)
        varlist=cons.cdr(varlist)
        if lisp.symbolp(elt) then
            temps[argnum]=vars.Qnil
        elseif not lisp.nilp(vars.F.cdr(vars.F.cdr(elt))) then
            error('TODO: err')
        else
            temps[argnum]=M.eval_sub(vars.F.car(vars.F.cdr(elt)))
        end
    end
    varlist=cons.car(args)
    local lexenv=vars.V.internal_interpreter_environment
    argnum=0
    while argnum<varlist_len and lisp.consp(varlist) do
        ---@cast varlist nelisp.cons
        elt=cons.car(varlist)
        varlist=cons.cdr(varlist)
        local var=lisp.symbolp(elt) and elt or vars.F.car(elt)
        local tem=temps[argnum]
        if not lisp.nilp(lexenv) then
            error('TODO')
        else
            specpdl.bind(var,tem)
        end
    end
    if not lisp.eq(lexenv,vars.V.internal_interpreter_environment) then
        error('TODO')
    end
    elt=vars.F.progn(vars.F.cdr(args))
    return specpdl.unbind_to(count,elt)
end
F.letX={'let*',1,-1,0,[[Bind variables according to VARLIST then eval BODY.
The value of the last form in BODY is returned.
Each element of VARLIST is a symbol (which is bound to nil)
or a list (SYMBOL VALUEFORM) (which binds SYMBOL to the value of VALUEFORM).
Each VALUEFORM can refer to the symbols already bound by this VARLIST.
usage: (let* VARLIST BODY...)]]}
function F.letX.f(args)
    local count=specpdl.index()
    local lexenv=vars.V.internal_interpreter_environment
    local _,tail=lisp.for_each_tail(cons.car(args),function (varlist)
        local elt=cons.car(varlist)
        local var,val
        if lisp.symbolp(elt) then
            error('TODO')
        else
            var=vars.F.car(elt)
            if not lisp.nilp(vars.F.cdr(cons.cdr(elt --[[@as nelisp.cons]]))) then
                error('TODO: err')
            end
            val=M.eval_sub(vars.F.car(cons.cdr(elt --[[@as nelisp.cons]])))
        end
        if not lisp.nilp(lexenv) and lisp.symbolp(var)
            and not symbol.get_special(var)
            and lisp.nilp(vars.F.memq(var,vars.V.internal_interpreter_environment)) then
            error('TODO')
        else
            specpdl.bind(var,val)
        end
    end)
    lisp.check_list_end(tail,cons.car(args))
    local val=vars.F.progn(cons.cdr(args))
    return specpdl.unbind_to(count,val)
end
local function defvar(sym,initvalue,docstring,eval)
    lisp.check_symbol(sym)
    local tem=vars.F.default_boundp(sym)
    vars.F.internal__define_uninitialized_variable(sym,docstring)
    if lisp.nilp(tem) then
        vars.F.set_default(sym,eval and M.eval_sub(initvalue) or initvalue)
    else
        error('TODO')
    end
    return sym
end
F.defvar={'defvar',1,-1,0,[[Define SYMBOL as a variable, and return SYMBOL.
You are not required to define a variable in order to use it, but
defining it lets you supply an initial value and documentation, which
can be referred to by the Emacs help facilities and other programming
tools.  The `defvar' form also declares the variable as \"special\",
so that it is always dynamically bound even if `lexical-binding' is t.

If SYMBOL's value is void and the optional argument INITVALUE is
provided, INITVALUE is evaluated and the result used to set SYMBOL's
value.  If SYMBOL is buffer-local, its default value is what is set;
buffer-local values are not affected.  If INITVALUE is missing,
SYMBOL's value is not set.

If SYMBOL is let-bound, then this form does not affect the local let
binding but the toplevel default binding instead, like
`set-toplevel-default-binding`.
(`defcustom' behaves similarly in this respect.)

The optional argument DOCSTRING is a documentation string for the
variable.

To define a user option, use `defcustom' instead of `defvar'.

To define a buffer-local variable, use `defvar-local'.
usage: (defvar SYMBOL &optional INITVALUE DOCSTRING)]]}
function F.defvar.f(args)
    local sym=cons.car(args)
    local tail=cons.cdr(args)
    lisp.check_symbol(sym)
    if not lisp.nilp(tail) then
        ---@cast tail nelisp.cons
        if not lisp.nilp(cons.cdr(tail)) and not lisp.nilp(cons.cdr(cons.cdr(tail) --[[@as nelisp.cons]])) then
            error('TODO: err')
        end
        local exp=cons.car(tail)
        tail=cons.cdr(tail)
        return defvar(sym,exp,cons.car(tail --[[@as nelisp.cons]]),true)
    else
        error('TODO')
    end
end
local function lexbound_p(sym)
    for i in specpdl.riter() do
        if i.type==specpdl.type.let or i.type==specpdl.type.let_default then
            if lisp.eq(i.symbol,vars.Qinternal_interpreter_environment) then
                error('TODO')
            end
        end
    end
    return false
end
F.internal__define_uninitialized_variable={'internal--define-uninitialized-variable',1,2,0,
    [[Define SYMBOL as a variable, with DOC as its docstring.
This is like `defvar' and `defconst' but without affecting the variable's
value.]]}
function F.internal__define_uninitialized_variable.f(sym,doc)
    if not symbol.get_special(sym) and lexbound_p(sym) then
        error('TODO: err')
    end
    symbol.set_special(sym)
    if not lisp.nilp(doc) then
        vars.F.put(sym,vars.Qvariable_documentation,doc)
    end
    lisp.loadhist_attach(sym)
    return vars.Qnil
end
F.if_={'if',2,-1,0,[[If COND yields non-nil, do THEN, else do ELSE...
Returns the value of THEN or the value of the last of the ELSE's.
THEN must be one expression, but ELSE... can be zero or more expressions.
If COND yields nil, and there are no ELSE's, the value is nil.
usage: (if COND THEN ELSE...)]]}
---@param args nelisp.cons
function F.if_.f(args)
    local cond=M.eval_sub(cons.car(args))
    if not lisp.nilp(cond) then
        return M.eval_sub(vars.F.car(cons.cdr(args)))
    end
    return vars.F.progn(vars.F.cdr(cons.cdr(args)))
end
F.while_={'while',1,-1,0,[[If TEST yields non-nil, eval BODY... and repeat.
The order of execution is thus TEST, BODY, TEST, BODY and so on
until TEST returns nil.

The value of a `while' form is always nil.

usage: (while TEST BODY...)]]}
function F.while_.f(args)
    local test=cons.car(args)
    local body=cons.cdr(args)
    while not lisp.nilp(M.eval_sub(test)) do
        vars.F.progn(body)
    end
    return vars.Qnil
end
F.cond={'cond',0,-1,0,[[Try each clause until one succeeds.
Each clause looks like (CONDITION BODY...).  CONDITION is evaluated
and, if the value is non-nil, this clause succeeds:
then the expressions in BODY are evaluated and the last one's
value is the value of the cond-form.
If a clause has one element, as in (CONDITION), then the cond-form
returns CONDITION's value, if that is non-nil.
If no clause succeeds, cond returns nil.
usage: (cond CLAUSES...)]]}
function F.cond.f(args)
    local val=args
    while lisp.consp(args) do
        local clause=cons.car(args)
        val=M.eval_sub(vars.F.car(clause))
        if not lisp.nilp(val) then
            ---@cast clause nelisp.cons
            if not lisp.nilp(cons.cdr(clause)) then
                val=vars.F.progn(cons.cdr(clause))
            end
            break
        end
        args=cons.cdr(args)
    end
    return val
end
F.or_={'or',0,-1,0,[[Eval args until one of them yields non-nil, then return that value.
The remaining args are not evalled at all.
If all args return nil, return nil.
usage: (or CONDITIONS...)]]}
function F.or_.f(args)
    local val=vars.Qnil
    while lisp.consp(args) do
        ---@cast args nelisp.cons
        local arg=cons.car(args)
        args=cons.cdr(args)
        val=M.eval_sub(arg)
        if not lisp.nilp(val) then
            break
        end
    end
    return val
end
F.and_={'and',0,-1,0,[[Eval args until one of them yields nil, then return nil.
The remaining args are not evalled at all.
If no arg yields nil, return the last arg's value.
usage: (and CONDITIONS...)]]}
function F.and_.f(args)
    local val=lisp.T
    while lisp.consp(args) do
        local arg=cons.car(args)
        args=cons.cdr(args)
        val=M.eval_sub(arg)
        if lisp.nilp(val) then
            break
        end
    end
    return val
end
F.quote={'quote',1,-1,0,[[Return the argument, without evaluating it.  `(quote x)' yields `x'.
Warning: `quote' does not construct its return value, but just returns
the value that was pre-constructed by the Lisp reader (see info node
`(elisp)Printed Representation').
This means that \\='(a . b) is not identical to (cons \\='a \\='b): the former
does not cons.  Quoting should be reserved for constants that will
never be modified by side-effects, unless you like self-modifying code.
See the common pitfall in info node `(elisp)Rearrangement' for an example
of unexpected results when a quoted object is modified.
usage: (quote ARG)]]}
function F.quote.f(args)
    if not lisp.nilp(cons.cdr(args)) then
        error('TODO: err')
    end
    return cons.car(args)
end
F.progn={'progn',0,-1,0,[[Eval BODY forms sequentially and return value of last one.
usage: (progn BODY...)]]}
function F.progn.f(body)
    local val=vars.Qnil
    while lisp.consp(body) do
        local form=cons.car(body)
        body=cons.cdr(body)
        val=M.eval_sub(form)
    end
    return val
end
local function funcall_general(fun,args)
    local original_fun=fun
    if lisp.symbolp(fun) and not lisp.nilp(fun) then
        fun=symbol.get_func(fun)
        if lisp.symbolp(fun) then
            error('TODO')
        end
    end
    if lisp.subrp(fun) and not lisp.subr_native_compiled_dynp(fun) then
        error('TODO')
    elseif lisp.compiledp(fun) or lisp.subr_native_compiled_dynp(fun) or lisp.module_functionp(fun) then
        error('TODO')
    end
    if lisp.nilp(fun) then
        error('TODO: err')
    elseif not lisp.consp(fun) then
        error('TODO: err')
    end
    local funcar=cons.car(fun)
    if not lisp.symbolp(funcar) then
        error('TODO: err')
    elseif lisp.eq(funcar,vars.Qlambda) or lisp.eq(funcar,vars.Qclosure) then
        return funcall_lambda(fun,args)
    elseif lisp.eq(funcar,vars.Qautoload) then
        error('TODO')
    else
        error('TODO: err')
    end
end
F.funcall={'funcall',1,-2,0,[[Call first argument as a function, passing remaining arguments to it.
Return the value that function returns.
Thus, (funcall \\='cons \\='x \\='y) returns (x . y).
usage: (funcall FUNCTION &rest ARGUMENTS)]]}
function F.funcall.f(args)
    vars.lisp_eval_depth=vars.lisp_eval_depth+1
    if vars.lisp_eval_depth>fixnum.tonumber(vars.V.max_lisp_eval_depth) then
        if fixnum.tonumber(vars.V.max_lisp_eval_depth)<100 then
            vars.V.max_lisp_eval_depth=fixnum.make(100)
        end
        if vars.lisp_eval_depth>fixnum.tonumber(vars.V.max_lisp_eval_depth) then
            error('TODO: err')
        end
    end
    local fun_args={unpack(args,2)}
    local count=specpdl.record_in_backtrace(args[1],fun_args,#fun_args)
    if not lisp.nilp(vars.V.debug_on_next_call) then
        error('TODO')
    end
    local val=funcall_general(args[1],fun_args)
    vars.lisp_eval_depth=vars.lisp_eval_depth-1
    if specpdl.backtrace_debug_on_exit(count) then
        error('TODO')
    end
    return specpdl.unbind_to(specpdl.index()-1,val)
end
F.apply={'apply',1,-2,0,[[Call FUNCTION with our remaining args, using our last arg as list of args.
Then return the value FUNCTION returns.
With a single argument, call the argument's first element using the
other elements as args.
Thus, (apply \\='+ 1 2 \\='(3 4)) returns 10.
usage: (apply FUNCTION &rest ARGUMENTS)]]}
function F.apply.f(args)
    local spread_arg=args[#args]
    local numargs=lisp.list_length(spread_arg)
    local fun=args[1]
    if numargs==0 then
        error('TODO')
    elseif numargs==1 then
        error('TODO')
    end
    numargs=#args-2
    if lisp.symbolp(fun) and not lisp.nilp(fun) then
        fun=symbol.get_func(fun)
        if lisp.symbolp(fun) then
            error('TODO')
        end
    end
    local funcall_args={}
    for _,v in ipairs(args) do
        table.insert(funcall_args,v)
    end
    while not lisp.nilp(spread_arg) do
        table.insert(funcall_args,cons.car(spread_arg))
        spread_arg=cons.cdr(spread_arg)
    end
    return vars.F.funcall(funcall_args)
end
F.function_={'function',0,-1,0,[[Like `quote', but preferred for objects which are functions.
In byte compilation, `function' causes its argument to be handled by
the byte compiler.  Similarly, when expanding macros and expressions,
ARG can be examined and possibly expanded.  If `quote' is used
instead, this doesn't happen.

usage: (function ARG)]]}
function F.function_.f(args)
    local quoted=cons.car(args)
    if not lisp.nilp(cons.cdr(args)) then
        error('TODO: err')
    end
    if not lisp.nilp(vars.V.internal_interpreter_environment) then
        error('TODO')
    end
    return quoted
end

function M.init_syms()
    vars.setsubr(F,'setq')
    vars.setsubr(F,'let')
    vars.setsubr(F,'letX')
    vars.setsubr(F,'defvar')
    vars.setsubr(F,'internal__define_uninitialized_variable')
    vars.setsubr(F,'if_')
    vars.setsubr(F,'while_')
    vars.setsubr(F,'cond')
    vars.setsubr(F,'or_')
    vars.setsubr(F,'and_')
    vars.setsubr(F,'quote')
    vars.setsubr(F,'progn')
    vars.setsubr(F,'funcall')
    vars.setsubr(F,'apply')
    vars.setsubr(F,'function_')

    vars.defvar_lisp('max_lisp_eval_depth','max-lisp-eval-depth',
        [[Limit on depth in `eval', `apply' and `funcall' before error.

This limit serves to catch infinite recursions for you before they cause
actual stack overflow in C, which would be fatal for Emacs.
You can safely make it considerably larger than its default value,
if that proves inconveniently small.  However, if you increase it too far,
Emacs could overflow the real C stack, and crash.]])
    vars.V.max_lisp_eval_depth=fixnum.make(1600)

    vars.defvar_bool('debug_on_next_call','debug-on-next-call',
        [[Non-nil means enter debugger before next `eval', `apply' or `funcall'.]])
    vars.V.debug_on_next_call=vars.Qnil

    vars.defvar_lisp('internal_interpreter_environment',nil,
        [[If non-nil, the current lexical environment of the lisp interpreter.
When lexical binding is not being used, this variable is nil.
A value of `(t)' indicates an empty environment, otherwise it is an
alist of active lexical bindings.]])
    vars.V.internal_interpreter_environment=vars.Qnil

    vars.defsym('Qautoload','autoload')
    vars.defsym('Qmacro','macro')
    vars.defsym('Qand_rest','&rest')
    vars.defsym('Qand_optional','&optional')
    vars.defsym('Qclosure','closure')

    vars.defsym('Qlexical_binding','lexical-binding')
end
return M