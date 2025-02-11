// TODO: In the future, this will be auto generated

#include "lual.c"

int pub ret(LObj) F_cons(lua_State *L) {
    VALIDATE(L,LObj,LObj);
    global_lua_state = L;

    // car(-2), cdr(-1)
    Lisp_Object cdr=userdata_to_obj(L);
    lua_insert(L,-2);
    // cdr(-2), car(-1)
    Lisp_Object car=userdata_to_obj(L);
    Lisp_Object obj=Fcons(car,cdr);
    push_obj(L,obj);
    return 1;
}
