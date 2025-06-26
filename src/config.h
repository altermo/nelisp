// TODO: this file will be generated

#ifndef EMACS_CONFIG_H
#define EMACS_CONFIG_H

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

#include "conf_post.h"

#endif
