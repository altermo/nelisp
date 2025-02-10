#include <stdlib.h>

#include "lisp.h"

enum mem_type
{
    MEM_TYPE_NON_LISP,
    MEM_TYPE_CONS,
    MEM_TYPE_STRING,
    MEM_TYPE_SYMBOL,
    MEM_TYPE_FLOAT,
    MEM_TYPE_VECTORLIKE,
    MEM_TYPE_VECTOR_BLOCK,
    MEM_TYPE_SPARE
};

typedef uintptr_t object_ct;
typedef uintptr_t byte_ct;

static struct gcstat
{
    object_ct total_conses, total_free_conses;
    object_ct total_symbols, total_free_symbols;
    object_ct total_strings, total_free_strings;
    byte_ct total_string_bytes;
    object_ct total_vectors, total_vector_slots, total_free_vector_slots;
    object_ct total_floats, total_free_floats;
    object_ct total_intervals, total_free_intervals;
    object_ct total_buffers;
} gcstat;

/* --- memory full handling -- */

void
memory_full (size_t nbytes) {
    UNUSED(nbytes);
    TODO_NELISP_LATER
}

/* --- C stack management -- */

static void *min_heap_address, *max_heap_address;

struct mem_node
{
    struct mem_node *left, *right;
    struct mem_node *parent;
    void *start, *end;
    enum {MEM_BLACK, MEM_RED} color;
    enum mem_type type;
};
static struct mem_node *mem_root;

static struct mem_node mem_z;
#define MEM_NIL &mem_z

static void
mem_init (void)
{
    mem_z.left = mem_z.right = MEM_NIL;
    mem_z.parent = NULL;
    mem_z.color = MEM_BLACK;
    mem_z.start = mem_z.end = NULL;
    mem_root = MEM_NIL;
}

static void
mem_rotate_left (struct mem_node *x) {
    struct mem_node *y;

    y = x->right;
    x->right = y->left;
    if (y->left != MEM_NIL)
        y->left->parent = x;

    if (y != MEM_NIL)
        y->parent = x->parent;

    if (x->parent)
    {
        if (x == x->parent->left)
            x->parent->left = y;
        else
            x->parent->right = y;
    }
    else {
        mem_root = y;
    }

    y->left = x;
    if (x != MEM_NIL)
        x->parent = y;
}

static void
mem_rotate_right (struct mem_node *x)
{
    struct mem_node *y = x->left;

    x->left = y->right;
    if (y->right != MEM_NIL)
        y->right->parent = x;

    if (y != MEM_NIL)
        y->parent = x->parent;
    if (x->parent)
    {
        if (x == x->parent->right)
            x->parent->right = y;
        else
            x->parent->left = y;
    }
    else {
        mem_root = y;
    }

    y->right = x;
    if (x != MEM_NIL)
        x->parent = y;
}

static void
mem_insert_fixup (struct mem_node *x)
{
    while (x != mem_root && x->parent->color == MEM_RED)
    {
        if (x->parent == x->parent->parent->left)
        {
            struct mem_node *y = x->parent->parent->right;

            if (y->color == MEM_RED)
            {
                x->parent->color = MEM_BLACK;
                y->color = MEM_BLACK;
                x->parent->parent->color = MEM_RED;
                x = x->parent->parent;
            }
            else
        {
                if (x == x->parent->right)
                {
                    x = x->parent;
                    mem_rotate_left (x);
                }

                x->parent->color = MEM_BLACK;
                x->parent->parent->color = MEM_RED;
                mem_rotate_right (x->parent->parent);
            }
        }
        else
    {
            struct mem_node *y = x->parent->parent->left;

            if (y->color == MEM_RED)
            {
                x->parent->color = MEM_BLACK;
                y->color = MEM_BLACK;
                x->parent->parent->color = MEM_RED;
                x = x->parent->parent;
            }
            else
        {
                if (x == x->parent->left)
                {
                    x = x->parent;
                    mem_rotate_right (x);
                }

                x->parent->color = MEM_BLACK;
                x->parent->parent->color = MEM_RED;
                mem_rotate_left (x->parent->parent);
            }
        }
    }

    mem_root->color = MEM_BLACK;
}

static struct mem_node *
mem_insert (void *start, void *end, enum mem_type type){
# if TODO_NELISP_LATER_ELSE
    if (mem_root == NULL) {
        mem_init();
    }
#endif
    struct mem_node *c, *parent, *x;
    if (min_heap_address == NULL || start < min_heap_address)
        min_heap_address = start;
    if (max_heap_address == NULL || end > max_heap_address)
        max_heap_address = end;

    c = mem_root;
    parent = NULL;

    while (c != MEM_NIL)
    {
        parent = c;
        c = start < c->start ? c->left : c->right;
    }
    #if TODO_NELISP_LATER_ELSE
    x = malloc (sizeof *x);
#endif
    x->start = start;
    x->end = end;
    x->type = type;
    x->parent = parent;
    x->left = x->right = MEM_NIL;
    x->color = MEM_RED;
    if (parent)
    {
        if (start < parent->start)
            parent->left = x;
        else
            parent->right = x;
    }
    else {
        mem_root = x;
    }
    mem_insert_fixup (x);
    return x;
}

static struct mem_node *
mem_find (void *start)
{
    struct mem_node *p;

    if (start < min_heap_address || start > max_heap_address)
        return MEM_NIL;

    /* Make the search always successful to speed up the loop below.  */
    mem_z.start = start;
    mem_z.end = (char *) start + 1;

    p = mem_root;
    while (start < p->start || start >= p->end)
        p = start < p->start ? p->left : p->right;
    return p;
}

static void
mem_delete_fixup (struct mem_node *x)
{
    while (x != mem_root && x->color == MEM_BLACK)
    {
        if (x == x->parent->left)
        {
            struct mem_node *w = x->parent->right;

            if (w->color == MEM_RED)
            {
                w->color = MEM_BLACK;
                x->parent->color = MEM_RED;
                mem_rotate_left (x->parent);
                w = x->parent->right;
            }

            if (w->left->color == MEM_BLACK && w->right->color == MEM_BLACK)
            {
                w->color = MEM_RED;
                x = x->parent;
            }
            else
        {
                if (w->right->color == MEM_BLACK)
                {
                    w->left->color = MEM_BLACK;
                    w->color = MEM_RED;
                    mem_rotate_right (w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = MEM_BLACK;
                w->right->color = MEM_BLACK;
                mem_rotate_left (x->parent);
                x = mem_root;
            }
        }
        else
    {
            struct mem_node *w = x->parent->left;

            if (w->color == MEM_RED)
            {
                w->color = MEM_BLACK;
                x->parent->color = MEM_RED;
                mem_rotate_right (x->parent);
                w = x->parent->left;
            }

            if (w->right->color == MEM_BLACK && w->left->color == MEM_BLACK)
            {
                w->color = MEM_RED;
                x = x->parent;
            }
            else
        {
                if (w->left->color == MEM_BLACK)
                {
                    w->right->color = MEM_BLACK;
                    w->color = MEM_RED;
                    mem_rotate_left (w);
                    w = x->parent->left;
                }

                w->color = x->parent->color;
                x->parent->color = MEM_BLACK;
                w->left->color = MEM_BLACK;
                mem_rotate_right (x->parent);
                x = mem_root;
            }
        }
    }

    x->color = MEM_BLACK;
}

static void
mem_delete (struct mem_node *z)
{
    struct mem_node *x, *y;

    if (!z || z == MEM_NIL)
        return;

    if (z->left == MEM_NIL || z->right == MEM_NIL)
        y = z;
    else
    {
        y = z->right;
        while (y->left != MEM_NIL)
            y = y->left;
    }

    if (y->left != MEM_NIL)
        x = y->left;
    else
        x = y->right;

    x->parent = y->parent;
    if (y->parent)
    {
        if (y == y->parent->left)
            y->parent->left = x;
        else
            y->parent->right = x;
    }
    else {
        mem_root = x;
    }

    if (y != z) {
        z->start = y->start;
        z->end = y->end;
        z->type = y->type;
    }

    if (y->color == MEM_BLACK)
        mem_delete_fixup (x);

    #if TODO_NELISP_LATER_ELSE
    free (y);
#endif
}

/* --- malloc -- */

#define BLOCK_ALIGN (1 << 10)
#define BLOCK_PADDING 0
#define BLOCK_BYTES \
(BLOCK_ALIGN - sizeof (struct ablocks *) - BLOCK_PADDING)
#define ABLOCKS_SIZE 16
struct ablock
{
    union
    {
        char payload[BLOCK_BYTES];
        struct ablock *next_free;
    } x;

    struct ablocks *abase;
};
struct ablocks
{
    struct ablock blocks[ABLOCKS_SIZE];
};
#define ABLOCKS_BYTES (sizeof (struct ablocks) - BLOCK_PADDING)
#define ABLOCK_ABASE(block) \
(((uintptr_t) (block)->abase) <= (1 + 2 * ABLOCKS_SIZE)	\
    ? (struct ablocks *) (block)					\
    : (block)->abase)
#define ABLOCKS_BUSY(a_base) ((a_base)->blocks[0].abase)
#define ABLOCKS_BASE(abase) (abase)

static struct ablock *free_ablock;

static void *
lisp_align_malloc (size_t nbytes, enum mem_type type) {
    void *base, *val;
    struct ablocks *abase;

    eassert (nbytes <= BLOCK_BYTES);

    if (!free_ablock) {
        int i;
        bool aligned;

        abase = base = aligned_alloc (BLOCK_ALIGN, ABLOCKS_BYTES);

        if (base == 0) {
            memory_full (ABLOCKS_BYTES);
        }

        aligned = (base == abase);
        if (!aligned)
            ((void **) abase)[-1] = base;

        for (i = 0; i < (aligned ? ABLOCKS_SIZE : ABLOCKS_SIZE - 1); i++)
        {
            abase->blocks[i].abase = abase;
            abase->blocks[i].x.next_free = free_ablock;
            free_ablock = &abase->blocks[i];
        }
        intptr_t ialigned = aligned;
        ABLOCKS_BUSY (abase) = (struct ablocks *) ialigned;

        eassert ((uintptr_t) abase % BLOCK_ALIGN == 0);
        eassert (ABLOCK_ABASE (&abase->blocks[3]) == abase); /* 3 is arbitrary */
        eassert (ABLOCK_ABASE (&abase->blocks[0]) == abase);
        eassert (ABLOCKS_BASE (abase) == base);
        eassert ((intptr_t) ABLOCKS_BUSY (abase) == aligned);
    }

    abase = ABLOCK_ABASE (free_ablock);
    ABLOCKS_BUSY (abase)
        = (struct ablocks *) (2 + (intptr_t) ABLOCKS_BUSY (abase));
    val = free_ablock;
    free_ablock = free_ablock->x.next_free;

    if (type != MEM_TYPE_NON_LISP)
        mem_insert (val, (char *) val + nbytes, type);

    eassert (0 == ((uintptr_t) val) % BLOCK_ALIGN);
    return val;
}

static void
lisp_align_free (void *block) {
    struct ablock *ablock = block;
    struct ablocks *abase = ABLOCK_ABASE (ablock);
    mem_delete (mem_find (block));
    ablock->x.next_free = free_ablock;
    free_ablock = ablock;
    intptr_t busy = (intptr_t) ABLOCKS_BUSY (abase) - 2;
    eassume (0 <= busy && busy <= 2 * ABLOCKS_SIZE - 1);
    ABLOCKS_BUSY (abase) = (struct ablocks *) busy;
    if (busy < 2)
    {
        int i = 0;
        bool aligned = busy;
        struct ablock **tem = &free_ablock;
        struct ablock *atop = &abase->blocks[aligned ? ABLOCKS_SIZE : ABLOCKS_SIZE - 1];

        while (*tem)
        {
            if (*tem >= (struct ablock *) abase && *tem < atop)
            {
                i++;
                *tem = (*tem)->x.next_free;
            }
            else
            tem = &(*tem)->x.next_free;
        }
        eassert ((aligned & 1) == aligned);
        eassert (i == (aligned ? ABLOCKS_SIZE : ABLOCKS_SIZE - 1));
        free (ABLOCKS_BASE (abase));
    }
}

void *
xpalloc (void *pa, ptrdiff_t *nitems, ptrdiff_t nitems_incr_min,
         ptrdiff_t nitems_max, ptrdiff_t item_size) {
    UNUSED(nitems_incr_min);
    UNUSED(nitems_max);
    TODO_NELISP_LATER
    pa=realloc(pa,(*nitems+1)*item_size*2);
    *nitems=(*nitems+1)*2;
    return pa;
}

/* --- float allocation -- */

#define XFLOAT_MARKED_P(fptr) \
GETMARKBIT (FLOAT_BLOCK (fptr), FLOAT_INDEX ((fptr)))

#define XFLOAT_MARK(fptr) \
SETMARKBIT (FLOAT_BLOCK (fptr), FLOAT_INDEX ((fptr)))

#define XFLOAT_UNMARK(fptr) \
UNSETMARKBIT (FLOAT_BLOCK (fptr), FLOAT_INDEX ((fptr)))

#define FLOAT_BLOCK_SIZE					\
(((BLOCK_BYTES - sizeof (struct float_block *)		\
    - (sizeof (struct Lisp_Float) - sizeof (bits_word))) * CHAR_BIT) \
    / (sizeof (struct Lisp_Float) * CHAR_BIT + 1))

#define GETMARKBIT(block,n)				\
(((block)->gcmarkbits[(n) / BITS_PER_BITS_WORD]	\
    >> ((n) % BITS_PER_BITS_WORD))			\
    & 1)

#define SETMARKBIT(block,n)				\
((block)->gcmarkbits[(n) / BITS_PER_BITS_WORD]	\
    |= (bits_word) 1 << ((n) % BITS_PER_BITS_WORD))

#define UNSETMARKBIT(block,n)				\
((block)->gcmarkbits[(n) / BITS_PER_BITS_WORD]	\
    &= ~((bits_word) 1 << ((n) % BITS_PER_BITS_WORD)))

#define FLOAT_BLOCK(fptr) \
(eassert (!pdumper_object_p (fptr)),                                  \
    ((struct float_block *) (((uintptr_t) (fptr)) & ~(BLOCK_ALIGN - 1))))

#define FLOAT_INDEX(fptr) \
((((uintptr_t) (fptr)) & (BLOCK_ALIGN - 1)) / sizeof (struct Lisp_Float))

struct float_block
{
    /* Place `floats' at the beginning, to ease up FLOAT_INDEX's job.  */
    struct Lisp_Float floats[FLOAT_BLOCK_SIZE];
    bits_word gcmarkbits[1 + FLOAT_BLOCK_SIZE / BITS_PER_BITS_WORD];
    struct float_block *next;
};

static struct float_block *float_block;

static int float_block_index = FLOAT_BLOCK_SIZE;

static struct Lisp_Float *float_free_list;

static void
XFLOAT_INIT (Lisp_Object f, double n)
{
    XFLOAT (f)->u.data = n;
}

Lisp_Object
make_float (double float_value)
{
    register Lisp_Object val;
    if (float_free_list) {
        XSETFLOAT (val, float_free_list);
        float_free_list = float_free_list->u.chain;
    } else {
        if (float_block_index == FLOAT_BLOCK_SIZE) {
            struct float_block *new = lisp_align_malloc (sizeof *new, MEM_TYPE_FLOAT);
            new->next = float_block;
            memset (new->gcmarkbits, 0, sizeof new->gcmarkbits);
            float_block = new;
            float_block_index = 0;
        }
        XSETFLOAT (val, &float_block->floats[float_block_index]);
        float_block_index++;
    }
    XFLOAT_INIT (val, float_value);
#if TODO_NELISP_LATER_AND
    eassert (!XFLOAT_MARKED_P (XFLOAT (val)));
    tally_consing (sizeof (struct Lisp_Float));
    floats_consed++;
#endif
    return val;
}

/* --- garbage collector -- */

struct mark_entry
{
    ptrdiff_t n;
    union {
        Lisp_Object value;
        Lisp_Object *values;
    } u;
};
struct mark_stack
{
    struct mark_entry *stack;
    ptrdiff_t size;
    ptrdiff_t sp;
};
static struct mark_stack mark_stk = {NULL, 0, 0};

static inline bool
mark_stack_empty_p (void)
{
    return mark_stk.sp <= 0;
}

NO_INLINE static void
grow_mark_stack (void)
{
    struct mark_stack *ms = &mark_stk;
    eassert (ms->sp == ms->size);
    ptrdiff_t min_incr = ms->sp == 0 ? 8192 : 1;
    ms->stack = xpalloc (ms->stack, &ms->size, min_incr, -1, sizeof *ms->stack);
    eassert (ms->sp < ms->size);
}

static inline void
mark_stack_push_value (Lisp_Object value)
{
    if (mark_stk.sp >= mark_stk.size)
        grow_mark_stack ();
    mark_stk.stack[mark_stk.sp++] = (struct mark_entry){.n = 0, .u.value = value};
}

static inline Lisp_Object
mark_stack_pop (void)
{
    eassume (!mark_stack_empty_p ());
    struct mark_entry *e = &mark_stk.stack[mark_stk.sp - 1];
    if (e->n == 0)		/* single value */
    {
        --mark_stk.sp;
        return e->u.value;
    }
    /* Array of values: pop them left to right, which seems to be slightly
     faster than right to left.  */
    e->n--;
    if (e->n == 0)
        --mark_stk.sp;		/* last value consumed */
    return (++e->u.values)[-1];
}

static void
process_mark_stack (ptrdiff_t base_sp) {
    TODO_NELISP_LATER
    eassume (mark_stk.sp >= base_sp && base_sp >= 0);

    while (mark_stk.sp > base_sp) {
        Lisp_Object obj = mark_stack_pop ();
        switch (XTYPE (obj)) {
            case Lisp_String:
                TODO
                break;
            case Lisp_Vectorlike:
                TODO
                break;
            case Lisp_Symbol:
                TODO
                break;
            case Lisp_Cons:
                TODO
                break;
            case Lisp_Float:
                if (pdumper_object_p(XFLOAT(obj)))
                    TODO
                else if (!XFLOAT_MARKED_P(XFLOAT(obj)))
                    XFLOAT_MARK(XFLOAT(obj));
                break;
            case_Lisp_Int:
                break;
            default:
                eassert(false);
        }
    }
}

void
mark_object (Lisp_Object obj)
{
    ptrdiff_t sp = mark_stk.sp;
    mark_stack_push_value (obj);
    process_mark_stack (sp);
}

static void
mark_lua (void) {
    lua_State *L = global_lua_state;
    if (!lua_checkstack(L,lua_gettop(L)+10))
        luaL_error(L,"Lua stack overflow");
    lua_getfield(L,LUA_ENVIRONINDEX,"memtbl");
    eassert(lua_istable(L,-1));
    // (-1)memtbl
    lua_pushnil(L);
    // (-2)memtbl, (-1)nil
    while (lua_next(L,-2)!=0){
        eassert(lua_isuserdata(L,-1) && !lua_islightuserdata(L,-1));
        eassert(lua_isstring(L,-2));
        // (-3)memtbl, (-2)key, (-1)value
        Lisp_Object obj=*(Lisp_Object*)lua_touserdata(L,-1);
        eassert(!FIXNUMP(obj));
        mark_object(obj);
        lua_pop(L,1);
    };
}

NO_INLINE static void
sweep_floats (void) {
    struct float_block **fprev = &float_block;
    int lim = float_block_index;
    object_ct num_free = 0, num_used = 0;

    float_free_list = 0;

    for (struct float_block *fblk; (fblk = *fprev); ) {
        int this_free = 0;
        for (int i = 0; i < lim; i++)
        {
            struct Lisp_Float *afloat = &fblk->floats[i];
            if (!XFLOAT_MARKED_P (afloat)) {
                this_free++;
                fblk->floats[i].u.chain = float_free_list;
                float_free_list = &fblk->floats[i];
            }
            else {
                num_used++;
                XFLOAT_UNMARK (afloat);
            }
        }
        lim = FLOAT_BLOCK_SIZE;
        if (this_free == FLOAT_BLOCK_SIZE && num_free > FLOAT_BLOCK_SIZE)
        {
            *fprev = fblk->next;
            float_free_list = fblk->floats[0].u.chain;
            lisp_align_free (fblk);
        }
        else {
            num_free += this_free;
            fprev = &fblk->next;
        }
    }
    gcstat.total_floats = num_used;
    gcstat.total_free_floats = num_free;
}

static void
gc_sweep(void){
    TODO_NELISP_LATER
    sweep_floats ();
}

void
garbage_collect_ (void) {
    TODO_NELISP_LATER
    eassert(mark_stack_empty_p ());

    mark_lua();

    eassert (mark_stack_empty_p ());

    gc_sweep ();
}
