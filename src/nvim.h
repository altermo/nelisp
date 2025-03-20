#ifndef NVIM_H
#define NVIM_H

#include "lisp.h"

extern Lisp_Object nvim_name_to_bufobj (Lisp_Object);
extern Lisp_Object nvim_create_buf (Lisp_Object, Lisp_Object);

#endif
