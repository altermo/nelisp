#ifndef LUA_H
#define LUA_H

#include "lisp.h"

struct _lua_assertchange
{
  struct lua_State *state;
  int change;
};

void
_lua_assertchange (struct _lua_assertchange *assertchange)
{
  TODO_NELISP_LATER;
  if (lua_gettop (assertchange->state) != assertchange->change)
    TODO;
}

// Sets the variable `L` to the global lua-state.
// The lua-state stack capacity is grown by stack.top+{n} elements.
// Asserts that the lua-state stack didn't change after run.
#define LUA(n) LUAC (n, 0)

// {L} is the lua-state to be used.
// The lua-state stack capacity is grown by stack.top+{n} elements.
// Asserts that the lua-state stack didn't change after run.
#define LUAL(L, n) LUALC (L, n, 0)

// Sets the variable `L` to the global lua-state.
// The lua-state stack capacity is grown by stack.top+{n} elements.
// Asserts that the lua-state stack changed by {change}
#define LUAC(n, change)                \
  LUALC (_global_lua_state, n, change) \
  for (lua_State *L = _global_lua_state; run; run = 0)

// {L} is the lua-state to be used.
// The lua-state stack capacity is grown by stack.top+{n} elements.
// Asserts that the lua-state stack changed by {change}
#define LUALC(L, n, change)                                              \
  for (int _top = (STATIC_ASSERT (n > 0, n_needs_to_be_positive),        \
                   _lcheckstack (L, n), lua_gettop (L) + change),        \
           run = 1;                                                      \
       run; run = 0)                                                     \
    for (__attribute__ ((unused,                                         \
                         cleanup (                                       \
                           _lua_assertchange))) struct _lua_assertchange \
           _assertchange                                                 \
         = { L, _top };                                                  \
         run; run = 0)

#endif
