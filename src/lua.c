#ifndef EMACS_LUA_C
#define EMACS_LUA_C

#include "alloc.c"
#include "lread.c"
#include "data.c"
#include "eval.c"

void t_number_to_fixnum(lua_State *L){
    Lisp_Object obj = make_fixnum(lua_tointeger(L,-1));
    push_obj(L,obj);
}
int pub ret(/*nelisp.obj*/) number_to_fixnum(lua_State *L){
    check_nargs(L,1);
    check_isnumber(L,1);
    tcall(L,t_number_to_fixnum);
    return 1;
}

void t_number_to_float(lua_State *L){
    Lisp_Object obj = make_float(lua_tonumber(L,-1));
    push_obj(L,obj);
}
int pub ret(/*nelisp.obj*/) number_to_float(lua_State *L){
    check_nargs(L,1);
    check_isnumber(L,1);
    tcall(L,t_number_to_float);
    return 1;
}

void t_fixnum_to_number(lua_State *L){
    Lisp_Object obj = userdata_to_obj(L,1);
    eassert(FIXNUMP(obj));
    EMACS_INT i = XFIXNUM(obj);
    lua_pushinteger(L, i);
}
int pub ret(/*number*/) fixnum_to_number(lua_State *L){
    check_nargs(L,1);
    check_isobject(L,1);
    Lisp_Object obj = userdata_to_obj(L,1);
    if (!FIXNUMP(obj))
        luaL_error(L,"Expected fixnum");
    tcall(L,t_fixnum_to_number);
    return 1;
}

void t_float_to_number(lua_State *L){
    Lisp_Object obj = userdata_to_obj(L,1);
    eassert(FLOATP(obj));
    double i = XFLOAT_DATA(obj);
    lua_pushnumber(L, i);
}
int pub ret(/*number*/) float_to_number(lua_State *L){
    check_nargs(L,1);
    check_isobject(L,1);
    Lisp_Object obj = userdata_to_obj(L,1);
    if (!FLOATP(obj))
        luaL_error(L,"Expected float");
    tcall(L,t_float_to_number);
    return 1;
}

void t_string_to_unibyte_lstring(lua_State *L){
    size_t len;
    const char* str = lua_tolstring(L,-1,&len);
    Lisp_Object obj = make_unibyte_string(str,len);
    push_obj(L,obj);
}
int pub ret(/*nelisp.obj*/) string_to_unibyte_lstring(lua_State *L){
    check_nargs(L,1);
    check_isstring(L,1);
    tcall(L,t_string_to_unibyte_lstring);
    return 1;
}

void t_unibyte_lstring_to_string(lua_State *L){
    Lisp_Object obj = userdata_to_obj(L,1);
    eassert(STRINGP(obj) && !STRING_MULTIBYTE(obj));
    const char* str = (const char*)SDATA(obj);
    lua_pushlstring(L,str,SBYTES(obj));
}
int pub ret(/*string*/) unibyte_lstring_to_string(lua_State *L){
    check_nargs(L,1);
    check_isobject(L,1);
    Lisp_Object obj = userdata_to_obj(L,1);
    if (!STRINGP(obj) || STRING_MULTIBYTE(obj))
        luaL_error(L,"Expected unibyte string");
    tcall(L,t_unibyte_lstring_to_string);
    return 1;
}

void t_cons_to_table(lua_State *L){
    Lisp_Object obj = userdata_to_obj(L,1);
    eassert(CONSP(obj));
    Lisp_Object car=XCAR(obj);
    Lisp_Object cdr=XCDR(obj);
    lua_newtable(L);
    // (-1)tbl
    push_obj(L,car);
    // (-2)tbl,(-1)car
    lua_rawseti(L,-2,1);
    // (-1)tbl
    push_obj(L,cdr);
    // (-2)tbl,(-1)cdr
    lua_rawseti(L,-2,2);
    // (-1)tbl
}
int pub ret(/*[nelisp.obj,nelisp.obj]*/) cons_to_table(lua_State *L){
    check_nargs(L,1);
    check_isobject(L,1);
    Lisp_Object obj = userdata_to_obj(L,1);
    if (!CONSP(obj))
        luaL_error(L,"Expected cons");
    tcall(L,t_cons_to_table);
    return 1;
}

void t_vector_to_table(lua_State *L){
    Lisp_Object obj = userdata_to_obj(L,1);
    eassert(VECTORP(obj));
    lua_newtable(L);
    // (-1)tbl
    ptrdiff_t len = ASIZE(obj);
    for (ptrdiff_t i=0; i<len; i++){
        // (-1)tbl
        push_obj(L,XVECTOR(obj)->contents[i]);
        // (-2)tbl,(-1)obj
        lua_rawseti(L,-2,i+1);
        // (-1)tbl
    };
    // (-1)tbl
}
int pub ret(/*nelisp.obj[]*/) vector_to_table(lua_State *L){
    check_nargs(L,1);
    check_isobject(L,1);
    Lisp_Object obj = userdata_to_obj(L,1);
    if (!VECTORP(obj))
        luaL_error(L,"Expected vector");
    tcall(L,t_vector_to_table);
    return 1;
}

void t__get_symbol(lua_State *L){
    for (unsigned long i = 0; i < ARRAYELTS (lispsym); i++) {
        size_t len;
        lua_tolstring(L,-1,&len);
        if (memcmp(lua_tolstring(L,-1,&len),defsym_name[i],len+1)==0){
            push_obj(L,builtin_lisp_symbol(i));
            return;
        }
    }
    lua_pushnil(L);
}
int pub ret(/*nelisp.obj*/) _get_symbol(lua_State *L){
    check_nargs(L,1);
    check_isstring(L,1);
    tcall(L,t__get_symbol);
    if (lua_isnil(L,-1))
        luaL_error(L,"Symbol '%s' not found",lua_tostring(L,1));
    return 1;
}

void t_eval(lua_State *L){
    size_t len;
    const char* str = lua_tolstring(L,-1,&len);
    Lisp_Object readcharfun = make_unibyte_string(str,len);
    read_from_string_index = 0;
    read_from_string_index_byte = string_char_to_byte (readcharfun, 0);
    read_from_string_limit = len;
    Lisp_Object ret=NULL;
    while (1){
        int c = READCHAR;
        if (c == ';')
        {
            while ((c = READCHAR) != '\n' && c != -1);
            continue;
        }
        if (c < 0)
        {
            break;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r'
            || c == NO_BREAK_SPACE)
            continue;
        UNREAD (c);
        Lisp_Object val = read0 (readcharfun, false);
        ret = eval_sub (val);
    }
    if (ret)
        push_obj(L,ret);
    else
        lua_pushnil(L);
}
int pub ret(/*nelisp.obj|nil*/) eval(lua_State *L){
    check_nargs(L,1);
    check_isstring(L,1);
    tcall(L,t_eval);
    return 1;
}

void t_collectgarbage(lua_State *L){
    UNUSED(L);
    garbage_collect();
}
int pub ret() collectgarbage(lua_State *L){
    check_nargs(L,0);
    tcall(L,t_collectgarbage);
    return 0;
}

bool inited=false;
int pub ret() init(lua_State *L){
    global_lua_state = L;
    check_nargs(L,0);
    if (inited)
        return 0;
    inited=true;

    init_alloc_once();
    init_eval_once();
    init_obarray_once();

    syms_of_lread();
    syms_of_data();
    syms_of_alloc();
    syms_of_eval();
    syms_of_fns();

    if (!setjmp(mainloop_return_jmp)){
        mainloop();
    }

    return 0;
}
#endif
