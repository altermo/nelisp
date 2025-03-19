#ifndef LUA_H
#define LUA_H

#include "lisp.h"

#define LUA(n)                                        \
  for (lua_State *L = _global_lua_state; L; L = NULL) \
  LUAL (L, n)

#define LUAL(L, n) LUALC (L, n, 0)

#define LUAC(n, change)                               \
  for (lua_State *L = _global_lua_state; L; L = NULL) \
  LUALC (L, n, change)

#define LUALC(L, n, change)                                          \
  for (int top = (STATIC_ASSERT (n > 0, n_needs_to_be_positive),     \
                  _lcheckstack (L, n), lua_gettop (L) + 1 + change); \
       (eassert (top > 0 || -top - 1 == lua_gettop (L)), top > 0); top = -top)

#endif
