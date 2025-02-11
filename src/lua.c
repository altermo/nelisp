#include "alloc.c"
#include "lual.c"

int pub number_to_fixnum(lua_State *L){
    global_lua_state = L;
    eassert(lua_isnumber(L,-1));
    Lisp_Object obj = make_fixnum(lua_tointeger(L,-1));
    push_fixnum_as_lightuserdata(L,obj);
    return 1;
}

int pub number_to_float(lua_State *L){
    global_lua_state = L;
    eassert(lua_isnumber(L,-1));
    Lisp_Object obj = make_float(lua_tonumber(L,-1));
    push_obj_as_userdata(L,obj);
    return 1;
}

int pub fixnum_to_number(lua_State *L){
    global_lua_state = L;
    eassert(lua_isuserdata(L,-1));
    Lisp_Object obj = userdata_to_obj(L);
    if (!FIXNUMP(obj))
        luaL_error(L,"Expected fixnum");
    EMACS_INT i = XFIXNUM(obj);
    lua_pushinteger(L, i);
    return 1;
}

int pub float_to_number(lua_State *L){
    global_lua_state = L;
    eassert(lua_isuserdata(L,-1));
    Lisp_Object obj = userdata_to_obj(L);
    if (!FLOATP(obj))
        luaL_error(L,"Expected float");
    double i = XFLOAT_DATA(obj);
    lua_pushnumber(L, i);
    return 1;
}

int pub string_to_unibyte_lstring(lua_State *L){
    global_lua_state = L;
    eassert(lua_isstring(L,-1));
    size_t len;
    const char* str = lua_tolstring(L,-1,&len);
    Lisp_Object obj = make_unibyte_string(str,len);
    push_obj_as_userdata(L,obj);
    return 1;
}

int pub unibyte_lstring_to_string(lua_State *L){
    global_lua_state = L;
    eassert(lua_isuserdata(L,-1));
    Lisp_Object obj = userdata_to_obj(L);
    if (!STRINGP(obj) || STRING_MULTIBYTE(obj))
        luaL_error(L,"Expected unibyte string");
    const char* str = (const char*)SDATA(obj);
    lua_pushlstring(L,str,SBYTES(obj));
    return 1;
}

int pub cons_to_table(lua_State *L){
    global_lua_state = L;
    eassert(lua_isuserdata(L,-1));
    Lisp_Object obj = userdata_to_obj(L);
    if (!CONSP(obj))
        luaL_error(L,"Expected cons");
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
    return 1;
}

int pub collectgarbage(lua_State *L){
    global_lua_state = L;
    garbage_collect_();
    return 0;
}
