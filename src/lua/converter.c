#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include "../lisp.h"

static void push_fixnum_as_lightuserdata(lua_State *L, Lisp_Object obj){
    if (!lua_checkstack(L,lua_gettop(L)+5))
        luaL_error(L,"Lua stack overflow");

    if (!FIXNUMP(obj))
        luaL_error(L,"Expected fixnum");

    lua_pushlightuserdata(L,obj);
    // (-1)obj
#ifdef ENABLE_CHECKING
    if (!suppress_checking){
        lua_getfield(L,LUA_ENVIRONINDEX,"obj_mt");
        // (-2)obj, (-1)mt
        lua_setmetatable(L,-2);
        // (-1)obj
    }
#endif
};

int number_to_fixnum(lua_State *L){
    eassert(lua_isnumber(L,-1));
    Lisp_Object obj = make_fixnum(lua_tointeger(L,-1));
    push_fixnum_as_lightuserdata(L,obj);
    return 1;
};
