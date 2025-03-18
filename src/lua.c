#include <sys/stat.h>

#include "lisp.h"
#include "character.h"

void lcheckstack(lua_State* L,int n){
    if (!lua_checkstack(L,lua_gettop(L)+n))
        luaL_error(L,"Lua stack overflow");
}

Lisp_Object userdata_to_obj(lua_State *L,int idx){
    lcheckstack(L,5);
    check_obj(L,idx);

    if (lua_islightuserdata(L,idx)){
        Lisp_Object obj=(Lisp_Object)lua_touserdata(L,idx);
        eassert(FIXNUMP(obj));
        return obj;
    } else {
        Lisp_Object obj=*(Lisp_Object*)lua_touserdata(L,idx);
        eassert(!FIXNUMP(obj));
        return obj;
    }
}

void push_obj(lua_State *L, Lisp_Object obj){
    lcheckstack(L,10);
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

void check_nargs(lua_State *L,int nargs){
    if (global_lua_state==NULL)
        luaL_error(L,"Nelisp is not inited (please run `require('nelisp.c').init()`)");
    if (unrecoverable_error)
        luaL_error(L,"Previous error was unrecoverable, please restart Neovim");
    if (nargs != lua_gettop(L))
        luaL_error(L,"Wrong number of arguments: expected %d, got %d",nargs,lua_gettop(L));
}
INLINE void check_isnumber(lua_State *L,int n){
    if (!lua_isnumber(L,n))
        luaL_error(L,"Wrong argument #%d: expected number, got %s",n,lua_typename(L,lua_type(L,n)));
}
INLINE void check_isstring(lua_State *L,int n){
    if (!lua_isstring(L,n))
        luaL_error(L,"Wrong argument #%d: expected string, got %s",n,lua_typename(L,lua_type(L,n)));
}
void check_isobject(lua_State *L,int n){
    if (!lua_isuserdata(L,n))
        luaL_error(L,"Wrong argument #%d: expected userdata(lisp object), got %s",n,lua_typename(L,lua_type(L,n)));
    check_obj(L,n);
}

void check_istable_with_obj(lua_State *L,int n){
    if (!lua_istable(L,n))
        luaL_error(L,"Wrong argument #%d: expected table, got %s",n,lua_typename(L,lua_type(L,n)));
    lcheckstack(L,5);
    for (lua_pushnil(L); lua_next(L,n); lua_pop(L,1)){
        if (!lua_isuserdata(L,-1))
            luaL_error(L,"Expected table of userdata(lisp objects)");
        check_obj(L,-1);
    }
}
#define Xkeyvalue()\
    X(1,nil,lua_isnil,"nil")\
    X(2,string,lua_isstring,"string")\
    X(4,boolean,lua_isboolean,"boolean")
#define X(mask,name,check,str) kv_mask_##name=mask,
enum kv_mask{
    Xkeyvalue()
};
#undef X
#define X(mask,name,check,str) str " or "
size_t kv_message_maxlen=sizeof(Xkeyvalue());
#undef X
struct kv_t {
    const char* key;
    enum kv_mask type;
};
INLINE void check_istable_with_keyvalue(lua_State *L,int n,struct kv_t keyvalue[]){
    if (!lua_istable(L,n))
        luaL_error(L,"Wrong argument #%d: expected table, got %s",n,lua_typename(L,lua_type(L,n)));
    lcheckstack(L,5);
    lua_pushnil(L);
    for (struct kv_t* kv=keyvalue; kv->key; kv++){
        lua_pop(L,1);
        lua_getfield(L,-1,kv->key);
        if (lua_isnil(L,-1) && !(kv->type&kv_mask_nil))
            luaL_error(L,"Key `%s` not set",kv->key);
#define X(mask,name,check,str) else if (kv->type&mask && check(L,-1)) continue;
        if (false);
        Xkeyvalue();
#undef X
        char type[kv_message_maxlen];
        char *p=type;
#define X(mask,name,check,str) if (kv->type&(mask)){\
    memcpy(p,str,strlen(str));\
    memcpy(p+strlen(str)," or ",4);\
    p+=strlen(str)+4;\
}
        Xkeyvalue();
#undef X
        memcpy(p-4,"\0",1);
        luaL_error(L,"Expected key `%s` be %s",kv->key,type);
    }
    lua_pop(L,1);
}

thrd_t main_thread;

mtx_t main_mutex;
mtx_t thread_mutex;

cnd_t main_cond;
cnd_t thread_cond;

bool tcall_error=false;
void (*main_func)(void)=NULL;

static void (*tcall_func_cb)(lua_State *L);
INLINE void tcall_func(void){
    tcall_func_cb(global_lua_state);
}
void tcall(lua_State *L,void (*f)(lua_State *L)){
    if (global_lua_state!=L)
        TODO; /*use lua_xmove to move between the states*/ \
    tcall_func_cb=f;
    main_func=tcall_func;

    mtx_lock(&thread_mutex);
    cnd_signal(&thread_cond);
    mtx_unlock(&thread_mutex);
    cnd_wait(&main_cond,&main_mutex);

    if (tcall_error){
        tcall_error=false;
        lua_error(global_lua_state);
    }
}

void t_number_to_fixnum(lua_State *L){
    Lisp_Object obj = make_fixnum(lua_tointeger(L,-1));
    push_obj(L,obj);
}
int pub ret(/*nelisp.obj*/) number_to_fixnum(lua_State *L){
    check_nargs(L,1);
    check_isnumber(L,1);
    tcall(L,t_number_to_fixnum);
    return 1;
}

void t_number_to_float(lua_State *L){
    Lisp_Object obj = make_float(lua_tonumber(L,-1));
    push_obj(L,obj);
}
int pub ret(/*nelisp.obj*/) number_to_float(lua_State *L){
    check_nargs(L,1);
    check_isnumber(L,1);
    tcall(L,t_number_to_float);
    return 1;
}

void t_fixnum_to_number(lua_State *L){
    Lisp_Object obj = userdata_to_obj(L,1);
    eassert(FIXNUMP(obj));
    EMACS_INT i = XFIXNUM(obj);
    lua_pushinteger(L, i);
}
int pub ret(/*number*/) fixnum_to_number(lua_State *L){
    check_nargs(L,1);
    check_isobject(L,1);
    Lisp_Object obj = userdata_to_obj(L,1);
    if (!FIXNUMP(obj))
        luaL_error(L,"Expected fixnum");
    tcall(L,t_fixnum_to_number);
    return 1;
}

void t_float_to_number(lua_State *L){
    Lisp_Object obj = userdata_to_obj(L,1);
    eassert(FLOATP(obj));
    double i = XFLOAT_DATA(obj);
    lua_pushnumber(L, i);
}
int pub ret(/*number*/) float_to_number(lua_State *L){
    check_nargs(L,1);
    check_isobject(L,1);
    Lisp_Object obj = userdata_to_obj(L,1);
    if (!FLOATP(obj))
        luaL_error(L,"Expected float");
    tcall(L,t_float_to_number);
    return 1;
}

void t_string_to_unibyte_lstring(lua_State *L){
    size_t len;
    const char* str = lua_tolstring(L,-1,&len);
    Lisp_Object obj = make_unibyte_string(str,len);
    push_obj(L,obj);
}
int pub ret(/*nelisp.obj*/) string_to_unibyte_lstring(lua_State *L){
    check_nargs(L,1);
    check_isstring(L,1);
    tcall(L,t_string_to_unibyte_lstring);
    return 1;
}

void t_unibyte_lstring_to_string(lua_State *L){
    Lisp_Object obj = userdata_to_obj(L,1);
    eassert(STRINGP(obj) && !STRING_MULTIBYTE(obj));
    const char* str = (const char*)SDATA(obj);
    lua_pushlstring(L,str,SBYTES(obj));
}
int pub ret(/*string*/) unibyte_lstring_to_string(lua_State *L){
    check_nargs(L,1);
    check_isobject(L,1);
    Lisp_Object obj = userdata_to_obj(L,1);
    if (!STRINGP(obj) || STRING_MULTIBYTE(obj))
        luaL_error(L,"Expected unibyte string");
    tcall(L,t_unibyte_lstring_to_string);
    return 1;
}

void t_cons_to_table(lua_State *L){
    Lisp_Object obj = userdata_to_obj(L,1);
    eassert(CONSP(obj));
    Lisp_Object car=XCAR(obj);
    Lisp_Object cdr=XCDR(obj);
    lua_newtable(L);
    // (-1)tbl
    push_obj(L,car);
    // (-2)tbl,(-1)car
    lua_rawseti(L,-2,1);
    // (-1)tbl
    push_obj(L,cdr);
    // (-2)tbl,(-1)cdr
    lua_rawseti(L,-2,2);
    // (-1)tbl
}
int pub ret(/*[nelisp.obj,nelisp.obj]*/) cons_to_table(lua_State *L){
    check_nargs(L,1);
    check_isobject(L,1);
    Lisp_Object obj = userdata_to_obj(L,1);
    if (!CONSP(obj))
        luaL_error(L,"Expected cons");
    tcall(L,t_cons_to_table);
    return 1;
}

void t_vector_to_table(lua_State *L){
    Lisp_Object obj = userdata_to_obj(L,1);
    eassert(VECTORP(obj));
    lua_newtable(L);
    // (-1)tbl
    ptrdiff_t len = ASIZE(obj);
    for (ptrdiff_t i=0; i<len; i++){
        // (-1)tbl
        push_obj(L,XVECTOR(obj)->contents[i]);
        // (-2)tbl,(-1)obj
        lua_rawseti(L,-2,i+1);
        // (-1)tbl
    };
    // (-1)tbl
}
int pub ret(/*nelisp.obj[]*/) vector_to_table(lua_State *L){
    check_nargs(L,1);
    check_isobject(L,1);
    Lisp_Object obj = userdata_to_obj(L,1);
    if (!VECTORP(obj))
        luaL_error(L,"Expected vector");
    tcall(L,t_vector_to_table);
    return 1;
}

void t__get_symbol(lua_State *L){
    for (unsigned long i = 0; i < ARRAYELTS (lispsym); i++) {
        size_t len;
        lua_tolstring(L,-1,&len);
        if (memcmp(lua_tolstring(L,-1,&len),defsym_name[i],len+1)==0){
            push_obj(L,builtin_lisp_symbol(i));
            return;
        }
    }
    lua_pushnil(L);
}
int pub ret(/*nelisp.obj*/) _get_symbol(lua_State *L){
    check_nargs(L,1);
    check_isstring(L,1);
    tcall(L,t__get_symbol);
    if (lua_isnil(L,-1))
        luaL_error(L,"Symbol '%s' not found",lua_tostring(L,1));
    return 1;
}


void t_eval(lua_State *L){
    specpdl_ref count = SPECPDL_INDEX ();
    size_t len;
    Lisp_Object lex_bound;
    const char* str = lua_tolstring(L,-1,&len);

    specbind (Qlexical_binding, Qnil);

    Lisp_Object readcharfun = make_unibyte_string(str,len);

    read_from_string_index = 0;
    read_from_string_index_byte = string_char_to_byte (readcharfun, 0);
    read_from_string_limit = len;

    if (lisp_file_lexical_cookie (readcharfun) == Cookie_Lex)
        Fset (Qlexical_binding, Qt);

    Lisp_Object ret=NULL;
    while (1){
        int c = READCHAR;
        if (c == ';')
        {
            while ((c = READCHAR) != '\n' && c != -1);
            continue;
        }
        if (c < 0)
        {
            break;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r'
            || c == NO_BREAK_SPACE)
            continue;
        UNREAD (c);
        lex_bound = find_symbol_value (Qlexical_binding);
        specbind (Qinternal_interpreter_environment,
                  (NILP (lex_bound) || BASE_EQ (lex_bound, Qunbound)
                  ? Qnil : list1 (Qt)));
        Lisp_Object val = read0 (readcharfun, false);
        ret = eval_sub (val);
    }
    if (ret)
        push_obj(L,ret);
    else
        lua_pushnil(L);
    unbind_to (count, Qnil);
}
int pub ret(/*nelisp.obj|nil*/) eval(lua_State *L){
    check_nargs(L,1);
    check_isstring(L,1);
    tcall(L,t_eval);
    return 1;
}

void t_collectgarbage(lua_State *L){
    UNUSED(L);
    garbage_collect();
}
int pub ret() collectgarbage(lua_State *L){
    check_nargs(L,0);
    tcall(L,t_collectgarbage);
    return 0;
}

static bool is_directory(const char* dir){
    struct stat st;
    if (stat(dir, &st)!=0) return false;
    return S_ISDIR(st.st_mode);
}
bool inited=false;
int t_init(void* args){
    (void)args;
    stack_top=&args;
    Frecursive_edit();
    eassume(false);
}
int pub ret() init(lua_State *L){
    global_lua_state = L;
    check_nargs(L,1);
    check_istable_with_keyvalue(L,1,(struct kv_t[]){
        {"runtime_path",kv_mask_string},
        {NULL,0}});
    if (inited)
        return 0;
    inited=true;

    lua_getfield(L,-1,"runtime_path");
    size_t len;
    const char* dir=lua_tolstring(L,-1,&len);
    if (memchr(dir,'\0',len)!=NULL)
        luaL_error(L,"runtime_path contains a null byte");
    if (!is_directory(dir))
        luaL_error(L,"runtime_path is not a directory");
    lua_pushliteral(L,"/lisp");
    lua_concat(L,2);
    size_t len_lisp_dir;
    const char* lisp_dir=lua_tolstring(L,-1,&len_lisp_dir);
    if (!is_directory(lisp_dir))
        luaL_error(L,"runtime_path directory doesn't have subdirectory `lisp/`");


    init_alloc_once();
    init_eval_once();
    init_obarray_once();

    Vload_path=list1(make_unibyte_string(lisp_dir,len_lisp_dir));

    syms_of_lread();
    syms_of_data();
    syms_of_alloc();
    syms_of_eval();
    syms_of_fns();
    syms_of_keyboard();
    syms_of_editfns();
    syms_of_emacs();
    syms_of_fileio();
    syms_of_coding();

    init_keyboard();
    init_eval();

    bool err=false;
    if (mtx_init(&main_mutex,mtx_plain)!=thrd_success)
        err=true;
    if (mtx_init(&thread_mutex,mtx_plain)!=thrd_success)
        err=true;
    if (cnd_init(&main_cond)!=thrd_success)
        err=true;
    if (cnd_init(&thread_cond)!=thrd_success)
        err=true;
    if (err)
    {
        unrecoverable_error=true;
        luaL_error(global_lua_state,"Failed to init thread");
    }

    mtx_lock(&main_mutex);
    thrd_create(&main_thread,t_init,NULL);
    cnd_wait(&main_cond,&main_mutex);

    return 0;
}
