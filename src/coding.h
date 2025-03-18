#ifndef EMACS_CODING_H
#define EMACS_CODING_H

#include "lisp.h"

INLINE Lisp_Object
encode_file_name (Lisp_Object name)
{
    if (STRING_MULTIBYTE(name))
        TODO;
    CHECK_STRING_NULL_BYTES(name);
    return name;
}

#define ENCODE_FILE(NAME)  encode_file_name (NAME)

#endif
