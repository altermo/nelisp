// TODO: In the future, this will be auto generated

#include "lual.c"

int pub F_cons(lua_State *L) {
    global_lua_state = L;
    eassert(lua_isuserdata(L,-1));
    eassert(lua_isuserdata(L,-2));
    // car(-2), cdr(-1)
    Lisp_Object cdr=userdata_to_obj(L);
    lua_insert(L,-2);
    // cdr(-2), car(-1)
    Lisp_Object car=userdata_to_obj(L);
    Lisp_Object obj=Fcons(car,cdr);
    if (FIXNUMP(obj)){
        push_fixnum_as_lightuserdata(L,obj);
    } else {
        push_obj_as_userdata(L,obj);
    }
    return 1;
}
