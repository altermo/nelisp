#ifndef LUA_H
#define LUA_H

#include "lisp.h"

#define LUA(n) LUAC (n, 0)

#define LUAL(L, n) LUALC (L, n, 0)

#define LUAC(n, change)                \
  LUALC (_global_lua_state, n, change) \
  for (lua_State *L = _global_lua_state; run; run = 0)

#define LUALC(L, n, change)                                      \
  for (int top = (STATIC_ASSERT (n > 0, n_needs_to_be_positive), \
                  _lcheckstack (L, n), lua_gettop (L) + change), \
           run = 1;                                              \
       (eassert (run || top == lua_gettop (L)), run); run = 0)

#endif
