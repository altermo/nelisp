#include "lisp.h"

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
}

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
    if (FIXNUMP(obj))
        push_fixnum_as_lightuserdata(L,obj);
    else
        push_obj_as_userdata(L,obj);
}

#define pub __attribute__((visibility("default")))

enum LType {
    LObj,
    LEND,
    LNum,
    LStr,
    LNO_ARG=LEND
};
#define ret(...)
#define VALIDATE(L,...) validate(L,0,__VA_ARGS__,LEND)
static void validate(lua_State *L,int count,...){
#define error(...) do {\
    va_end(ap);\
    luaL_error(L,__VA_ARGS__);\
    return;\
} while(0)

    va_list ap;
    va_start(ap, count);
    int nargs=0;
    while (va_arg(ap, enum LType) != LEND) {
        nargs++;
    };
    va_end(ap);

    if (nargs != lua_gettop(L))
        luaL_error(L,"Wrong number of arguments: expected %d, got %d",nargs,lua_gettop(L));

    enum LType type;
    va_start(ap, count);
    int n=0;
    while ((type=va_arg(ap, enum LType)) != LEND) {
        n++;
        switch (type){
            case LObj:
                if (!lua_isuserdata(L,n))
                    error("Wrong argument #%d: expected userdata(lisp object), got %s",n,lua_typename(L,lua_type(L,n)));
#ifdef ENABLE_CHECKING
            if (!suppress_checking){
                lua_getfield(L,LUA_ENVIRONINDEX,"obj_mt");
                eassert(lua_istable(L,-1));
                lua_getmetatable(L,n);
                if (!lua_equal(L,-1,-2)){
                    error("Wrong argument #%d: userdata not marked as lisp object",n));
                };
                lua_pop(L,2);
            }
#endif
            break;
            case LNum:
                if (!lua_isnumber(L,n))
                    error("Wrong argument #%d: expected number, got %s",n,lua_typename(L,lua_type(L,n)));
                break;
            case LStr:
                if (!lua_isstring(L,n))
                    error("Wrong argument #%d: expected string, got %s",n,lua_typename(L,lua_type(L,n)));
                break;
            case LEND:
                eassert(0);
        }
    };
    va_end(ap);
    return;
    #undef error
}
