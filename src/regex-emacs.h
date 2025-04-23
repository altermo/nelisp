#ifndef EMACS_REGEX_H
#define EMACS_REGEX_H 1

#include "lisp.h"

extern Lisp_Object re_match_object;

struct re_registers
{
  ptrdiff_t num_regs;
  ptrdiff_t *start;
  ptrdiff_t *end;
};

struct re_pattern_buffer
{
  unsigned char *buffer;

  ptrdiff_t allocated;

  ptrdiff_t used;

  int charset_unibyte;

  char *fastmap;

  Lisp_Object translate;

  ptrdiff_t re_nsub;

  bool_bf can_be_null : 1;

  unsigned regs_allocated : 2;

  bool_bf fastmap_accurate : 1;

  bool_bf used_syntax : 1;

  bool_bf multibyte : 1;

  bool_bf target_multibyte : 1;
};

typedef enum
{
  RECC_ERROR = 0,
  RECC_ALNUM,
  RECC_ALPHA,
  RECC_WORD,
  RECC_GRAPH,
  RECC_PRINT,
  RECC_LOWER,
  RECC_UPPER,
  RECC_PUNCT,
  RECC_CNTRL,
  RECC_DIGIT,
  RECC_XDIGIT,
  RECC_BLANK,
  RECC_SPACE,
  RECC_MULTIBYTE,
  RECC_NONASCII,
  RECC_ASCII,
  RECC_UNIBYTE,
  RECC_NUM_CLASSES = RECC_UNIBYTE
} re_wctype_t;

extern const char *re_compile_pattern (const char *pattern, ptrdiff_t length,
                                       bool posix_backtracking,
                                       const char *whitespace_regexp,
                                       struct re_pattern_buffer *buffer);

extern ptrdiff_t re_search (struct re_pattern_buffer *buffer,
                            const char *string, ptrdiff_t length,
                            ptrdiff_t start, ptrdiff_t range,
                            struct re_registers *regs);

extern ptrdiff_t re_search_2 (struct re_pattern_buffer *buffer,
                              const char *string1, ptrdiff_t length1,
                              const char *string2, ptrdiff_t length2,
                              ptrdiff_t start, ptrdiff_t range,
                              struct re_registers *regs, ptrdiff_t stop);

extern void re_set_registers (struct re_pattern_buffer *buffer,
                              struct re_registers *regs, ptrdiff_t num_regs,
                              ptrdiff_t *starts, ptrdiff_t *ends);

#endif
