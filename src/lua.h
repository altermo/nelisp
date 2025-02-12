#include "lisp.h"

#ifdef ENABLE_CHECKING
#error "TODO"
#else
#define set_obj_check(L,idx)
#define check_obj(L,idx)
#endif


static Lisp_Object userdata_to_obj(lua_State *L){
    if (!lua_checkstack(L,lua_gettop(L)+5))
        luaL_error(L,"Lua stack overflow");
    check_obj(L,-1);

    if (lua_islightuserdata(L,-1)){
        Lisp_Object obj=(Lisp_Object)lua_touserdata(L,-1);
        eassert(FIXNUMP(obj));
        return obj;
    } else {
        Lisp_Object obj=*(Lisp_Object*)lua_touserdata(L,-1);
        eassert(!FIXNUMP(obj));
        return obj;
    }
}

static void push_obj(lua_State *L, Lisp_Object obj){
    if (!lua_checkstack(L,lua_gettop(L)+10))
        luaL_error(L,"Lua stack overflow");
    if (FIXNUMP(obj)) {
        lua_pushlightuserdata(L,obj);
        set_obj_check(L,-1);
        return;
    }
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
        Lisp_Object* ptr=(Lisp_Object*)lua_touserdata(L,-1);
        eassert(*ptr==obj);
        lua_remove(L,-2);
        // (-1)obj
        return;
    }
    // (-2)memtbl, (-1)nil
    lua_pop(L,2);
    //
    Lisp_Object* ptr=(Lisp_Object*)lua_newuserdata(L,sizeof(Lisp_Object));
    *ptr=obj;
    // (-1)obj
    lua_getfield(L,LUA_ENVIRONINDEX,"memtbl");
    eassert(lua_istable(L,-1));
    // (-2)obj, (-1)memtbl
    lua_pushlstring(L,u.c,sizeof(Lisp_Object));
    // (-3)obj, (-2)memtbl, (-1)idx
    lua_pushvalue(L,-3);
    // (-4)obj, (-3)memtbl, (-2)idx, (-1)obj
    lua_settable(L,-3);
    // (-2)obj, (-1)memtbl
    lua_pop(L,1);
    // (-1)obj
    set_obj_check(L,-1);
    return;
}

#define pub __attribute__((visibility("default")))
#define ret(...)

inline static void check_nargs(lua_State *L,int nargs){
    if (nargs != lua_gettop(L))
        luaL_error(L,"Wrong number of arguments: expected %d, got %d",nargs,lua_gettop(L));
}
inline static void check_isnumber(lua_State *L,int n){
    if (!lua_isnumber(L,n))
        luaL_error(L,"Wrong argument #%d: expected number, got %s",n,lua_typename(L,lua_type(L,n)));
}
inline static void check_isstring(lua_State *L,int n){
    if (!lua_isstring(L,n))
        luaL_error(L,"Wrong argument #%d: expected string, got %s",n,lua_typename(L,lua_type(L,n)));
}
inline static void check_isobject(lua_State *L,int n){
    if (!lua_isuserdata(L,n))
        luaL_error(L,"Wrong argument #%d: expected userdata(lisp object), got %s",n,lua_typename(L,lua_type(L,n)));
    check_obj(L,n);
}

inline static void check_istable(lua_State *L,int n){
    if (!lua_istable(L,n))
        luaL_error(L,"Wrong argument #%d: expected table, got %s",n,lua_typename(L,lua_type(L,n)));
}
