#include "lisp.h"

union specbinding *specpdl_ptr;
union specbinding *specpdl_end;
union specbinding *specpdl;

intmax_t lisp_eval_depth;

struct handler *handlerlist;
struct handler *handlerlist_sentinel;
