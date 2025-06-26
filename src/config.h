// TODO: this file will be generated

#ifndef EMACS_CONFIG_H
#define EMACS_CONFIG_H

#define _GL_CONFIG_H_INCLUDED 1

#define DIRECTORY_SEP '/'
#define IS_DIRECTORY_SEP(_c_) ((_c_) == DIRECTORY_SEP)
#define IS_ANY_SEP(_c_) (IS_DIRECTORY_SEP (_c_))
#define IS_DEVICE_SEP(_c_) 0
#define PACKAGE_VERSION "30.1"
#if defined __linux__
# define SYSTEM_TYPE "gnu/linux"
#else
# error "TODO: Unknown system type"
#endif

#define _GL_INLINE_HEADER_BEGIN
#define _GL_INLINE_HEADER_END
#define _GL_INLINE static inline __attribute__ ((__gnu_inline__))

#include "conf_post.h"

#endif
