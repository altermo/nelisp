#include "lisp.h"
#include "regex-emacs.h"

union specbinding *specpdl_ptr;
union specbinding *specpdl_end;
union specbinding *specpdl;

intmax_t lisp_eval_depth;
struct bc_thread_state bc_;

struct handler *handlerlist;
struct handler *handlerlist_sentinel;

struct re_registers search_regs;

Lisp_Object last_thing_searched;
