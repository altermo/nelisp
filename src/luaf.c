// TODO: In the future, this will be auto generated

#include "lua.h"

int pub ret(/*nelisp.obj*/) F_cons(lua_State *L) {
    check_nargs(L,2);
    check_isobject(L,1);
    check_isobject(L,2);
    global_lua_state = L;

    Lisp_Object obj=Fcons(userdata_to_obj(L,1),userdata_to_obj(L,2));
    push_obj(L,obj);
    return 1;
}

int pub ret(/*nelisp.obj*/) F_make_vector(lua_State *L) {
    check_nargs(L,2);
    check_isobject(L,1);
    check_isobject(L,2);
    global_lua_state = L;

    Lisp_Object obj=Fmake_vector(userdata_to_obj(L,1),userdata_to_obj(L,2));
    push_obj(L,obj);
    return 1;
}
