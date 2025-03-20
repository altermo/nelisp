#include "lisp.h"
#include "buffer.h"
#include "lua.h"

static void
push_vim_fn (lua_State *L, const char *name)
{
  LUALC (L, 5, 1)
  {
    lua_getglobal (L, "vim");
    lua_getfield (L, -1, "fn");
    lua_remove (L, -2);
    lua_getfield (L, -1, name);
    lua_remove (L, -2);
    eassert (lua_isfunction (L, -1));
  }
}

static void
push_vim_b (lua_State *L, long bufid)
{
  LUALC (L, 5, 1)
  {
    lua_getglobal (L, "vim");
    lua_getfield (L, -1, "b");
    lua_remove (L, -2);
    lua_pushnumber (L, bufid);
    lua_gettable (L, -2);
    lua_remove (L, -2);
    eassert (lua_istable (L, -1));
  }
}

static Lisp_Object
create_buffer (long bufid)
{
  register Lisp_Object buffer;
  register struct buffer *b;
  b = allocate_buffer ();

  b->_last_obj = Qnil;
  b->bufid = bufid;

  XSETBUFFER (buffer, b);
  return buffer;
}

Lisp_Object
nvim_bufid_to_bufobj (long bufid)
{
  Lisp_Object obj = NULL;
  LUA (10)
  {
    lua_getfield (L, LUA_ENVIRONINDEX, "buftbl");
    eassert (lua_istable (L, -1));
    // buftbl
    push_vim_b (L, bufid);
    // buftbl, vim.b
    lua_getfield (L, -1, "nelisp_reference");
    // buftbl, vim.b, nil/nelisp_reference
    if (!lua_isnil (L, -1))
      {
        eassert (lua_isfunction (L, -1));
        lua_gettable (L, -3);
        // buftbl, vim.b, userdata
        obj = userdata_to_obj (L, -1);
        lua_pop (L, 1);
        goto done;
      }
    lua_pop (L, 1);
    // buftbl, vim.b
    luaL_dostring (L, "return function() end");
    // buftbl, vim.b, nelisp_reference
    lua_pushvalue (L, -1);
    // buftbl, vim.b, nelisp_reference, nelisp_reference
    lua_setfield (L, -3, "nelisp_reference");
    // buftbl, vim.b, nelisp_reference
    obj = create_buffer (bufid);
    push_obj (L, obj);
    // buftbl, vim.b, nelisp_reference, obj
    lua_settable (L, -4);
  done:
    // buftbl, vim.b
    lua_pop (L, 2);
  }
  eassert (obj);
  return obj;
}

Lisp_Object
nvim_name_to_bufobj (Lisp_Object name)
{
  long bufid;
  CHECK_STRING (name);
  LUA (5)
  {
    push_vim_fn (L, "bufnr");
    lua_pushlstring (L, (char *) SDATA (name), SBYTES (name));
    lua_call (L, 1, 1);
    eassert (lua_isnumber (L, -1));
    bufid = lua_tointeger (L, -1);
    lua_pop (L, 1);
  }
  if (bufid == -1)
    return Qnil;
  return nvim_bufid_to_bufobj (bufid);
}
