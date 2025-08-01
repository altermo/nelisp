#include "nvim.h"
#include "lisp.h"
#include "buffer.h"
#include "dispextern.h"
#include "frame.h"
#include "lua.h"
#include "termhooks.h"

static void
push_vim_api (lua_State *L, const char *name)
{
  LUALC (L, 5, 1)
  {
    lua_getglobal (L, "vim");
    lua_getfield (L, -1, "api");
    lua_remove (L, -2);
    lua_getfield (L, -1, name);
    lua_remove (L, -2);
    eassert (lua_isfunction (L, -1));
  }
}

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

static void
push_vim_t (lua_State *L, long tabpageid)
{
  LUALC (L, 5, 1)
  {
    lua_getglobal (L, "vim");
    lua_getfield (L, -1, "t");
    lua_remove (L, -2);
    lua_pushnumber (L, tabpageid);
    lua_gettable (L, -2);
    lua_remove (L, -2);
    eassert (lua_istable (L, -1));
  }
}

static void
push_vim_w (lua_State *L, long winid)
{
  LUALC (L, 5, 1)
  {
    lua_getglobal (L, "vim");
    lua_getfield (L, -1, "w");
    lua_remove (L, -2);
    lua_pushnumber (L, winid);
    lua_gettable (L, -2);
    lua_remove (L, -2);
    eassert (lua_istable (L, -1));
  }
}

static void
push_vim_bo (lua_State *L, long bufid)
{
  LUALC (L, 5, 1)
  {
    lua_getglobal (L, "vim");
    lua_getfield (L, -1, "bo");
    lua_remove (L, -2);
    lua_pushnumber (L, bufid);
    lua_gettable (L, -2);
    lua_remove (L, -2);
    eassert (lua_istable (L, -1));
  }
}

static void
push_vim_cmd (lua_State *L, const char *name)
{
  LUALC (L, 5, 1)
  {
    lua_getglobal (L, "vim");
    lua_getfield (L, -1, "cmd");
    lua_remove (L, -2);
    lua_getfield (L, -1, name);
    lua_remove (L, -2);
    eassert (lua_isfunction (L, -1));
  }
}

enum nvim_kind
{
  NVIM_BUFFER,
  NVIM_WINDOW,
  NVIM_TABPAGE,
};

Lisp_Object
nvim_id_to_obj (long id, enum nvim_kind kind, Lisp_Object (create) (long))
{
  Lisp_Object obj;
  LUA (10)
  {
    switch (kind)
      {
      case NVIM_BUFFER:
        push_vim_b (L, id);
        break;
      case NVIM_WINDOW:
        push_vim_w (L, id);
        break;
      case NVIM_TABPAGE:
        push_vim_t (L, id);
        break;
      }
    // vim.b
    lua_getfield (L, -1, "nelisp_reference");
    // vim.b, nil/nelisp_reference
    if (!lua_isnil (L, -1))
      {
        eassert (lua_isfunction (L, -1));
        // vim.b, nelisp_reference
        lua_getfenv (L, -1);
        // vim.b, nelisp_reference, fenv
        lua_rawgeti (L, -1, 1);
        eassert (lua_isuserdata (L, -1));
        // vim.b, nelisp_reference, fenv, obj
        obj = userdata_to_obj (L, -1);
        lua_pop (L, 4);
        return obj;
      }
    lua_pop (L, 1);
    // vim.b
    luaL_dostring (L, "return function() end");
    // vim.b, nelisp_reference
    lua_pushvalue (L, -1);
    // vim.b, nelisp_reference, nelisp_reference
    lua_setfield (L, -3, "nelisp_reference");
    // vim.b, nelisp_reference
    lua_createtable (L, 1, 0);
    // vim.b, nelisp_reference, fenv
    obj = create (id);
    push_obj (L, obj);
    // vim.b, nelisp_reference, fenv, obj
    lua_rawseti (L, -2, 1);
    // vim.b, nelisp_reference, fenv
    lua_setfenv (L, -2);
    // vim.b, nelisp_reference
    lua_pop (L, 2);
  }
  return obj;
}

// --- buffer --

static Lisp_Object
create_buffer (long bufid)
{
  register Lisp_Object buffer;
  register struct buffer *b;
  b = allocate_buffer ();

  b->local_var_alist_ = Qnil;
  b->last_obj_ = Qnil;
  b->downcase_table_ = Vascii_downcase_table;
  b->upcase_table_ = XCHAR_TABLE (Vascii_downcase_table)->extras[0];
  b->case_canon_table_ = XCHAR_TABLE (Vascii_downcase_table)->extras[1];
  b->case_eqv_table_ = XCHAR_TABLE (Vascii_downcase_table)->extras[2];
  b->enable_multibyte_characters_ = Qt;

  b->bufid = bufid;

#if TODO_NELISP_LATER_ELSE
  b->syntax_table_ = BVAR (&buffer_defaults, syntax_table);
  b->category_table_ = BVAR (&buffer_defaults, category_table);
#endif

  XSETBUFFER (buffer, b);
  return buffer;
}

Lisp_Object
nvim_bufid_to_bufobj (long bufid)
{
  return nvim_id_to_obj (bufid, NVIM_BUFFER, create_buffer);
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

Lisp_Object
nvim_create_buf (Lisp_Object name, Lisp_Object inhibit_buffer_hooks)
{
  long bufid;
  UNUSED (inhibit_buffer_hooks);
  CHECK_STRING (name);
  eassert (NILP (nvim_name_to_bufobj (name)));
  LUA (10)
  {
    push_vim_api (L, "nvim_create_buf");
    lua_pushboolean (L, true);
    lua_pushboolean (L, true);
    lua_call (L, 2, 1);
    push_vim_api (L, "nvim_buf_set_name");
    lua_pushvalue (L, -2);
    lua_pushlstring (L, (char *) SDATA (name), SBYTES (name));
    lua_call (L, 2, 0);
    eassert (lua_isnumber (L, -1));
    bufid = lua_tointeger (L, -1);
    lua_pop (L, 1);
  }
  return nvim_bufid_to_bufobj (bufid);
}

Lisp_Object
nvim_buffer_name (struct buffer *b)
{
  long bufid = b->bufid;
  Lisp_Object obj = Qnil;
  LUA (5)
  {
    push_vim_api (L, "nvim_buf_is_valid");
    lua_pushnumber (L, bufid);
    lua_call (L, 1, 1);
    eassert (lua_isboolean (L, -1));
    if (!lua_toboolean (L, -1))
      goto done;
    push_vim_fn (L, "bufname");
    lua_pushnumber (L, bufid);
    lua_call (L, 1, 1);
    eassert (lua_isstring (L, -1));
    TODO_NELISP_LATER; // the returned string should always be the same until
                       // name changed
    size_t len;
    const char *name = lua_tolstring (L, -1, &len);

    TODO_NELISP_LATER;
    if (len == 0)
      obj = make_string (" ", 1);
    else

      obj = make_string (name, len);
    lua_pop (L, 1);
  done:
    lua_pop (L, 1);
  }
  return obj;
}

Lisp_Object
nvim_buffer_list (void)
{
  Lisp_Object buffers = Qnil;
  LUA (20)
  {
    push_vim_api (L, "nvim_list_bufs");
    lua_call (L, 0, 1);
    eassert (lua_istable (L, -1));
    // TODO: maye don't include unloaded buffers
    lua_pushnil (L);
    while (lua_next (L, -2) != 0)
      {
        eassert (lua_isnumber (L, -1));
        buffers = Fcons (nvim_bufid_to_bufobj (lua_tointeger (L, -1)), buffers);
        lua_pop (L, 1);
      }
    lua_pop (L, 1);
  }
  return buffers;
}

void
nvim_set_buffer (struct buffer *b)
{
  eassert (BUFFER_LIVE_P (b));
  LUA (5)
  {
    push_vim_api (L, "nvim_set_current_buf");
    lua_pushnumber (L, b->bufid);
    lua_call (L, 1, 0);
  }
}

struct buffer *
nvim_current_buffer (void)
{
  long bufid;
  LUA (5)
  {
    push_vim_api (L, "nvim_get_current_buf");
    lua_call (L, 0, 1);
    eassert (lua_isnumber (L, -1));
    bufid = lua_tointeger (L, -1);
    lua_pop (L, 1);
  }
  return XBUFFER (nvim_bufid_to_bufobj (bufid));
}

ptrdiff_t
nvim_get_field_zv (struct buffer *b, bool chars)
{
  eassert (BUFFER_LIVE_P (b));
  long bufid = b->bufid;
  ptrdiff_t zv;
  LUA (10)
  {
    push_vim_api (L, "nvim_buf_call");
    lua_pushnumber (L, bufid);
    push_vim_fn (L, "wordcount");
    lua_call (L, 2, 1);
    if (chars)
      lua_getfield (L, -1, "chars");
    else
      lua_getfield (L, -1, "bytes");
    eassert (lua_isnumber (L, -1));
    zv = lua_tointeger (L, -1);
    zv = zv + 1;
    lua_pop (L, 2);
  }
  return zv;
}

ptrdiff_t
nvim_get_field_begv (struct buffer *b, bool chars)
{
  eassert (BUFFER_LIVE_P (b));
  UNUSED (chars);
  return 1;
}

ptrdiff_t
nvim_get_field_pt (struct buffer *b)
{
  eassert (BUFFER_LIVE_P (b));
  long bufid = b->bufid;
  ptrdiff_t pt;
  LUA (10)
  {
    push_vim_api (L, "nvim_buf_call");
    lua_pushnumber (L, bufid);
    push_vim_fn (L, "wordcount");
    lua_call (L, 2, 1);
    lua_getfield (L, -1, "cursor_chars");
    eassert (lua_isnumber (L, -1));
    pt = lua_tointeger (L, -1);
    if (pt == 0)
      pt = 1;
    lua_pop (L, 2);
  }
  return pt;
}

struct pos
{
  ptrdiff_t row;
  ptrdiff_t col;
};

struct pos
nvim_buf_byte1_to_pos0 (ptrdiff_t byte1)
{
  // NOTE: if ever changed to accept buffer as argument, need to temp set that
  // buffer
  struct pos pos;
  eassert (byte1 >= 1);
  ptrdiff_t byte = byte1 - 1;
  if (byte == 0)
    {
      pos.row = 0;
      pos.col = 0;
      return pos;
    }
  LUA (10)
  {
    push_vim_fn (L, "line2byte");
    push_vim_fn (L, "byte2line");
    lua_pushinteger (L, byte);
    lua_call (L, 1, 1);
    eassert (lua_isnumber (L, -1));
    eassert (lua_tointeger (L, -1) != -1);
    pos.row = lua_tointeger (L, -1);
    lua_call (L, 1, 1);
    eassert (lua_isnumber (L, -1));
    eassert (lua_tointeger (L, -1) != -1);
    pos.col = lua_tointeger (L, -1);
    lua_pop (L, 1);
    push_vim_fn (L, "getline");
    lua_pushnumber (L, pos.row);
    lua_call (L, 1, 1);
    eassert (lua_isstring (L, -1));
    ptrdiff_t len = lua_objlen (L, -1);
    lua_pop (L, 1);
    if (pos.col > len)
      {
        pos.col = 0;
        pos.row++;
      }
  }
  return pos;
}

void
nvim_buf_memcpy (unsigned char *dst, ptrdiff_t beg1, ptrdiff_t size)
{
  eassert (1 <= beg1);
  ptrdiff_t end1 = size + beg1;
  eassert (end1 <= ZV_BYTE);

  struct pos start_pos = nvim_buf_byte1_to_pos0 (beg1);
  struct pos end_pos = nvim_buf_byte1_to_pos0 (end1);

  long bufid = current_buffer->bufid;
  LUA (10)
  {
    lua_getglobal (L, "table");
    lua_getfield (L, -1, "concat");
    lua_remove (L, -2);
    push_vim_api (L, "nvim_buf_get_text");
    lua_pushnumber (L, bufid);
    lua_pushnumber (L, start_pos.row);
    lua_pushnumber (L, start_pos.col);
    if (end1 == ZV_BYTE)
      {
        // neovim automatically adds a newline at the end of the buffer,
        // but the api doesn't recognize this extra character,
        // so we need to get one less char and then append the newline.
        lua_pushnumber (L, end_pos.row - 1);
        lua_pushnumber (L, -1);
      }
    else
      {
        lua_pushnumber (L, end_pos.row);
        lua_pushnumber (L, end_pos.col);
      }
    lua_newtable (L);
    lua_call (L, 6, 1);
    eassert (lua_istable (L, -1));
    lua_pushliteral (L, "\n");
    lua_call (L, 2, 1);
    if (end1 == ZV_BYTE)
      {
        lua_pushliteral (L, "\n");
        lua_concat (L, 2);
      }
    eassert (lua_objlen (L, -1) == (unsigned long) size);
    const char *text = lua_tostring (L, -1);
    memcpy (dst, text, size);
    lua_pop (L, 1);
  }
}

bool
nvim_buffer_option_is_true (struct buffer *b, const char opt[])
{
  bool optval;
  eassert (BUFFER_LIVE_P (b));
  LUA (10)
  {
    push_vim_bo (L, b->bufid);
    lua_getfield (L, -1, opt);
    eassert (lua_isboolean (L, -1));
    optval = lua_toboolean (L, -1);
    lua_pop (L, 2);
  }
  return optval;
}

Lisp_Object
nvim_buffer_filename (struct buffer *b)
{
  long bufid = b->bufid;
  Lisp_Object obj = Qnil;
  LUA (5)
  {
    push_vim_api (L, "nvim_buf_is_valid");
    lua_pushnumber (L, bufid);
    lua_call (L, 1, 1);
    eassert (lua_isboolean (L, -1));
    if (!lua_toboolean (L, -1))
      {
        lua_pop (L, 1);
        return Qnil;
      }
    lua_pop (L, 1);

    push_vim_bo (L, bufid);
    lua_getfield (L, -1, "buftype");
    eassert (lua_isstring (L, -1));
    if (lua_objlen (L, -1) != 0)
      {
        lua_pop (L, 2);
        return Qnil;
      }
    lua_pop (L, 2);

    push_vim_fn (L, "fnamemodify");
    push_vim_fn (L, "bufname");
    lua_pushnumber (L, bufid);
    lua_call (L, 1, 1);
    eassert (lua_isstring (L, -1));
    TODO_NELISP_LATER; // the returned string should always be the same until
                       // name changed
    if (lua_objlen (L, -1) == 0)
      {
        lua_pop (L, 2);
        return Qnil;
      }
    lua_pushliteral (L, ":p");
    lua_call (L, 2, 1);
    eassert (lua_isstring (L, -1));
    size_t len;
    const char *name = lua_tolstring (L, -1, &len);
    obj = make_string (name, len);
    lua_pop (L, 1);
  }
  return obj;
}

void
nvim_buffer_set_bool_option (struct buffer *b, const char opt[], bool value)
{
  eassert (BUFFER_LIVE_P (b));

  LUA (5)
  {
    push_vim_bo (L, b->bufid);
    lua_pushboolean (L, value);
    lua_setfield (L, -2, opt);
    lua_pop (L, 1);
  }
}

Lisp_Object
nvim_buffer_undo_list (struct buffer *b)
{
  TODO_NELISP_LATER;

  ptrdiff_t undolevels;
  eassert (BUFFER_LIVE_P (b));

  LUA (5)
  {
    push_vim_bo (L, b->bufid);
    lua_getfield (L, -1, "undolevels");
    eassert (lua_isnumber (L, -1));
    undolevels = lua_tointeger (L, -1);
    lua_pop (L, 2);
  }

  return undolevels == -1 ? Qt : (TODO, Qnil);
}
void
nvim_buffer_set_undo_list (struct buffer *b, Lisp_Object value)
{
  TODO_NELISP_LATER;

  eassert (BUFFER_LIVE_P (b));

  LUA (5)
  {
    push_vim_bo (L, b->bufid);
    if (EQ (value, Qt))
      lua_pushinteger (L, -1);
    else
      lua_pushnil (L);
    lua_setfield (L, -2, "undolevels");
    lua_pop (L, 1);
  }
}

bool
nvim_buffer_kill (struct buffer *b)
{
  TODO_NELISP_LATER;

  eassert (BUFFER_LIVE_P (b));
  bool success;

  LUA (10)
  {
    push_vim_api (L, "nvim_buf_delete");
    lua_pushnumber (L, b->bufid);
    lua_newtable (L);
    int err = lua_pcall (L, 2, 0, 0);
    if (err == 0)
      success = true;
    else
      {
        success = false;
        lua_pop (L, 1);
      }
  }
  return success;
}

void
nvim_set_point (ptrdiff_t position)
{
  if (position < 0)
    position = 0;
  LUA (10)
  {
    push_vim_cmd (L, "goto");
    lua_createtable (L, 0, 1);
    lua_createtable (L, 1, 0);
    lua_pushnumber (L, position);
    lua_rawseti (L, -2, 1);
    lua_setfield (L, -2, "range");
    lua_call (L, 1, 0);
  }
}

void
nvim_buf_insert (const char *string, ptrdiff_t nbytes)
{
  if (nbytes == 0)
    return;

  LUA (20)
  {
    push_vim_api (L, "nvim_buf_set_text");
    lua_pushnumber (L, 0);
    push_vim_api (L, "nvim_win_get_cursor");
    lua_pushnumber (L, 0);
    lua_call (L, 1, 1);
    eassert (lua_istable (L, -1));
    lua_rawgeti (L, -1, 1);
    eassert (lua_isnumber (L, -1));
    lua_pushnumber (L, lua_tonumber (L, -1) - 1);
    lua_remove (L, -2);
    lua_rawgeti (L, -2, 2);
    eassert (lua_isnumber (L, -1));
    lua_remove (L, -3);
    lua_pushvalue (L, -2);
    lua_pushvalue (L, -2);
    lua_createtable (L, 1, 0);

    long index = 1;
    char *next_newline;
    while ((next_newline = memchr (string, '\n', nbytes)))
      {
        ptrdiff_t diff = next_newline - string;
        nbytes = nbytes - diff - 1;
        lua_pushlstring (L, string, diff);
        lua_rawseti (L, -2, index++);
        string = next_newline + 1;
      }
    lua_pushlstring (L, string, nbytes);
    lua_rawseti (L, -2, index);
    lua_call (L, 6, 0);
  }
}

// --- terminal --
static struct terminal _terminal_sentinel;
static bool _terminal_sentinel_inited;
struct terminal *
nvim_terminal_sentinel (void)
{
  struct terminal *ptr = &_terminal_sentinel;
  if (!_terminal_sentinel_inited)
    {
      int memlen = VECSIZE (struct terminal);
      int lisplen = PSEUDOVECSIZE (struct terminal, _last_obj);
      XSETPVECTYPESIZE (ptr, PVEC_TERMINAL, lisplen, memlen - lisplen);

      ptr->name = "nvim";

      _terminal_sentinel_inited = true;
    }

  return ptr;
}

// --- frame (tabpage) --

Lisp_Object
create_frame (long id)
{
  Lisp_Object frame;
  struct frame *f;
  f = allocate_frame ();

  f->face_hash_table
    = make_hash_table (&hashtest_eq, DEFAULT_HASH_SIZE, Weak_None, false);

  f->face_cache = NULL;

  if (!noninteractive)
    init_frame_faces (f);

  f->tabpageid = id;

  XSETFRAME (frame, f);
  return frame;
}

Lisp_Object
nvim_tabpageid_to_frameobj (long id)
{
  return nvim_id_to_obj (id, NVIM_TABPAGE, create_frame);
}

Lisp_Object
nvim_frame_list (void)
{
  Lisp_Object frames = Qnil;
  LUA (10)
  {
    push_vim_api (L, "nvim_list_tabpages");
    lua_call (L, 0, 1);
    eassert (lua_istable (L, -1));
    lua_pushnil (L);
    while (lua_next (L, -2))
      {
        eassert (lua_isnumber (L, -1));
        frames
          = Fcons (nvim_tabpageid_to_frameobj (lua_tointeger (L, -1)), frames);
        lua_pop (L, 1);
      }
    lua_pop (L, 1);
  }
  return Fnreverse (frames);
}

bool
nvim_frame_is_valid (struct frame *f)
{
  bool ret;
  LUA (10)
  {
    push_vim_api (L, "nvim_tabpage_is_valid");
    lua_pushnumber (L, f->tabpageid);
    lua_call (L, 1, 1);
    eassert (lua_isboolean (L, -1));
    ret = lua_toboolean (L, -1);
    lua_pop (L, 1);
  }
  return ret;
}

Lisp_Object
nvim_get_current_frame (void)
{
  long id;
  LUA (10)
  {
    push_vim_api (L, "nvim_get_current_tabpage");
    lua_call (L, 0, 1);
    eassert (lua_isnumber (L, -1));
    id = lua_tointeger (L, -1);
    lua_pop (L, 1);
  }
  return nvim_tabpageid_to_frameobj (id);
}

Lisp_Object
nvim_frame_name (struct frame *f)
{
  TODO_NELISP_LATER;
  return Qnil;
}

int
nvim_frame_lines (struct frame *f)
{
  TODO_NELISP_LATER;
  int height;
  LUA (5)
  {
    push_vim_api (L, "nvim_get_option_value");
    lua_pushliteral (L, "lines");
    lua_newtable (L);
    lua_call (L, 2, 1);
    eassert (lua_isnumber (L, -1));
    height = lua_tointeger (L, -1);
    lua_pop (L, 1);
  }
  return height;
}

int
nvim_frame_cols (struct frame *f)
{
  TODO_NELISP_LATER;
  int width;
  LUA (5)
  {
    push_vim_api (L, "nvim_get_option_value");
    lua_pushliteral (L, "columns");
    lua_newtable (L);
    lua_call (L, 2, 1);
    eassert (lua_isnumber (L, -1));
    width = lua_tointeger (L, -1);
    lua_pop (L, 1);
  }
  return width;
}

bool
nvim_frame_wants_modeline_p (struct frame *f)
{
  TODO_NELISP_LATER;
  return false;
}
bool
nvim_frame_no_split_p (struct frame *f)
{
  TODO_NELISP_LATER;
  return false;
}
Lisp_Object
nvim_frame_buried_buffer_list (struct frame *f)
{
  TODO_NELISP_LATER;
  return Qnil;
}
int
nvim_frame_menu_bar_lines (struct frame *f)
{
  TODO_NELISP_LATER;
  return 0;
}
int
nvim_frame_tab_bar_lines (struct frame *f)
{
  TODO_NELISP_LATER;
  return 0;
}

Lisp_Object
nvim_frame_buffer_list (struct frame *f)
{
  TODO_NELISP_LATER;
  int tabpageid = f->tabpageid;
  Lisp_Object buffers = Qnil;
  LUA (20)
  {
    lua_newtable (L);
    // tbl
    push_vim_api (L, "nvim_tabpage_list_wins");
    lua_pushinteger (L, tabpageid);
    lua_call (L, 1, 1);
    eassert (lua_istable (L, -1));
    // tbl, list
    lua_pushnil (L);
    while (lua_next (L, -2) != 0)
      {
        eassert (lua_isnumber (L, -1));
        // tbl, list, *, winid
        push_vim_api (L, "nvim_win_get_buf");
        lua_pushvalue (L, -2);
        lua_call (L, 1, 1);
        eassert (lua_isnumber (L, -1));
        // tbl, list, *, winid, bufid
        lua_rawseti (L, -5, lua_tointeger (L, -1));
        // tbl, list, *, winid
        lua_pop (L, 1);
      }
    // tbl, list
    lua_pop (L, 1);
    // tbl
    lua_pushnil (L);
    while (lua_next (L, -2) != 0)
      {
        eassert (lua_isnumber (L, -1));
        buffers = Fcons (nvim_bufid_to_bufobj (lua_tointeger (L, -1)), buffers);
        lua_pop (L, 1);
      }
    lua_pop (L, 1);
  }
  return buffers;
}

struct terminal *
nvim_frame_terminal (struct frame *f)
{
  if (!FRAME_LIVE_P (f))
    return NULL;
  return nvim_terminal_sentinel ();
}
