#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include "../lisp.h"
#include "../alloc.c"

static void push_fixnum_as_lightuserdata(lua_State *L, Lisp_Object obj){
    if (!lua_checkstack(L,lua_gettop(L)+5))
        luaL_error(L,"Lua stack overflow");

    eassert(FIXNUMP(obj));

    lua_pushlightuserdata(L,obj);
    // (-1)obj
#ifdef ENABLE_CHECKING
    if (!suppress_checking){
        lua_getfield(L,LUA_ENVIRONINDEX,"obj_mt");
        eassert(lua_istable(L,-1));
        // (-2)obj, (-1)mt
        lua_setmetatable(L,-2);
        // (-1)obj
    }
#endif
};

static void push_obj_as_userdata(lua_State *L, Lisp_Object obj){
    if (!lua_checkstack(L,lua_gettop(L)+10))
        luaL_error(L,"Lua stack overflow");

    eassert(!FIXNUMP(obj));

    union {
        Lisp_Object l;
        char c[sizeof(Lisp_Object)];
    } u;
    u.l=obj;

    lua_getfield(L,LUA_ENVIRONINDEX,"memtbl");
    eassert(lua_istable(L,-1));
    // (-1)memtbl
    lua_pushlstring(L,u.c,sizeof(Lisp_Object));
    // (-2)memtbl, (-1)idx
    lua_gettable(L,-2);
    // (-2)memtbl, (-1)nil/obj
    if (lua_isuserdata(L,-1)){
        // (-2)memtbl, (-1)obj
        Lisp_Object* ptr=lua_touserdata(L,-1);
        eassert(ptr==&obj);
        lua_remove(L,-2);
        // (-1)obj
        return;
    }
    // (-2)memtbl, (-1)nil
    lua_pop(L,2);
    //
    Lisp_Object* ptr=lua_newuserdata(L,sizeof(Lisp_Object));
    ptr=&obj;
    // (-1)obj
    lua_getfield(L,LUA_ENVIRONINDEX,"memtbl");
    eassert(lua_istable(L,-1));
    // (-2)obj, (-1)memtbl
    lua_pushlstring(L,u.c,sizeof(Lisp_Object));
    // (-3)obj, (-2)memtbl, (-1)idx
    lua_pushvalue(L,-3);
    // (-4)obj, (-3)memtbl, (-2)idx, (-1)obj
#ifdef ENABLE_CHECKING
    if (!suppress_checking){
        lua_getfield(L,LUA_ENVIRONINDEX,"obj_mt");
        eassert(lua_istable(L,-1));
        // (-5)obj, (-4)memtbl, (-3)idx, (-2)obj, (-1)mt
        lua_setmetatable(L,-2);
        // (-4)obj, (-3)memtbl, (-2)idx, (-1)obj
    }
#endif
    lua_settable(L,-3);
    // (-2)obj, (-1)memtbl
    lua_pop(L,1);
    // (-1)obj
}

static Lisp_Object userdata_to_obj(lua_State *L){
    eassert(lua_isuserdata(L,-1));
    if (!lua_checkstack(L,lua_gettop(L)+5))
        luaL_error(L,"Lua stack overflow");

    // (-1)obj
#ifdef ENABLE_CHECKING
    if (!suppress_checking){
        lua_getfield(L,LUA_ENVIRONINDEX,"obj_mt");
        eassert(lua_istable(L,-1));
        // (-2)obj, (-1)mt
        lua_getmetatable(L,-1);
        // (-3)obj, (-2)mt, (-1)mt
        eassert(lua_equal(L,-1,-2));
        lua_pop(L,2);
        // (-1)obj
    }
#endif
    if (lua_islightuserdata(L,-1)){
        Lisp_Object obj=lua_touserdata(L,-1);
        eassert(FIXNUMP(obj));
        return obj;
    } else {
        Lisp_Object obj=*(Lisp_Object*)lua_touserdata(L,-1);
        eassert(!FIXNUMP(obj));
        return obj;
    }
};

int number_to_fixnum(lua_State *L){
    eassert(lua_isnumber(L,-1));
    Lisp_Object obj = make_fixnum(lua_tointeger(L,-1));
    push_fixnum_as_lightuserdata(L,obj);
    return 1;
};

int number_to_float(lua_State *L){
    eassert(lua_isnumber(L,-1));
    Lisp_Object obj = make_float(lua_tonumber(L,-1));
    push_obj_as_userdata(L,obj);
    return 1;
};

int fixnum_to_number(lua_State *L){
    eassert(lua_isuserdata(L,-1));
    Lisp_Object obj = userdata_to_obj(L);
    if (!FIXNUMP(obj))
        luaL_error(L,"Expected fixnum");
    EMACS_INT i = XFIXNUM(obj);
    lua_pushinteger(L, i);
    return 1;
};
