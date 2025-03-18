#include <stdlib.h>

#include "lisp.h"
#include "character.h"

#define USE_ALIGNED_ALLOC 1
#define MALLOC_0_IS_NONNULL 1
Lisp_Object empty_unibyte_string, empty_multibyte_string;

struct emacs_globals globals;

union emacs_align_type
{
  // struct frame frame;
  // struct Lisp_Bignum Lisp_Bignum;
  struct Lisp_Bool_Vector Lisp_Bool_Vector;
  // struct Lisp_Char_Table Lisp_Char_Table;
  // struct Lisp_CondVar Lisp_CondVar;
  // struct Lisp_Finalizer Lisp_Finalizer;
  struct Lisp_Float Lisp_Float;
  // struct Lisp_Hash_Table Lisp_Hash_Table;
  // struct Lisp_Marker Lisp_Marker;
  // struct Lisp_Misc_Ptr Lisp_Misc_Ptr;
  // struct Lisp_Mutex Lisp_Mutex;
  // struct Lisp_Overlay Lisp_Overlay;
  // struct Lisp_Sub_Char_Table Lisp_Sub_Char_Table;
  struct Lisp_Subr Lisp_Subr;
  // struct Lisp_Sqlite Lisp_Sqlite;
  // struct Lisp_User_Ptr Lisp_User_Ptr;
  struct Lisp_Vector Lisp_Vector;
  // struct terminal terminal;
  // struct thread_state thread_state;
  // struct window window;
};
#define MALLOC_SIZE_NEAR(n) \
  (ROUNDUP (max (n, sizeof (size_t)), MALLOC_ALIGNMENT) - sizeof (size_t))
enum
{
  MALLOC_ALIGNMENT = max (2 * sizeof (size_t), alignof (long double))
};
#define XMARK_STRING(S) ((S)->u.s.size |= ARRAY_MARK_FLAG)
#define XUNMARK_STRING(S) ((S)->u.s.size &= ~ARRAY_MARK_FLAG)
#define XSTRING_MARKED_P(S) (((S)->u.s.size & ARRAY_MARK_FLAG) != 0)
#define XMARK_VECTOR(V) ((V)->header.size |= ARRAY_MARK_FLAG)
#define XUNMARK_VECTOR(V) ((V)->header.size &= ~ARRAY_MARK_FLAG)
#define XVECTOR_MARKED_P(V) (((V)->header.size & ARRAY_MARK_FLAG) != 0)
typedef uintptr_t byte_ct;
typedef intptr_t object_ct;
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
  byte_ct total_hash_table_bytes;
} gcstat;
static ptrdiff_t hash_table_allocated_bytes = 0;

#define SYSTEM_PURESIZE_EXTRA 0
#define SITELOAD_PURESIZE_EXTRA 0
#define BASE_PURESIZE \
  (3400000 + SYSTEM_PURESIZE_EXTRA + SITELOAD_PURESIZE_EXTRA)
#define PURESIZE (BASE_PURESIZE * PURESIZE_RATIO * PURESIZE_CHECKING_RATIO)
#define PURESIZE_RATIO 10 / 6
#define PURESIZE_CHECKING_RATIO 1
EMACS_INT pure[(PURESIZE + sizeof (EMACS_INT) - 1) / sizeof (EMACS_INT)] = {
  1,
};
#define PUREBEG (char *) pure
static char *purebeg;
static ptrdiff_t pure_size;
static ptrdiff_t pure_bytes_used_before_overflow;
static ptrdiff_t pure_bytes_used_lisp;
static ptrdiff_t pure_bytes_used_non_lisp;
#if TODO_NELISP_LATER_ELSE
static ptrdiff_t pure_bytes_used;
#endif

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
struct mem_node
{
  struct mem_node *left, *right;
  struct mem_node *parent;
  void *start, *end;
  enum
  {
    MEM_BLACK,
    MEM_RED
  } color;
  enum mem_type type;
};
static struct mem_node *mem_root;
static void *min_heap_address, *max_heap_address;
static struct mem_node mem_z;
#define MEM_NIL &mem_z
static struct mem_node *mem_insert (void *, void *, enum mem_type);
static void mem_insert_fixup (struct mem_node *);
static void mem_rotate_left (struct mem_node *);
static void mem_rotate_right (struct mem_node *);
static void mem_delete (struct mem_node *);
static void mem_delete_fixup (struct mem_node *);
static struct mem_node *mem_find (void *);
void mark_object (Lisp_Object obj);
static bool vector_marked_p (struct Lisp_Vector const *);
void staticpro (Lisp_Object const *varaddress);
enum
{
  NSTATICS = 2048
};
Lisp_Object const *staticvec[NSTATICS];
int staticidx;
static void *pure_alloc (size_t, int);
static void *
pointer_align (void *ptr, int alignment)
{
  return (void *) ROUNDUP ((uintptr_t) ptr, alignment);
}
static void *
XPNTR (Lisp_Object a)
{
  return (BARE_SYMBOL_P (a)
            ? (char *) lispsym + (XLI (a) - LISP_WORD_TAG (Lisp_Symbol))
            : (char *) XLP (a) - (XLI (a) & ~VALMASK));
}
static void
XFLOAT_INIT (Lisp_Object f, double n)
{
  XFLOAT (f)->u.data = n;
}
static void
tally_consing (ptrdiff_t nbytes)
{
  UNUSED (nbytes);
  TODO_NELISP_LATER;
}

/* --- memory full handling -- */
void
memory_full (size_t nbytes)
{
  UNUSED (nbytes);
  TODO;
}

/* --- malloc -- */

#define COMMON_MULTIPLE(a, b) \
  ((a) % (b) == 0 ? (a) : (b) % (a) == 0 ? (b) : (a) * (b))
enum
{
  LISP_ALIGNMENT = alignof (union
                            {
                              union emacs_align_type x;
                              GCALIGNED_UNION_MEMBER
                            })
};
enum
{
  MALLOC_IS_LISP_ALIGNED = alignof (max_align_t) % LISP_ALIGNMENT == 0
};
#define MALLOC_BLOCK_INPUT ((void) 0)
#define MALLOC_UNBLOCK_INPUT ((void) 0)
static void *lmalloc (size_t, bool);
static void *lrealloc (void *, size_t);
void *
xmalloc (size_t size)
{
  void *val;

  MALLOC_BLOCK_INPUT;
  val = lmalloc (size, false);
  MALLOC_UNBLOCK_INPUT;

  if (!val)
    memory_full (size);
  return val;
}
void *
xzalloc (size_t size)
{
  void *val;

  MALLOC_BLOCK_INPUT;
  val = lmalloc (size, true);
  MALLOC_UNBLOCK_INPUT;

  if (!val)
    memory_full (size);
  return val;
}
void *
xrealloc (void *block, size_t size)
{
  void *val;

  MALLOC_BLOCK_INPUT;
  if (!block)
    val = lmalloc (size, false);
  else
    val = lrealloc (block, size);
  MALLOC_UNBLOCK_INPUT;

  if (!val)
    memory_full (size);
  return val;
}
void
xfree (void *block)
{
  if (!block)
    return;
  if (pdumper_object_p (block))
    return;
  MALLOC_BLOCK_INPUT;
  free (block);
  MALLOC_UNBLOCK_INPUT;
}

void *
xpalloc (void *pa, ptrdiff_t *nitems, ptrdiff_t nitems_incr_min,
         ptrdiff_t nitems_max, ptrdiff_t item_size)
{
  ptrdiff_t n0 = *nitems;
  eassume (0 < item_size && 0 < nitems_incr_min && 0 <= n0 && -1 <= nitems_max);

  enum
  {
    DEFAULT_MXFAST = 64 * sizeof (size_t) / 4
  };

  ptrdiff_t n, nbytes;
  if (ckd_add (&n, n0, n0 >> 1))
    n = PTRDIFF_MAX;
  if (0 <= nitems_max && nitems_max < n)
    n = nitems_max;

  ptrdiff_t adjusted_nbytes
    = ((ckd_mul (&nbytes, n, item_size) || SIZE_MAX < (size_t) nbytes)
         ? min (PTRDIFF_MAX, SIZE_MAX)
       : nbytes < DEFAULT_MXFAST ? DEFAULT_MXFAST
                                 : 0);
  if (adjusted_nbytes)
    {
      n = adjusted_nbytes / item_size;
      nbytes = adjusted_nbytes - adjusted_nbytes % item_size;
    }

  if (!pa)
    *nitems = 0;
  if (n - n0 < nitems_incr_min
      && (ckd_add (&n, n0, nitems_incr_min)
          || (0 <= nitems_max && nitems_max < n)
          || ckd_mul (&nbytes, n, item_size)))
    memory_full (SIZE_MAX);
  pa = xrealloc (pa, nbytes);
  *nitems = n;
  return pa;
}

static void *
lisp_malloc (size_t nbytes, bool clearit, enum mem_type type)
{
  register void *val;

  MALLOC_BLOCK_INPUT;

  val = lmalloc (nbytes, clearit);

  if (val && type != MEM_TYPE_NON_LISP)
    mem_insert (val, (char *) val + nbytes, type);

  MALLOC_UNBLOCK_INPUT;
  if (!val)
    memory_full (nbytes);
  return val;
}
static void
lisp_free (void *block)
{
  if (pdumper_object_p (block))
    return;

  MALLOC_BLOCK_INPUT;
  struct mem_node *m = mem_find (block);
  free (block);
  mem_delete (m);
  MALLOC_UNBLOCK_INPUT;
}

/* --- align malloc -- */

#define BLOCK_ALIGN (1 << 15)
#define BLOCK_PADDING 0
#define BLOCK_BYTES (BLOCK_ALIGN - sizeof (struct ablocks *) - BLOCK_PADDING)
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
#define ABLOCK_ABASE(block)                               \
  (((uintptr_t) (block)->abase) <= (1 + 2 * ABLOCKS_SIZE) \
     ? (struct ablocks *) (block)                         \
     : (block)->abase)
#define ABLOCKS_BUSY(a_base) ((a_base)->blocks[0].abase)
#define ABLOCKS_BASE(abase) (abase)
#define ASAN_POISON_ABLOCK(b) ((void) 0)
#define ASAN_UNPOISON_ABLOCK(b) ((void) 0)
static struct ablock *free_ablock;
static void *
lisp_align_malloc (size_t nbytes, enum mem_type type)
{
  void *base, *val;
  struct ablocks *abase;

  eassert (nbytes <= BLOCK_BYTES);

  MALLOC_BLOCK_INPUT;

  if (!free_ablock)
    {
      int i;
      bool aligned;

      abase = base = aligned_alloc (BLOCK_ALIGN, ABLOCKS_BYTES);

      if (base == 0)
        {
          MALLOC_UNBLOCK_INPUT;
          memory_full (ABLOCKS_BYTES);
        }

      aligned = (base == abase);
      if (!aligned)
        ((void **) abase)[-1] = base;

      for (i = 0; i < (aligned ? ABLOCKS_SIZE : ABLOCKS_SIZE - 1); i++)
        {
          abase->blocks[i].abase = abase;
          abase->blocks[i].x.next_free = free_ablock;
          ASAN_POISON_ABLOCK (&abase->blocks[i]);
          free_ablock = &abase->blocks[i];
        }
      intptr_t ialigned = aligned;
      ABLOCKS_BUSY (abase) = (struct ablocks *) ialigned;

      eassert ((uintptr_t) abase % BLOCK_ALIGN == 0);
      eassert (ABLOCK_ABASE (&abase->blocks[3]) == abase);
      eassert (ABLOCK_ABASE (&abase->blocks[0]) == abase);
      eassert (ABLOCKS_BASE (abase) == base);
      eassert ((intptr_t) ABLOCKS_BUSY (abase) == aligned);
    }

  ASAN_UNPOISON_ABLOCK (free_ablock);
  abase = ABLOCK_ABASE (free_ablock);
  ABLOCKS_BUSY (abase)
    = (struct ablocks *) (2 + (intptr_t) ABLOCKS_BUSY (abase));
  val = free_ablock;
  free_ablock = free_ablock->x.next_free;

  if (type != MEM_TYPE_NON_LISP)
    mem_insert (val, (char *) val + nbytes, type);

  MALLOC_UNBLOCK_INPUT;

  eassert (0 == ((uintptr_t) val) % BLOCK_ALIGN);
  return val;
}

static void
lisp_align_free (void *block)
{
  struct ablock *ablock = block;
  struct ablocks *abase = ABLOCK_ABASE (ablock);

  MALLOC_BLOCK_INPUT;
  mem_delete (mem_find (block));
  ablock->x.next_free = free_ablock;
  ASAN_POISON_ABLOCK (ablock);
  free_ablock = ablock;
  intptr_t busy = (intptr_t) ABLOCKS_BUSY (abase) - 2;
  eassume (0 <= busy && busy <= 2 * ABLOCKS_SIZE - 1);
  ABLOCKS_BUSY (abase) = (struct ablocks *) busy;

  if (busy < 2)
    {
      int i = 0;
      bool aligned = busy;
      struct ablock **tem = &free_ablock;
      struct ablock *atop
        = &abase->blocks[aligned ? ABLOCKS_SIZE : ABLOCKS_SIZE - 1];
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
  MALLOC_UNBLOCK_INPUT;
}
static bool
laligned (void *p, size_t size)
{
  return (MALLOC_IS_LISP_ALIGNED || (intptr_t) p % LISP_ALIGNMENT == 0
          || size % LISP_ALIGNMENT != 0);
}
static void *
lmalloc (size_t size, bool clearit)
{
#ifdef USE_ALIGNED_ALLOC
  if (!MALLOC_IS_LISP_ALIGNED && size % LISP_ALIGNMENT == 0)
    {
      void *p = aligned_alloc (LISP_ALIGNMENT, size);
      if (p)
        {
          if (clearit)
            memclear (p, size);
        }
      else if (!(MALLOC_0_IS_NONNULL || size))
        return aligned_alloc (LISP_ALIGNMENT, LISP_ALIGNMENT);
      return p;
    }
#endif

  while (true)
    {
      void *p = clearit ? calloc (1, size) : malloc (size);
      if (laligned (p, size) && (MALLOC_0_IS_NONNULL || size || p))
        return p;
      free (p);
      size_t bigger = size + LISP_ALIGNMENT;
      if (size < bigger)
        size = bigger;
    }
}
static void *
lrealloc (void *p, size_t size)
{
  while (true)
    {
      p = realloc (p, size);
      if (laligned (p, size) && (size || p))
        return p;
      size_t bigger = size + LISP_ALIGNMENT;
      if (size < bigger)
        size = bigger;
    }
}

void *
hash_table_alloc_bytes (ptrdiff_t nbytes)
{
  if (nbytes == 0)
    return NULL;
  tally_consing (nbytes);
  hash_table_allocated_bytes += nbytes;
  return xmalloc (nbytes);
}
void
hash_table_free_bytes (void *p, ptrdiff_t nbytes)
{
  tally_consing (-nbytes);
  hash_table_allocated_bytes -= nbytes;
  xfree (p);
}

/* --- pure storage -- */

#define puresize_h_PURE_P(ptr) \
  ((uintptr_t) (ptr) - (uintptr_t) pure <= PURESIZE)
#define PURE_P(ptr) puresize_h_PURE_P (ptr)

static void *
pure_alloc (size_t size, int type)
{
  void *result;
  static bool pure_overflow_warned = false;

again:
  if (type >= 0)
    {
      result = pointer_align (purebeg + pure_bytes_used_lisp, LISP_ALIGNMENT);
      pure_bytes_used_lisp = ((char *) result - (char *) purebeg) + size;
    }
  else
    {
      ptrdiff_t unaligned_non_lisp = pure_bytes_used_non_lisp + size;
      char *unaligned = purebeg + pure_size - unaligned_non_lisp;
      int decr = (intptr_t) unaligned & (-1 - type);
      pure_bytes_used_non_lisp = unaligned_non_lisp + decr;
      result = unaligned - decr;
    }
  pure_bytes_used = pure_bytes_used_lisp + pure_bytes_used_non_lisp;

  if (pure_bytes_used <= pure_size)
    return result;

  if (!pure_overflow_warned)
    {
      TODO; // message ("Pure Lisp storage overflowed");
      pure_overflow_warned = true;
    }

  int small_amount = 10000;
  eassert ((long) size <= (long) small_amount - LISP_ALIGNMENT);
  purebeg = xzalloc (small_amount);
  pure_size = small_amount;
  pure_bytes_used_before_overflow += pure_bytes_used - size;
  pure_bytes_used = 0;
  pure_bytes_used_lisp = pure_bytes_used_non_lisp = 0;

#if TODO_NELISP_LATER_AND
  garbage_collection_inhibited++;
#endif
  goto again;
}

static char *
find_string_data_in_pure (const char *data, ptrdiff_t nbytes)
{
  int i;
  ptrdiff_t skip, bm_skip[256], last_char_skip, infinity, start, start_max;
  const unsigned char *p;
  char *non_lisp_beg;

  if (pure_bytes_used_non_lisp <= nbytes)
    return NULL;

  skip = nbytes + 1;
  for (i = 0; i < 256; i++)
    bm_skip[i] = skip;

  p = (const unsigned char *) data;
  while (--skip > 0)
    bm_skip[*p++] = skip;

  last_char_skip = bm_skip['\0'];

  non_lisp_beg = purebeg + pure_size - pure_bytes_used_non_lisp;
  start_max = pure_bytes_used_non_lisp - (nbytes + 1);

  infinity = pure_bytes_used_non_lisp + 1;
  bm_skip['\0'] = infinity;

  p = (const unsigned char *) non_lisp_beg + nbytes;
  start = 0;
  do
    {
      do
        {
          start += bm_skip[*(p + start)];
        }
      while (start <= start_max);

      if (start < infinity)
        return NULL;

      start -= infinity;

      if (memcmp (data, non_lisp_beg + start, nbytes) == 0)
        return non_lisp_beg + start;

      start += last_char_skip;
    }
  while (start <= start_max);

  return NULL;
}
Lisp_Object
make_pure_string (const char *data, ptrdiff_t nchars, ptrdiff_t nbytes,
                  bool multibyte)
{
  Lisp_Object string;
  struct Lisp_String *s = pure_alloc (sizeof *s, Lisp_String);
  s->u.s.data = (unsigned char *) find_string_data_in_pure (data, nbytes);
  if (s->u.s.data == NULL)
    {
      s->u.s.data = pure_alloc (nbytes + 1, -1);
      memcpy (s->u.s.data, data, nbytes);
      s->u.s.data[nbytes] = '\0';
    }
  s->u.s.size = nchars;
  s->u.s.size_byte = multibyte ? nbytes : -1;
  s->u.s.intervals = NULL;
  XSETSTRING (string, s);
  return string;
}
Lisp_Object
make_pure_c_string (const char *data, ptrdiff_t nchars)
{
  Lisp_Object string;
  struct Lisp_String *s = pure_alloc (sizeof *s, Lisp_String);
  s->u.s.size = nchars;
  s->u.s.size_byte = -2;
  s->u.s.data = (unsigned char *) data;
  s->u.s.intervals = NULL;
  XSETSTRING (string, s);
  return string;
}

static Lisp_Object
make_pure_vector (ptrdiff_t len)
{
  Lisp_Object new;
  size_t size = header_size + len * word_size;
  struct Lisp_Vector *p = pure_alloc (size, Lisp_Vectorlike);
  XSETVECTOR (new, p);
  XVECTOR (new)->header.size = len;
  return new;
}

/* --- string allocation -- */

enum
{
  SBLOCK_SIZE = MALLOC_SIZE_NEAR (8192)
};
#define LARGE_STRING_BYTES 1024
struct sdata
{
  struct Lisp_String *string;
  unsigned char data[FLEXIBLE_ARRAY_MEMBER];
};
typedef union
{
  struct Lisp_String *string;
  struct
  {
    struct Lisp_String *string;
    ptrdiff_t nbytes;
  } n;
} sdata;
#define SDATA_NBYTES(S) (S)->n.nbytes
#define SDATA_DATA(S) ((struct sdata *) (S))->data
enum
{
  SDATA_DATA_OFFSET = offsetof (struct sdata, data)
};
struct sblock
{
  struct sblock *next;
  sdata *next_free;
  sdata data[FLEXIBLE_ARRAY_MEMBER];
};
enum
{
  STRING_BLOCK_SIZE
  = ((MALLOC_SIZE_NEAR (1024) - sizeof (struct string_block *))
     / sizeof (struct Lisp_String))
};
struct string_block
{
  struct Lisp_String strings[STRING_BLOCK_SIZE];
  struct string_block *next;
};
static struct sblock *oldest_sblock, *current_sblock;
static struct sblock *large_sblocks;
static struct string_block *string_blocks;
static struct Lisp_String *string_free_list;
#define NEXT_FREE_LISP_STRING(S) ((S)->u.next)
#define SDATA_OF_STRING(S) ((sdata *) ((S)->u.s.data - SDATA_DATA_OFFSET))
#define GC_STRING_OVERRUN_COOKIE_SIZE 0
static ptrdiff_t
sdata_size (ptrdiff_t n)
{
  ptrdiff_t unaligned_size
    = max ((unsigned long) (SDATA_DATA_OFFSET + n + 1), sizeof (sdata));
  int sdata_align = max (FLEXALIGNOF (struct sdata), alignof (sdata));
  return (unaligned_size + sdata_align - 1) & ~(sdata_align - 1);
}
#define GC_STRING_EXTRA GC_STRING_OVERRUN_COOKIE_SIZE
static ptrdiff_t const STRING_BYTES_MAX
  = min (STRING_BYTES_BOUND,
         ((SIZE_MAX - GC_STRING_EXTRA - offsetof (struct sblock, data)
           - SDATA_DATA_OFFSET)
          & ~(sizeof (EMACS_INT) - 1)));
#define ASAN_PREPARE_DEAD_SDATA(s, size) ((void) 0)
#define ASAN_PREPARE_LIVE_SDATA(s, nbytes) ((void) 0)
#define ASAN_POISON_SBLOCK_DATA(b, size) ((void) 0)
#define ASAN_POISON_STRING_BLOCK(b) ((void) 0)
#define ASAN_UNPOISON_STRING_BLOCK(b) ((void) 0)
#define ASAN_POISON_STRING(s) ((void) 0)
#define ASAN_UNPOISON_STRING(s) ((void) 0)
#define check_string_free_list()
static struct Lisp_String *
allocate_string (void)
{
  struct Lisp_String *s;

  MALLOC_BLOCK_INPUT;

  if (string_free_list == NULL)
    {
      struct string_block *b = lisp_malloc (sizeof *b, false, MEM_TYPE_STRING);
      int i;

      b->next = string_blocks;
      string_blocks = b;

      for (i = STRING_BLOCK_SIZE - 1; i >= 0; --i)
        {
          s = b->strings + i;
          s->u.s.data = NULL;
          NEXT_FREE_LISP_STRING (s) = string_free_list;
          string_free_list = s;
        }
      ASAN_POISON_STRING_BLOCK (b);
    }

  check_string_free_list ();

  s = string_free_list;
  ASAN_UNPOISON_STRING (s);
  string_free_list = NEXT_FREE_LISP_STRING (s);

  MALLOC_UNBLOCK_INPUT;

  ++strings_consed;
  tally_consing (sizeof *s);

  return s;
}
static void
allocate_string_data (struct Lisp_String *s, EMACS_INT nchars, EMACS_INT nbytes,
                      bool clearit, bool immovable)
{
  sdata *data;
  struct sblock *b;

  if (STRING_BYTES_MAX < nbytes)
#if TODO_NELISP_LATER_AND
    string_overflow ();
#else
    TODO;
#endif

  ptrdiff_t needed = sdata_size (nbytes);

  MALLOC_BLOCK_INPUT;

  if (nbytes > LARGE_STRING_BYTES || immovable)
    {
      size_t size = FLEXSIZEOF (struct sblock, data, needed);

      b = lisp_malloc (size + GC_STRING_EXTRA, clearit, MEM_TYPE_NON_LISP);
      ASAN_POISON_SBLOCK_DATA (b, size);

      data = b->data;
      b->next = large_sblocks;
      b->next_free = data;
      large_sblocks = b;
    }
  else
    {
      b = current_sblock;

      if (b == NULL
          || (SBLOCK_SIZE - GC_STRING_EXTRA
              < (char *) b->next_free - (char *) b + needed))
        {
          b = lisp_malloc (SBLOCK_SIZE, false, MEM_TYPE_NON_LISP);
          ASAN_POISON_SBLOCK_DATA (b, SBLOCK_SIZE);

          data = b->data;
          b->next = NULL;
          b->next_free = data;

          if (current_sblock)
            current_sblock->next = b;
          else
            oldest_sblock = b;
          current_sblock = b;
        }

      data = b->next_free;

      if (clearit)
        {
          memset (SDATA_DATA (data), 0, nbytes);
        }
    }

  ASAN_PREPARE_LIVE_SDATA (data, nbytes);
  data->string = s;
  b->next_free = (sdata *) ((char *) data + needed + GC_STRING_EXTRA);
  eassert ((uintptr_t) b->next_free % alignof (sdata) == 0);

  MALLOC_UNBLOCK_INPUT;

  s->u.s.data = SDATA_DATA (data);
  s->u.s.size = nchars;
  s->u.s.size_byte = nbytes;
  s->u.s.data[nbytes] = '\0';

  tally_consing (needed);
}

static void
free_large_strings (void)
{
  struct sblock *b, *next;
  struct sblock *live_blocks = NULL;

  for (b = large_sblocks; b; b = next)
    {
      next = b->next;

      if (b->data[0].string == NULL)
        lisp_free (b);
      else
        {
          b->next = live_blocks;
          live_blocks = b;
        }
    }

  large_sblocks = live_blocks;
}
NO_INLINE static void
sweep_strings (void)
{
  struct string_block *b, *next;
  struct string_block *live_blocks = NULL;

  string_free_list = NULL;
  gcstat.total_strings = gcstat.total_free_strings = 0;
  gcstat.total_string_bytes = 0;

  for (b = string_blocks; b; b = next)
    {
      int i, nfree = 0;
      struct Lisp_String *free_list_before = string_free_list;

      ASAN_UNPOISON_STRING_BLOCK (b);

      next = b->next;

      for (i = 0; i < STRING_BLOCK_SIZE; ++i)
        {
          struct Lisp_String *s = b->strings + i;

          ASAN_UNPOISON_STRING (s);

          if (s->u.s.data)
            {
              if (XSTRING_MARKED_P (s))
                {
                  XUNMARK_STRING (s);

#if TODO_NELISP_LATER_AND
                  s->u.s.intervals = balance_intervals (s->u.s.intervals);
#endif

                  gcstat.total_strings++;
                  gcstat.total_string_bytes += STRING_BYTES (s);
                }
              else
                {
                  sdata *data = SDATA_OF_STRING (s);
                  data->n.nbytes = STRING_BYTES (s);
                  data->string = NULL;
                  s->u.s.data = NULL;
                  NEXT_FREE_LISP_STRING (s) = string_free_list;
                  ASAN_POISON_STRING (s);
                  ASAN_PREPARE_DEAD_SDATA (data, SDATA_NBYTES (data));
                  string_free_list = s;
                  ++nfree;
                }
            }
          else
            {
              NEXT_FREE_LISP_STRING (s) = string_free_list;
              ASAN_POISON_STRING (s);

              string_free_list = s;
              ++nfree;
            }
        }
      if (nfree == STRING_BLOCK_SIZE
          && gcstat.total_free_strings > STRING_BLOCK_SIZE)
        {
          lisp_free (b);
          string_free_list = free_list_before;
        }
      else
        {
          gcstat.total_free_strings += nfree;
          b->next = live_blocks;
          live_blocks = b;
        }
    }

  check_string_free_list ();

  string_blocks = live_blocks;
  free_large_strings ();
#if TODO_NELISP_LATER_AND
  compact_small_strings ();
#endif

  check_string_free_list ();
}

static Lisp_Object
make_clear_multibyte_string (EMACS_INT nchars, EMACS_INT nbytes, bool clearit)
{
  Lisp_Object string;
  struct Lisp_String *s;

  if (nchars < 0)
    emacs_abort ();
  if (!nbytes)
    return empty_multibyte_string;

  s = allocate_string ();
  s->u.s.intervals = NULL;
  allocate_string_data (s, nchars, nbytes, clearit, false);
  XSETSTRING (string, s);
  string_chars_consed += nbytes;
  return string;
}
Lisp_Object
make_uninit_multibyte_string (EMACS_INT nchars, EMACS_INT nbytes)
{
  return make_clear_multibyte_string (nchars, nbytes, false);
}
Lisp_Object
make_specified_string (const char *contents, ptrdiff_t nchars, ptrdiff_t nbytes,
                       bool multibyte)
{
  Lisp_Object val;

  if (nchars < 0)
    {
      if (multibyte)
        {
          TODO;
        }
      else
        nchars = nbytes;
    }
  val = make_uninit_multibyte_string (nchars, nbytes);
  memcpy (SDATA (val), contents, nbytes);
  if (!multibyte)
    STRING_SET_UNIBYTE (val);
  return val;
}
static Lisp_Object
make_clear_string (EMACS_INT length, bool clearit)
{
  Lisp_Object val;

  if (!length)
    return empty_unibyte_string;
  val = make_clear_multibyte_string (length, length, clearit);
  STRING_SET_UNIBYTE (val);
  return val;
}
Lisp_Object
make_uninit_string (EMACS_INT length)
{
  return make_clear_string (length, false);
}
Lisp_Object
make_unibyte_string (const char *contents, ptrdiff_t length)
{
  register Lisp_Object val;
  val = make_uninit_string (length);
  memcpy (SDATA (val), contents, length);
  return val;
}
Lisp_Object
make_string (const char *contents, ptrdiff_t nbytes)
{
  register Lisp_Object val;
  ptrdiff_t nchars, multibyte_nbytes;

  parse_str_as_multibyte ((const unsigned char *) contents, nbytes, &nchars,
                          &multibyte_nbytes);
  if (nbytes == nchars || nbytes != multibyte_nbytes)
    val = make_unibyte_string (contents, nbytes);
  else
    TODO;
  return val;
}

static void
init_strings (void)
{
  empty_unibyte_string = make_pure_string ("", 0, 0, 0);
  staticpro (&empty_unibyte_string);
  empty_multibyte_string = make_pure_string ("", 0, 0, 1);
  staticpro (&empty_multibyte_string);
}
/* --- float allocation -- */

#define FLOAT_BLOCK_SIZE                                  \
  (((BLOCK_BYTES - sizeof (struct float_block *)          \
     - (sizeof (struct Lisp_Float) - sizeof (bits_word))) \
    * CHAR_BIT)                                           \
   / (sizeof (struct Lisp_Float) * CHAR_BIT + 1))
#define GETMARKBIT(block, n)                      \
  (((block)->gcmarkbits[(n) / BITS_PER_BITS_WORD] \
    >> ((n) % BITS_PER_BITS_WORD))                \
   & 1)
#define SETMARKBIT(block, n)                     \
  ((block)->gcmarkbits[(n) / BITS_PER_BITS_WORD] \
   |= (bits_word) 1 << ((n) % BITS_PER_BITS_WORD))
#define UNSETMARKBIT(block, n)                   \
  ((block)->gcmarkbits[(n) / BITS_PER_BITS_WORD] \
   &= ~((bits_word) 1 << ((n) % BITS_PER_BITS_WORD)))
#define FLOAT_BLOCK(fptr)              \
  (eassert (!pdumper_object_p (fptr)), \
   ((struct float_block *) (((uintptr_t) (fptr)) & ~(BLOCK_ALIGN - 1))))
#define FLOAT_INDEX(fptr) \
  ((((uintptr_t) (fptr)) & (BLOCK_ALIGN - 1)) / sizeof (struct Lisp_Float))
struct float_block
{
  struct Lisp_Float floats[FLOAT_BLOCK_SIZE];
  bits_word gcmarkbits[1 + FLOAT_BLOCK_SIZE / BITS_PER_BITS_WORD];
  struct float_block *next;
};
#define XFLOAT_MARKED_P(fptr) \
  GETMARKBIT (FLOAT_BLOCK (fptr), FLOAT_INDEX (fptr))
#define XFLOAT_MARK(fptr) SETMARKBIT (FLOAT_BLOCK (fptr), FLOAT_INDEX (fptr))
#define XFLOAT_UNMARK(fptr) \
  UNSETMARKBIT (FLOAT_BLOCK (fptr), FLOAT_INDEX (fptr))
#define ASAN_POISON_FLOAT_BLOCK(fblk) ((void) 0)
#define ASAN_UNPOISON_FLOAT_BLOCK(fblk) ((void) 0)
#define ASAN_POISON_FLOAT(p) ((void) 0)
#define ASAN_UNPOISON_FLOAT(p) ((void) 0)
static struct float_block *float_block;
static int float_block_index = FLOAT_BLOCK_SIZE;
static struct Lisp_Float *float_free_list;
Lisp_Object
make_float (double float_value)
{
  register Lisp_Object val;

  MALLOC_BLOCK_INPUT;

  if (float_free_list)
    {
      XSETFLOAT (val, float_free_list);
      ASAN_UNPOISON_FLOAT (float_free_list);
      float_free_list = float_free_list->u.chain;
    }
  else
    {
      if (float_block_index == FLOAT_BLOCK_SIZE)
        {
          struct float_block *new
            = lisp_align_malloc (sizeof *new, MEM_TYPE_FLOAT);
          new->next = float_block;
          memset (new->gcmarkbits, 0, sizeof new->gcmarkbits);
          ASAN_POISON_FLOAT_BLOCK (new);
          float_block = new;
          float_block_index = 0;
        }
      ASAN_UNPOISON_FLOAT (&float_block->floats[float_block_index]);
      XSETFLOAT (val, &float_block->floats[float_block_index]);
      float_block_index++;
    }

  MALLOC_UNBLOCK_INPUT;

  XFLOAT_INIT (val, float_value);
  eassert (!XFLOAT_MARKED_P (XFLOAT (val)));
  tally_consing (sizeof (struct Lisp_Float));
  floats_consed++;
  return val;
}

/* --- cons allocation -- */

#define CONS_BLOCK_SIZE                                  \
  (((BLOCK_BYTES - sizeof (struct cons_block *)          \
     - (sizeof (struct Lisp_Cons) - sizeof (bits_word))) \
    * CHAR_BIT)                                          \
   / (sizeof (struct Lisp_Cons) * CHAR_BIT + 1))
#define CONS_BLOCK(fptr)               \
  (eassert (!pdumper_object_p (fptr)), \
   ((struct cons_block *) ((uintptr_t) (fptr) & ~(BLOCK_ALIGN - 1))))
#define CONS_INDEX(fptr) \
  (((uintptr_t) (fptr) & (BLOCK_ALIGN - 1)) / sizeof (struct Lisp_Cons))
struct cons_block
{
  struct Lisp_Cons conses[CONS_BLOCK_SIZE];
  bits_word gcmarkbits[1 + CONS_BLOCK_SIZE / BITS_PER_BITS_WORD];
  struct cons_block *next;
};
#define XCONS_MARKED_P(fptr) GETMARKBIT (CONS_BLOCK (fptr), CONS_INDEX (fptr))
#define XMARK_CONS(fptr) SETMARKBIT (CONS_BLOCK (fptr), CONS_INDEX (fptr))
#define XUNMARK_CONS(fptr) UNSETMARKBIT (CONS_BLOCK (fptr), CONS_INDEX (fptr))
static struct cons_block *cons_block;
static int cons_block_index = CONS_BLOCK_SIZE;
static struct Lisp_Cons *cons_free_list;
#define ASAN_POISON_CONS_BLOCK(b) ((void) 0)
#define ASAN_POISON_CONS(p) ((void) 0)
#define ASAN_UNPOISON_CONS(p) ((void) 0)

DEFUN ("cons", Fcons, Scons, 2, 2, 0,
       doc: /* Create a new cons, give it CAR and CDR as components, and return it.  */)
(Lisp_Object car, Lisp_Object cdr)
{
  register Lisp_Object val;

  MALLOC_BLOCK_INPUT;

  if (cons_free_list)
    {
      ASAN_UNPOISON_CONS (cons_free_list);
      XSETCONS (val, cons_free_list);
      cons_free_list = cons_free_list->u.s.u.chain;
    }
  else
    {
      if (cons_block_index == CONS_BLOCK_SIZE)
        {
          struct cons_block *new
            = lisp_align_malloc (sizeof *new, MEM_TYPE_CONS);
          memset (new->gcmarkbits, 0, sizeof new->gcmarkbits);
          ASAN_POISON_CONS_BLOCK (new);
          new->next = cons_block;
          cons_block = new;
          cons_block_index = 0;
        }
      ASAN_UNPOISON_CONS (&cons_block->conses[cons_block_index]);
      XSETCONS (val, &cons_block->conses[cons_block_index]);
      cons_block_index++;
    }

  MALLOC_UNBLOCK_INPUT;

  XSETCAR (val, car);
  XSETCDR (val, cdr);
  eassert (!XCONS_MARKED_P (XCONS (val)));
#if TODO_NELISP_LATER_AND
  consing_until_gc -= sizeof (struct Lisp_Cons);
#endif
  cons_cells_consed++;
  return val;
}

Lisp_Object
list1 (Lisp_Object arg1)
{
  return Fcons (arg1, Qnil);
}

Lisp_Object
list2 (Lisp_Object arg1, Lisp_Object arg2)
{
  return Fcons (arg1, Fcons (arg2, Qnil));
}

Lisp_Object
list3 (Lisp_Object arg1, Lisp_Object arg2, Lisp_Object arg3)
{
  return Fcons (arg1, Fcons (arg2, Fcons (arg3, Qnil)));
}

Lisp_Object
list4 (Lisp_Object arg1, Lisp_Object arg2, Lisp_Object arg3, Lisp_Object arg4)
{
  return Fcons (arg1, Fcons (arg2, Fcons (arg3, Fcons (arg4, Qnil))));
}

Lisp_Object
list5 (Lisp_Object arg1, Lisp_Object arg2, Lisp_Object arg3, Lisp_Object arg4,
       Lisp_Object arg5)
{
  return Fcons (arg1,
                Fcons (arg2, Fcons (arg3, Fcons (arg4, Fcons (arg5, Qnil)))));
}

/* --- vector allocation -- */

static struct Lisp_Vector *
next_vector (struct Lisp_Vector *v)
{
  return XUNTAG (v->contents[0], Lisp_Int0, struct Lisp_Vector);
}
static void
set_next_vector (struct Lisp_Vector *v, struct Lisp_Vector *p)
{
  v->contents[0] = make_lisp_ptr (p, Lisp_Int0);
}
enum
{
  VECTOR_BLOCK_SIZE = 4096
};
enum
{
  roundup_size = COMMON_MULTIPLE (LISP_ALIGNMENT, word_size)
};
#define vroundup_ct(x) ROUNDUP (x, roundup_size)
#define vroundup(x) (eassume ((x) >= 0), vroundup_ct (x))
enum
{
  VECTOR_BLOCK_BYTES = VECTOR_BLOCK_SIZE - vroundup_ct (sizeof (void *))
};
enum
{
  VBLOCK_BYTES_MIN = vroundup_ct (header_size + sizeof (Lisp_Object))
};
enum
{
  VBLOCK_BYTES_MAX = vroundup_ct ((VECTOR_BLOCK_BYTES / 2) - word_size)
};
enum
{
  VECTOR_FREE_LIST_ARRAY_SIZE
  = (VBLOCK_BYTES_MAX - VBLOCK_BYTES_MIN) / roundup_size + 1 + 2
};
static struct Lisp_Vector *
ADVANCE (struct Lisp_Vector *v, ptrdiff_t nbytes)
{
  void *vv = v;
  char *cv = vv;
  void *p = cv + nbytes;
  return p;
}
static ptrdiff_t
VINDEX (ptrdiff_t nbytes)
{
  eassume (VBLOCK_BYTES_MIN <= nbytes);
  return (nbytes - VBLOCK_BYTES_MIN) / roundup_size;
}
struct large_vector
{
  struct large_vector *next;
};
enum
{
  large_vector_offset = ROUNDUP (sizeof (struct large_vector), LISP_ALIGNMENT)
};
static struct Lisp_Vector *
large_vector_vec (struct large_vector *p)
{
  return (struct Lisp_Vector *) ((char *) p + large_vector_offset);
}

struct vector_block
{
  char data[VECTOR_BLOCK_BYTES];
  struct vector_block *next;
};
static struct vector_block *vector_blocks;
static struct Lisp_Vector *vector_free_lists[VECTOR_FREE_LIST_ARRAY_SIZE];
static ptrdiff_t last_inserted_vector_free_idx = VECTOR_FREE_LIST_ARRAY_SIZE;
static struct large_vector *large_vectors;
Lisp_Object zero_vector;
#define ASAN_POISON_VECTOR_CONTENTS(v, bytes) ((void) 0)
#define ASAN_UNPOISON_VECTOR_CONTENTS(v, bytes) ((void) 0)
#define ASAN_UNPOISON_VECTOR_BLOCK(b) ((void) 0)
static void
setup_on_free_list (struct Lisp_Vector *v, ptrdiff_t nbytes)
{
  eassume (header_size <= nbytes);
  ptrdiff_t nwords = (nbytes - header_size) / word_size;
  XSETPVECTYPESIZE (v, PVEC_FREE, 0, nwords);
  eassert (nbytes % roundup_size == 0);
  ptrdiff_t vindex = VINDEX (nbytes);
  vindex = min (vindex, VECTOR_FREE_LIST_ARRAY_SIZE - 1);
  set_next_vector (v, vector_free_lists[vindex]);
  ASAN_POISON_VECTOR_CONTENTS (v, nbytes - header_size);
  vector_free_lists[vindex] = v;
  last_inserted_vector_free_idx = vindex;
}
static struct vector_block *
allocate_vector_block (void)
{
  struct vector_block *block = xmalloc (sizeof *block);

  mem_insert (block->data, block->data + VECTOR_BLOCK_BYTES,
              MEM_TYPE_VECTOR_BLOCK);

  block->next = vector_blocks;
  vector_blocks = block;
  return block;
}

static void
init_vectors (void)
{
  zero_vector = make_pure_vector (0);
  staticpro (&zero_vector);
}
static ptrdiff_t
pseudovector_nbytes (const union vectorlike_header *hdr)
{
  eassert (!PSEUDOVECTOR_TYPEP (hdr, PVEC_BOOL_VECTOR));
  ptrdiff_t nwords
    = ((hdr->size & PSEUDOVECTOR_SIZE_MASK)
       + ((hdr->size & PSEUDOVECTOR_REST_MASK) >> PSEUDOVECTOR_SIZE_BITS));
  return vroundup (header_size + word_size * nwords);
}
static struct Lisp_Vector *
allocate_vector_from_block (ptrdiff_t nbytes)
{
  struct Lisp_Vector *vector;
  struct vector_block *block;
  size_t index, restbytes;

  eassume (VBLOCK_BYTES_MIN <= nbytes && nbytes <= VBLOCK_BYTES_MAX);
  eassume (nbytes % roundup_size == 0);

  index = VINDEX (nbytes);
  if (vector_free_lists[index])
    {
      vector = vector_free_lists[index];
      ASAN_UNPOISON_VECTOR_CONTENTS (vector, nbytes - header_size);
      vector_free_lists[index] = next_vector (vector);
      return vector;
    }

  for (index = max (VINDEX (nbytes + VBLOCK_BYTES_MIN),
                    last_inserted_vector_free_idx);
       index < VECTOR_FREE_LIST_ARRAY_SIZE; index++)
    if (vector_free_lists[index])
      {
        vector = vector_free_lists[index];
        size_t vector_nbytes = pseudovector_nbytes (&vector->header);
        eassert ((long) vector_nbytes > nbytes);
        ASAN_UNPOISON_VECTOR_CONTENTS (vector, nbytes - header_size);
        vector_free_lists[index] = next_vector (vector);

        restbytes = vector_nbytes - nbytes;
        eassert (restbytes % roundup_size == 0);
#if GC_ASAN_POISON_OBJECTS
#endif
        setup_on_free_list (ADVANCE (vector, nbytes), restbytes);
        return vector;
      }

  block = allocate_vector_block ();

  vector = (struct Lisp_Vector *) block->data;

  restbytes = VECTOR_BLOCK_BYTES - nbytes;
  if (restbytes >= VBLOCK_BYTES_MIN)
    {
      eassert (restbytes % roundup_size == 0);
      setup_on_free_list (ADVANCE (vector, nbytes), restbytes);
    }
  return vector;
}
#define VECTOR_IN_BLOCK(vector, block) \
  ((char *) (vector) <= (block)->data + VECTOR_BLOCK_BYTES - VBLOCK_BYTES_MIN)
ptrdiff_t
vectorlike_nbytes (const union vectorlike_header *hdr)
{
  ptrdiff_t size = hdr->size & ~ARRAY_MARK_FLAG;
  ptrdiff_t nwords;

  if (size & PSEUDOVECTOR_FLAG)
    {
      if (PSEUDOVECTOR_TYPEP (hdr, PVEC_BOOL_VECTOR))
        {
          TODO;
          nwords = 0;
        }
      else
        return pseudovector_nbytes (hdr);
    }
  else
    {
      nwords = size;
    }
  return vroundup (header_size + word_size * nwords);
}
INLINE ptrdiff_t
vector_nbytes (const struct Lisp_Vector *v)
{
  return vectorlike_nbytes (&v->header);
}
static void
cleanup_vector (struct Lisp_Vector *vector)
{
  if ((vector->header.size & PSEUDOVECTOR_FLAG) == 0)
    return;
  switch (PSEUDOVECTOR_TYPE (vector))
    {
    case PVEC_BIGNUM:
      TODO;
      break;
    case PVEC_OVERLAY:
      TODO;
      break;
    case PVEC_FINALIZER:
      TODO;
      break;
    case PVEC_FONT:
      TODO;
      break;
    case PVEC_THREAD:
      TODO;
      break;
    case PVEC_MUTEX:
      TODO;
      break;
    case PVEC_CONDVAR:
      TODO;
      break;
    case PVEC_MARKER:
      TODO;
      break;
    case PVEC_USER_PTR:
      TODO;
      break;
    case PVEC_TS_PARSER:
      TODO;
      break;
    case PVEC_TS_COMPILED_QUERY:
      TODO;
      break;
    case PVEC_MODULE_FUNCTION:
      TODO;
      break;
    case PVEC_NATIVE_COMP_UNIT:
      TODO;
      break;
    case PVEC_SUBR:
      TODO;
      break;
    case PVEC_HASH_TABLE:
      TODO;
      break;
    case PVEC_OBARRAY:
      TODO;
      break;
    case PVEC_NORMAL_VECTOR:
    case PVEC_FREE:
    case PVEC_SYMBOL_WITH_POS:
    case PVEC_MISC_PTR:
    case PVEC_PROCESS:
    case PVEC_FRAME:
    case PVEC_WINDOW:
    case PVEC_BOOL_VECTOR:
    case PVEC_BUFFER:
    case PVEC_TERMINAL:
    case PVEC_WINDOW_CONFIGURATION:
    case PVEC_OTHER:
    case PVEC_XWIDGET:
    case PVEC_XWIDGET_VIEW:
    case PVEC_TS_NODE:
    case PVEC_SQLITE:
    case PVEC_CLOSURE:
    case PVEC_CHAR_TABLE:
    case PVEC_SUB_CHAR_TABLE:
    case PVEC_RECORD:
      break;
    }
}

NO_INLINE static void
sweep_vectors (void)
{
  struct vector_block *block, **bprev = &vector_blocks;
  struct large_vector *lv, **lvprev = &large_vectors;
  struct Lisp_Vector *vector, *next;

  gcstat.total_vectors = 0;
  gcstat.total_vector_slots = gcstat.total_free_vector_slots = 0;
  memset (vector_free_lists, 0, sizeof (vector_free_lists));
  last_inserted_vector_free_idx = VECTOR_FREE_LIST_ARRAY_SIZE;

  for (block = vector_blocks; block; block = *bprev)
    {
      bool free_this_block = false;

      for (vector = (struct Lisp_Vector *) block->data;
           VECTOR_IN_BLOCK (vector, block); vector = next)
        {
          ASAN_UNPOISON_VECTOR_BLOCK (block);
          if (XVECTOR_MARKED_P (vector))
            {
              XUNMARK_VECTOR (vector);
              gcstat.total_vectors++;
              ptrdiff_t nbytes = vector_nbytes (vector);
              gcstat.total_vector_slots += nbytes / word_size;
              next = ADVANCE (vector, nbytes);
            }
          else
            {
              ptrdiff_t total_bytes = 0;

              next = vector;
              do
                {
                  cleanup_vector (next);
                  ptrdiff_t nbytes = vector_nbytes (next);
                  total_bytes += nbytes;
                  next = ADVANCE (next, nbytes);
                }
              while (VECTOR_IN_BLOCK (next, block) && !vector_marked_p (next));

              eassert (total_bytes % roundup_size == 0);

              if (vector == (struct Lisp_Vector *) block->data
                  && !VECTOR_IN_BLOCK (next, block))
                free_this_block = true;
              else
                {
                  setup_on_free_list (vector, total_bytes);
                  gcstat.total_free_vector_slots += total_bytes / word_size;
                }
            }
        }

      if (free_this_block)
        {
          *bprev = block->next;
          mem_delete (mem_find (block->data));
          xfree (block);
        }
      else
        bprev = &block->next;
    }

  for (lv = large_vectors; lv; lv = *lvprev)
    {
      vector = large_vector_vec (lv);
      if (XVECTOR_MARKED_P (vector))
        {
          XUNMARK_VECTOR (vector);
          gcstat.total_vectors++;
          gcstat.total_vector_slots
            += (vector->header.size & PSEUDOVECTOR_FLAG
                  ? vector_nbytes (vector) / word_size
                  : header_size / word_size + vector->header.size);
          lvprev = &lv->next;
        }
      else
        {
          *lvprev = lv->next;
          lisp_free (lv);
        }
    }

  gcstat.total_hash_table_bytes = hash_table_allocated_bytes;
}
#define VECTOR_ELTS_MAX                                         \
  ((ptrdiff_t) min (((min (PTRDIFF_MAX, SIZE_MAX) - header_size \
                      - large_vector_offset)                    \
                     / word_size),                              \
                    MOST_POSITIVE_FIXNUM))
static struct Lisp_Vector *
allocate_vectorlike (ptrdiff_t len, bool clearit)
{
  eassert (0 < len && len <= VECTOR_ELTS_MAX);
  ptrdiff_t nbytes = header_size + len * word_size;
  struct Lisp_Vector *p;

  MALLOC_BLOCK_INPUT;

  if (nbytes <= VBLOCK_BYTES_MAX)
    {
      p = allocate_vector_from_block (vroundup (nbytes));
      if (clearit)
        memclear (p, nbytes);
    }
  else
    {
      struct large_vector *lv = lisp_malloc (large_vector_offset + nbytes,
                                             clearit, MEM_TYPE_VECTORLIKE);
      lv->next = large_vectors;
      large_vectors = lv;
      p = large_vector_vec (lv);
    }

  tally_consing (nbytes);
  vector_cells_consed += len;

  MALLOC_UNBLOCK_INPUT;

  return p;
}
static struct Lisp_Vector *
allocate_clear_vector (ptrdiff_t len, bool clearit)
{
  if (len == 0)
    return XVECTOR (zero_vector);
  if (VECTOR_ELTS_MAX < len)
    memory_full (SIZE_MAX);
  struct Lisp_Vector *v = allocate_vectorlike (len, clearit);
  v->header.size = len;
  return v;
}

struct Lisp_Vector *
allocate_pseudovector (int memlen, int lisplen, int zerolen, enum pvec_type tag)
{
  enum
  {
    size_max = (1 << PSEUDOVECTOR_SIZE_BITS) - 1
  };
  enum
  {
    rest_max = (1 << PSEUDOVECTOR_REST_BITS) - 1
  };
  eassert (0 <= tag && tag <= PVEC_TAG_MAX);
  eassert (0 <= lisplen && lisplen <= zerolen && zerolen <= memlen);
  eassert (lisplen <= size_max);
  eassert (memlen <= size_max + rest_max);

  struct Lisp_Vector *v = allocate_vectorlike (memlen, false);
  memclear (v->contents, zerolen * word_size);
  XSETPVECTYPESIZE (v, tag, lisplen, memlen - lisplen);
  return v;
}

Lisp_Object
make_vector (ptrdiff_t length, Lisp_Object init)
{
  bool clearit = NIL_IS_ZERO && NILP (init);
  struct Lisp_Vector *p = allocate_clear_vector (length, clearit);
  if (!clearit)
    for (ptrdiff_t i = 0; i < length; i++)
      p->contents[i] = init;
  return make_lisp_ptr (p, Lisp_Vectorlike);
}
DEFUN ("make-vector", Fmake_vector, Smake_vector, 2, 2, 0,
       doc: /* Return a newly created vector of length LENGTH, with each element being INIT.
See also the function `vector'.  */)
(Lisp_Object length, Lisp_Object init)
{
  CHECK_TYPE (FIXNATP (length) && XFIXNAT (length) <= PTRDIFF_MAX, Qwholenump,
              length);
  return make_vector (XFIXNAT (length), init);
}

/* --- symbol allocation -- */

#define SYMBOL_BLOCK_SIZE \
  ((1020 - sizeof (struct symbol_block *)) / sizeof (struct Lisp_Symbol))
struct symbol_block
{
  struct Lisp_Symbol symbols[SYMBOL_BLOCK_SIZE];
  struct symbol_block *next;
};
#define ASAN_POISON_SYMBOL_BLOCK(s) ((void) 0)
#define ASAN_UNPOISON_SYMBOL_BLOCK(s) ((void) 0)
#define ASAN_POISON_SYMBOL(sym) ((void) 0)
#define ASAN_UNPOISON_SYMBOL(sym) ((void) 0)
static struct symbol_block *symbol_block;
static int symbol_block_index = SYMBOL_BLOCK_SIZE;
static struct Lisp_Symbol *symbol_free_list;
static void
set_symbol_name (Lisp_Object sym, Lisp_Object name)
{
  XBARE_SYMBOL (sym)->u.s.name = name;
}
void
init_symbol (Lisp_Object val, Lisp_Object name)
{
  struct Lisp_Symbol *p = XBARE_SYMBOL (val);
  set_symbol_name (val, name);
  set_symbol_plist (val, Qnil);
  p->u.s.redirect = SYMBOL_PLAINVAL;
  SET_SYMBOL_VAL (p, Qunbound);
  set_symbol_function (val, Qnil);
  set_symbol_next (val, NULL);
  p->u.s.gcmarkbit = false;
  p->u.s.interned = SYMBOL_UNINTERNED;
  p->u.s.trapped_write = SYMBOL_UNTRAPPED_WRITE;
  p->u.s.declared_special = false;
  p->u.s.pinned = false;
}
DEFUN ("make-symbol", Fmake_symbol, Smake_symbol, 1, 1, 0,
       doc: /* Return a newly allocated uninterned symbol whose name is NAME.
Its value is void, and its function definition and property list are nil.  */)
(Lisp_Object name)
{
  Lisp_Object val;

  CHECK_STRING (name);

  MALLOC_BLOCK_INPUT;

  if (symbol_free_list)
    {
      ASAN_UNPOISON_SYMBOL (symbol_free_list);
      XSETSYMBOL (val, symbol_free_list);
      symbol_free_list = symbol_free_list->u.s.next;
    }
  else
    {
      if (symbol_block_index == SYMBOL_BLOCK_SIZE)
        {
          struct symbol_block *new
            = lisp_malloc (sizeof *new, false, MEM_TYPE_SYMBOL);
          ASAN_POISON_SYMBOL_BLOCK (new);
          new->next = symbol_block;
          symbol_block = new;
          symbol_block_index = 0;
        }

      ASAN_UNPOISON_SYMBOL (&symbol_block->symbols[symbol_block_index]);
      XSETSYMBOL (val, &symbol_block->symbols[symbol_block_index]);
      symbol_block_index++;
    }

  MALLOC_UNBLOCK_INPUT;

  init_symbol (val, name);
  tally_consing (sizeof (struct Lisp_Symbol));
  symbols_consed++;
  return val;
}

/* --- mark bit -- */
static bool
vector_marked_p (const struct Lisp_Vector *v)
{
  if (pdumper_object_p (v))
    {
      if (pdumper_cold_object_p (v))
        {
          eassert (PSEUDOVECTOR_TYPE (v) == PVEC_BOOL_VECTOR);
          return true;
        }
      return pdumper_marked_p (v);
    }
  return XVECTOR_MARKED_P (v);
}
static void
set_vector_marked (struct Lisp_Vector *v)
{
  if (pdumper_object_p (v))
    {
      eassert (PSEUDOVECTOR_TYPE (v) != PVEC_BOOL_VECTOR);
      pdumper_set_marked (v);
    }
  else
    XMARK_VECTOR (v);
}

static bool
string_marked_p (const struct Lisp_String *s)
{
  return pdumper_object_p (s) ? pdumper_marked_p (s) : XSTRING_MARKED_P (s);
}
static void
set_string_marked (struct Lisp_String *s)
{
  if (pdumper_object_p (s))
    pdumper_set_marked (s);
  else
    XMARK_STRING (s);
}

static bool
symbol_marked_p (const struct Lisp_Symbol *s)
{
  return pdumper_object_p (s) ? pdumper_marked_p (s) : s->u.s.gcmarkbit;
}
static void
set_symbol_marked (struct Lisp_Symbol *s)
{
  if (pdumper_object_p (s))
    pdumper_set_marked (s);
  else
    s->u.s.gcmarkbit = true;
}

static bool
cons_marked_p (const struct Lisp_Cons *c)
{
  return pdumper_object_p (c) ? pdumper_marked_p (c) : XCONS_MARKED_P (c);
}
static void
set_cons_marked (struct Lisp_Cons *c)
{
  if (pdumper_object_p (c))
    pdumper_set_marked (c);
  else
    XMARK_CONS (c);
}

/* --- c stack marking --- */
static void
mem_init (void)
{
  mem_z.left = mem_z.right = MEM_NIL;
  mem_z.parent = NULL;
  mem_z.color = MEM_BLACK;
  mem_z.start = mem_z.end = NULL;
  mem_root = MEM_NIL;
}

static struct mem_node *
mem_find (void *start)
{
  struct mem_node *p;

  if (start < min_heap_address || start > max_heap_address)
    return MEM_NIL;

  mem_z.start = start;
  mem_z.end = (char *) start + 1;

  p = mem_root;
  while (start < p->start || start >= p->end)
    p = start < p->start ? p->left : p->right;
  return p;
}
static struct mem_node *
mem_insert (void *start, void *end, enum mem_type type)
{
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

  x = xmalloc (sizeof *x);
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
  else
    mem_root = x;

  mem_insert_fixup (x);

  return x;
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
static void
mem_rotate_left (struct mem_node *x)
{
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
  else
    mem_root = y;

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
  else
    mem_root = y;

  y->right = x;
  if (x != MEM_NIL)
    x->parent = y;
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
  else
    mem_root = x;

  if (y != z)
    {
      z->start = y->start;
      z->end = y->end;
      z->type = y->type;
    }

  if (y->color == MEM_BLACK)
    mem_delete_fixup (x);

  xfree (y);
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
mark_maybe_pointer (void *p, bool symbol_only)
{
  struct mem_node *m;

  if (pdumper_object_p (p))
    {
      TODO;
    }

  m = mem_find (p);
  if (m != MEM_NIL)
    {
      Lisp_Object obj;
#if TODO_NELISP_LATER_ELSE
      obj = NULL;
#endif

      switch (m->type)
        {
        case MEM_TYPE_NON_LISP:
        case MEM_TYPE_SPARE:
          return;
        case MEM_TYPE_CONS:
          {
            if (symbol_only)
              return;
            TODO;
          }
          break;
        case MEM_TYPE_STRING:
          {
            if (symbol_only)
              return;
            TODO;
          }
          break;
        case MEM_TYPE_SYMBOL:
          {
            TODO;
          }
          break;
        case MEM_TYPE_FLOAT:
          {
            TODO;
          }
          break;
        case MEM_TYPE_VECTORLIKE:
          {
            TODO;
          }
          break;

        case MEM_TYPE_VECTOR_BLOCK:
          {
            TODO;
          }
          break;
        default:
          emacs_abort ();
        }
#if TODO_NELISP_LATER_ELSE
      if (obj)
#endif
        mark_object (obj);
    }
}

/* --- garbage collector -- */

void
staticpro (Lisp_Object const *varaddress)
{
  for (int i = 0; i < staticidx; i++)
    eassert (staticvec[i] != varaddress);
  if (staticidx >= NSTATICS)
    {
      TODO; // fatal ("NSTATICS too small; try increasing and recompiling
            // Emacs.");
    }
  staticvec[staticidx++] = varaddress;
}

struct mark_entry
{
  ptrdiff_t n;
  union
  {
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
static struct mark_stack mark_stk = { NULL, 0, 0 };
static inline bool
mark_stack_empty_p (void)
{
  return mark_stk.sp <= 0;
}
static inline Lisp_Object
mark_stack_pop (void)
{
  eassume (!mark_stack_empty_p ());
  struct mark_entry *e = &mark_stk.stack[mark_stk.sp - 1];
  if (e->n == 0)
    {
      --mark_stk.sp;
      return e->u.value;
    }
  e->n--;
  if (e->n == 0)
    --mark_stk.sp;
  return (++e->u.values)[-1];
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
  mark_stk.stack[mark_stk.sp++]
    = (struct mark_entry) { .n = 0, .u.value = value };
}
static inline void
mark_stack_push_values (Lisp_Object *values, ptrdiff_t n)
{
  eassume (n >= 0);
  if (n == 0)
    return;
  if (mark_stk.sp >= mark_stk.size)
    grow_mark_stack ();
  mark_stk.stack[mark_stk.sp++]
    = (struct mark_entry) { .n = n, .u.values = values };
}
static void
process_mark_stack (ptrdiff_t base_sp)
{
  eassume (mark_stk.sp >= base_sp && base_sp >= 0);

  while (mark_stk.sp > base_sp)
    {
      Lisp_Object obj = mark_stack_pop ();
    mark_obj:;
      void *po = XPNTR (obj);
      if (PURE_P (po))
        continue;
#define CHECK_ALLOCATED_AND_LIVE(LIVEP, MEM_TYPE) ((void) 0)
#define CHECK_ALLOCATED_AND_LIVE_SYMBOL() ((void) 0)
      switch (XTYPE (obj))
        {
        case Lisp_String:
          {
            register struct Lisp_String *ptr = XSTRING (obj);
            if (string_marked_p (ptr))
              break;
            CHECK_ALLOCATED_AND_LIVE (live_string_p, MEM_TYPE_STRING);
            set_string_marked (ptr);
#if TODO_NELISP_LATER_AND
            mark_interval_tree (ptr->u.s.intervals);
#endif
          }
          break;

        case Lisp_Vectorlike:
          {
            register struct Lisp_Vector *ptr = XVECTOR (obj);

            if (vector_marked_p (ptr))
              break;

            enum pvec_type pvectype = PSEUDOVECTOR_TYPE (ptr);

            switch (pvectype)
              {
              case PVEC_BUFFER:
                TODO;
                break;

              case PVEC_FRAME:
                TODO;
                break;

              case PVEC_WINDOW:
                TODO;
                break;

              case PVEC_HASH_TABLE:
                TODO;
                break;
              case PVEC_OBARRAY:
                {
                  struct Lisp_Obarray *o = (struct Lisp_Obarray *) ptr;
                  set_vector_marked (ptr);
                  mark_stack_push_values (o->buckets, obarray_size (o));
                }
                break;

              case PVEC_CHAR_TABLE:
              case PVEC_SUB_CHAR_TABLE:
                TODO;
                break;

              case PVEC_BOOL_VECTOR:
                TODO;
                break;

              case PVEC_OVERLAY:
                TODO;
                break;

              case PVEC_SUBR:
                break;

              case PVEC_FREE:
                emacs_abort ();

              default:
                TODO;
                break;
              }
          }
          break;

        case Lisp_Symbol:
          {
            struct Lisp_Symbol *ptr = XBARE_SYMBOL (obj);
          nextsym:
            if (symbol_marked_p (ptr))
              break;
            CHECK_ALLOCATED_AND_LIVE_SYMBOL ();
            set_symbol_marked (ptr);
#if TODO_NELISP_LATER_AND
            eassert (valid_lisp_object_p (ptr->u.s.function));
#endif
            mark_stack_push_value (ptr->u.s.function);
            mark_stack_push_value (ptr->u.s.plist);
            switch (ptr->u.s.redirect)
              {
              case SYMBOL_PLAINVAL:
                mark_stack_push_value (SYMBOL_VAL (ptr));
                break;
              case SYMBOL_VARALIAS:
                TODO;
                break;
              case SYMBOL_LOCALIZED:
                TODO;
                break;
              case SYMBOL_FORWARDED:
                break;
              default:
                emacs_abort ();
              }
            if (!PURE_P (XSTRING (ptr->u.s.name)))
              set_string_marked (XSTRING (ptr->u.s.name));
#if TODO_NELISP_LATER_AND
            mark_interval_tree (string_intervals (ptr->u.s.name));
#endif
            po = ptr = ptr->u.s.next;
            if (ptr)
              goto nextsym;
          }
          break;

        case Lisp_Cons:
          {
            struct Lisp_Cons *ptr = XCONS (obj);
            if (cons_marked_p (ptr))
              break;
            CHECK_ALLOCATED_AND_LIVE (live_cons_p, MEM_TYPE_CONS);
            set_cons_marked (ptr);
            if (!NILP (ptr->u.s.u.cdr))
              {
                mark_stack_push_value (ptr->u.s.u.cdr);
              }
            obj = ptr->u.s.car;
            goto mark_obj;
          }

        case Lisp_Float:
          {
            struct Lisp_Float *f = XFLOAT (obj);
            if (!f)
              break;
            CHECK_ALLOCATED_AND_LIVE (live_float_p, MEM_TYPE_FLOAT);
            if (pdumper_object_p (f))
              eassert (pdumper_cold_object_p (f));
            else if (!XFLOAT_MARKED_P (f))
              XFLOAT_MARK (f);
            break;
          }

        case_Lisp_Int:
          break;

        default:
          emacs_abort ();
        }
    }

#undef CHECK_LIVE
#undef CHECK_ALLOCATED
#undef CHECK_ALLOCATED_AND_LIVE
}
void
mark_object (Lisp_Object obj)
{
  ptrdiff_t sp = mark_stk.sp;
  mark_stack_push_value (obj);
  process_mark_stack (sp);
}

void
mark_roots (void)
{
  for (unsigned long i = 0; i < ARRAYELTS (lispsym); i++)
    mark_object (builtin_lisp_symbol (i));
  for (int i = 0; i < staticidx; i++)
    mark_object (*staticvec[i]);
}

#define GC_POINTER_ALIGNMENT alignof (void *)
void
mark_c_stack (void)
{
  if (!stack_top)
    TODO;
  const void *start = stack_top;
  const void *end = &start;

  char const *pp;

  if (end < start)
    {
      void const *tem = start;
      start = end;
      end = tem;
    }

  eassert (((uintptr_t) start) % GC_POINTER_ALIGNMENT == 0);

  for (pp = start; (void const *) pp < end; pp += GC_POINTER_ALIGNMENT)
    {
      void *p = *(void *const *) pp;
      mark_maybe_pointer (p, false);

      intptr_t ip;
      ckd_add (&ip, (intptr_t) p, (intptr_t) lispsym);
      mark_maybe_pointer ((void *) ip, true);
    }
}

static void
mark_lua (void)
{
  lua_State *L = global_lua_state;
  lcheckstack (L, 10);
  lua_getfield (L, LUA_ENVIRONINDEX, "memtbl");
  eassert (lua_istable (L, -1));
  // (-1)memtbl
  lua_pushnil (L);
  // (-2)memtbl, (-1)nil
  while (lua_next (L, -2) != 0)
    {
      eassert (lua_isuserdata (L, -1) && !lua_islightuserdata (L, -1));
      eassert (lua_isstring (L, -2));
      // (-3)memtbl, (-2)key, (-1)value
      Lisp_Object obj = *(Lisp_Object *) lua_touserdata (L, -1);
      eassert (!FIXNUMP (obj));
      mark_object (obj);
      lua_pop (L, 1);
    };
}

NO_INLINE static void
sweep_conses (void)
{
  struct cons_block **cprev = &cons_block;
  int lim = cons_block_index;
  object_ct num_free = 0, num_used = 0;

  cons_free_list = 0;

  for (struct cons_block *cblk; (cblk = *cprev);)
    {
      int i = 0;
      int this_free = 0;
      int ilim = (lim + BITS_PER_BITS_WORD - 1) / BITS_PER_BITS_WORD;

      for (i = 0; i < ilim; i++)
        {
          if (cblk->gcmarkbits[i] == BITS_WORD_MAX)
            {
              cblk->gcmarkbits[i] = 0;
              num_used += BITS_PER_BITS_WORD;
            }
          else
            {
              int start, pos, stop;

              start = i * BITS_PER_BITS_WORD;
              stop = lim - start;
              if (stop > BITS_PER_BITS_WORD)
                stop = BITS_PER_BITS_WORD;
              stop += start;

              for (pos = start; pos < stop; pos++)
                {
                  struct Lisp_Cons *acons = &cblk->conses[pos];
                  if (!XCONS_MARKED_P (acons))
                    {
                      ASAN_UNPOISON_CONS (&cblk->conses[pos]);
                      this_free++;
                      cblk->conses[pos].u.s.u.chain = cons_free_list;
                      cons_free_list = &cblk->conses[pos];
                      cons_free_list->u.s.car = dead_object ();
                      ASAN_POISON_CONS (&cblk->conses[pos]);
                    }
                  else
                    {
                      num_used++;
                      XUNMARK_CONS (acons);
                    }
                }
            }
        }

      lim = CONS_BLOCK_SIZE;
      if (this_free == CONS_BLOCK_SIZE && num_free > (long) CONS_BLOCK_SIZE)
        {
          *cprev = cblk->next;
          ASAN_UNPOISON_CONS (&cblk->conses[0]);
          cons_free_list = cblk->conses[0].u.s.u.chain;
          lisp_align_free (cblk);
        }
      else
        {
          num_free += this_free;
          cprev = &cblk->next;
        }
    }
  gcstat.total_conses = num_used;
  gcstat.total_free_conses = num_free;
}

NO_INLINE static void
sweep_floats (void)
{
  struct float_block **fprev = &float_block;
  int lim = float_block_index;
  object_ct num_free = 0, num_used = 0;

  float_free_list = 0;

  for (struct float_block *fblk; (fblk = *fprev);)
    {
      int this_free = 0;
      ASAN_UNPOISON_FLOAT_BLOCK (fblk);
      for (int i = 0; i < lim; i++)
        {
          struct Lisp_Float *afloat = &fblk->floats[i];
          if (!XFLOAT_MARKED_P (afloat))
            {
              this_free++;
              fblk->floats[i].u.chain = float_free_list;
              ASAN_POISON_FLOAT (&fblk->floats[i]);
              float_free_list = &fblk->floats[i];
            }
          else
            {
              num_used++;
              XFLOAT_UNMARK (afloat);
            }
        }
      lim = FLOAT_BLOCK_SIZE;
      if (this_free == FLOAT_BLOCK_SIZE && num_free > (long) FLOAT_BLOCK_SIZE)
        {
          *fprev = fblk->next;
          ASAN_UNPOISON_FLOAT (&fblk->floats[0]);
          float_free_list = fblk->floats[0].u.chain;
          lisp_align_free (fblk);
        }
      else
        {
          num_free += this_free;
          fprev = &fblk->next;
        }
    }
  gcstat.total_floats = num_used;
  gcstat.total_free_floats = num_free;
}

NO_INLINE static void
sweep_symbols (void)
{
  struct symbol_block *sblk;
  struct symbol_block **sprev = &symbol_block;
  int lim = symbol_block_index;
  object_ct num_free = 0, num_used = ARRAYELTS (lispsym);

  symbol_free_list = NULL;

  for (unsigned long i = 0; i < ARRAYELTS (lispsym); i++)
    lispsym[i].u.s.gcmarkbit = 0;

  for (sblk = symbol_block; sblk; sblk = *sprev)
    {
      ASAN_UNPOISON_SYMBOL_BLOCK (sblk);

      int this_free = 0;
      struct Lisp_Symbol *sym = sblk->symbols;
      struct Lisp_Symbol *end = sym + lim;

      for (; sym < end; ++sym)
        {
          if (!sym->u.s.gcmarkbit)
            {
              if (sym->u.s.redirect == SYMBOL_LOCALIZED)
                {
                  TODO;
                }
              sym->u.s.next = symbol_free_list;
              symbol_free_list = sym;
              symbol_free_list->u.s.function = dead_object ();
              ASAN_POISON_SYMBOL (sym);
              ++this_free;
            }
          else
            {
              ++num_used;
              sym->u.s.gcmarkbit = 0;
#if TODO_NELISP_LATER_AND
              eassert (valid_lisp_object_p (sym->u.s.function));
#endif
            }
        }

      lim = SYMBOL_BLOCK_SIZE;
      if (this_free == SYMBOL_BLOCK_SIZE && num_free > (long) SYMBOL_BLOCK_SIZE)
        {
          *sprev = sblk->next;
          ASAN_UNPOISON_SYMBOL (&sblk->symbols[0]);
          symbol_free_list = sblk->symbols[0].u.s.next;
          lisp_free (sblk);
        }
      else
        {
          num_free += this_free;
          sprev = &sblk->next;
        }
    }
  gcstat.total_symbols = num_used;
  gcstat.total_free_symbols = num_free;
}

static void
gc_sweep (void)
{
  sweep_strings ();
  sweep_conses ();
  sweep_floats ();
  sweep_symbols ();
  sweep_vectors ();
}

extern void mark_lread (void);
extern void mark_specpdl (void);
void
garbage_collect (void)
{
  TODO_NELISP_LATER;
  eassert (mark_stack_empty_p ());
  mark_roots ();
  mark_c_stack ();
  mark_lua ();
  mark_lread ();
  mark_specpdl ();
  eassert (mark_stack_empty_p ());
  gc_sweep ();
}

static void
init_alloc_once_for_pdumper (void)
{
  TODO_NELISP_LATER;
  purebeg = PUREBEG;
  pure_size = PURESIZE;
  mem_init ();
}
void
init_alloc_once (void)
{
  TODO_NELISP_LATER;
  init_alloc_once_for_pdumper ();
  init_strings ();
  init_vectors ();
}

void
syms_of_alloc (void)
{
  DEFVAR_INT ("cons-cells-consed", cons_cells_consed,
                doc: /* Number of cons cells that have been consed so far.  */);
  DEFVAR_INT ("floats-consed", floats_consed,
                doc: /* Number of floats that have been consed so far.  */);
  DEFVAR_INT ("vector-cells-consed", vector_cells_consed,
                doc: /* Number of vector cells that have been consed so far.  */);
  DEFVAR_INT ("symbols-consed", symbols_consed,
                doc: /* Number of symbols that have been consed so far.  */);
  symbols_consed += ARRAYELTS (lispsym);
  DEFVAR_INT ("string-chars-consed", string_chars_consed,
                doc: /* Number of string characters that have been consed so far.  */);
  DEFVAR_INT ("strings-consed", strings_consed,
                doc: /* Number of strings that have been consed so far.  */);

  defsubr (&Scons);
  defsubr (&Smake_vector);
  defsubr (&Smake_symbol);
}
