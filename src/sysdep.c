#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define O_BINARY 0
#define O_TEXT 0

#include "lisp.h"

#define POSIX_CLOSE_RESTART 1
static int
posix_close (int fd, int flag)
{
  eassert (flag == POSIX_CLOSE_RESTART);
  return close (fd) == 0 || errno == EINTR ? 0 : -1;
}

int
emacs_close (int fd)
{
  int r;

  while (1)
    {
      r = posix_close (fd, POSIX_CLOSE_RESTART);

      if (r == 0)
        return r;
      if (!POSIX_CLOSE_RESTART || errno != EINTR)
        {
          eassert (errno != EBADF || fd < 0);
          return errno == EINPROGRESS ? 0 : r;
        }
    }
}

int
emacs_openat (int dirfd, char const *file, int oflags, int mode)
{
  int fd;
  if (!(oflags & O_TEXT))
    oflags |= O_BINARY;
  oflags |= O_CLOEXEC;
  while ((fd = openat (dirfd, file, oflags, mode)) < 0 && errno == EINTR)
    maybe_quit ();
  return fd;
}

int
emacs_open (char const *file, int oflags, int mode)
{
  return emacs_openat (AT_FDCWD, file, oflags, mode);
}

FILE *
emacs_fdopen (int fd, const char *mode)
{
  return fdopen (fd, mode);
}
