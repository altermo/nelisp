#ifndef LUA_H
#define LUA_H

#include "lisp.h"

#define LUA(n)                                                       \
  lcheckstack (_global_lua_state, n);                                \
  for (lua_State *L = _global_lua_state; L; L = NULL)                \
    for (int top = lua_gettop (L) + 1;                               \
         (eassert (top > 0 || -top - 1 == lua_gettop (L)), top > 0); \
         top = -top)

#endif
