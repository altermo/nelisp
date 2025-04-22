#ifndef LUA_H
#define LUA_H

#include "lisp.h"

// Sets the variable `L` to the global lua-state.
// The lua-state stack capacity is grown by stack.top+{n} elements.
// Asserts that the lua-state stack didn't change after run.
// WARNING: DO NOT RETURN OR JUMP OUT OF THE MACRO.
#define LUA(n) LUAC (n, 0)

// {L} is the lua-state to be used.
// The lua-state stack capacity is grown by stack.top+{n} elements.
// Asserts that the lua-state stack didn't change after run.
// WARNING: DO NOT RETURN OR JUMP OUT OF THE MACRO.
#define LUAL(L, n) LUALC (L, n, 0)

// Sets the variable `L` to the global lua-state.
// The lua-state stack capacity is grown by stack.top+{n} elements.
// Asserts that the lua-state stack changed by {change}
// WARNING: DO NOT RETURN OR JUMP OUT OF THE MACRO.
#define LUAC(n, change)                \
  LUALC (_global_lua_state, n, change) \
  for (lua_State *L = _global_lua_state; run; run = 0)

// {L} is the lua-state to be used.
// The lua-state stack capacity is grown by stack.top+{n} elements.
// Asserts that the lua-state stack changed by {change}
// WARNING: DO NOT RETURN OR JUMP OUT OF THE MACRO.
#define LUALC(L, n, change)                                      \
  for (int top = (STATIC_ASSERT (n > 0, n_needs_to_be_positive), \
                  _lcheckstack (L, n), lua_gettop (L) + change), \
           run = 1;                                              \
       (eassert (run || top == lua_gettop (L)), run); run = 0)

#endif
