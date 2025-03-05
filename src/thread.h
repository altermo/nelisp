#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>

union specbinding *specpdl_ptr;
union specbinding *specpdl_end;
union specbinding *specpdl;

intmax_t lisp_eval_depth;

#endif
