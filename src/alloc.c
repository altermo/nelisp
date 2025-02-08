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

/* --- memory full handling -- */

void
memory_full (size_t nbytes) {
#ifdef NELISP_LATER
    #error "TODO"
#endif
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
#ifdef NELISP_LATER
    #error "TODO"
#else
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
#ifdef NELISP_LATER
    #error "TODO"
#else
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

/* --- float allocation -- */

#define FLOAT_BLOCK_SIZE					\
(((BLOCK_BYTES - sizeof (struct float_block *)		\
    - (sizeof (struct Lisp_Float) - sizeof (bits_word))) * CHAR_BIT) \
    / (sizeof (struct Lisp_Float) * CHAR_BIT + 1))

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
#ifdef NELISP_LATER
    eassert (!XFLOAT_MARKED_P (XFLOAT (val)));
    tally_consing (sizeof (struct Lisp_Float));
    floats_consed++;
#endif
    return val;
}
