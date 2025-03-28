#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>

struct bc_thread_state
{
  struct bc_frame *fp;
  char *stack;
  char *stack_end;
};

extern union specbinding *specpdl_ptr;
extern union specbinding *specpdl_end;
extern union specbinding *specpdl;

extern intmax_t lisp_eval_depth;
extern struct bc_thread_state bc_;

extern struct handler *handlerlist;
extern struct handler *handlerlist_sentinel;

#endif
