#define ATTRIBUTE_FORMAT_PRINTF(string_index, first_to_check) \
  __attribute__ ((__format__ (__printf__, string_index, first_to_check)))
#ifdef lint
# define UNINIT \
   = {          \
     0,         \
   }
#else
# define UNINIT
#endif
#define INLINE EXTERN_INLINE
#define EXTERN_INLINE static inline __attribute__ ((unused))
#define NO_INLINE __attribute__ ((__noinline__))
#define FLEXIBLE_ARRAY_MEMBER /**/
#define FLEXALIGNOF(type) (sizeof (type) & ~(sizeof (type) - 1))
#define FLEXSIZEOF(type, member, n)                         \
  ((offsetof (type, member) + FLEXALIGNOF (type) - 1 + (n)) \
   & ~(FLEXALIGNOF (type) - 1))
#ifdef bool // TODO: lsp HACK
typedef bool bool_bf;
#endif
#define FALLTHROUGH __attribute__ ((fallthrough))
