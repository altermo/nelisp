#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>

extern union specbinding *specpdl_ptr;
extern union specbinding *specpdl_end;
extern union specbinding *specpdl;

extern intmax_t lisp_eval_depth;

extern struct handler *handlerlist;
extern struct handler *handlerlist_sentinel;

#endif
