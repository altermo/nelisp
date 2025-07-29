#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define O_BINARY 0
#define O_TEXT 0

#include "lisp.h"

int
sys_fstat (int fd, struct stat *statb)
{
  return fstat (fd, statb);
}

int
sys_faccessat (int fd, const char *pathname, int mode, int flags)
{
  return faccessat (fd, pathname, mode, flags);
}

_Noreturn void
emacs_abort (void)
{
  TODO;
  __builtin_unreachable ();
}
int
emacs_fstatat (int dirfd, char const *filename, void *st, int flags)
{
  int r;
  while ((r = fstatat (dirfd, filename, st, flags)) != 0 && errno == EINTR)
    maybe_quit ();
  return r;
}

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

#ifndef MAX_RW_COUNT
# define MAX_RW_COUNT (INT_MAX >> 18 << 18)
#endif

static ptrdiff_t
emacs_intr_read (int fd, void *buf, ptrdiff_t nbyte, bool interruptible)
{
  eassert (nbyte <= MAX_RW_COUNT);

  ssize_t result;

  do
    {
      if (interruptible)
        maybe_quit ();
      result = read (fd, buf, nbyte);
    }
  while (result < 0 && errno == EINTR);

  return result;
}

ptrdiff_t
emacs_read_quit (int fd, void *buf, ptrdiff_t nbyte)
{
  return emacs_intr_read (fd, buf, nbyte, true);
}

#ifndef RAND_BITS
# ifdef HAVE_RANDOM
#  define RAND_BITS 31
# else
#  ifdef HAVE_LRAND48
#   define RAND_BITS 31
#   define random lrand48
#  else
#   define RAND_BITS 15
#   if RAND_MAX == 32767
#    define random rand
#   else
#    if RAND_MAX == 2147483647
#     define random() (rand () >> 16)
#    else
#     ifdef USG
#      define random rand
#     else
#      define random() (rand () >> 16)
#     endif
#    endif
#   endif
#  endif
# endif
#endif

EMACS_INT
get_random (void)
{
  EMACS_UINT val = 0;
  int i;
  for (i = 0; i < (FIXNUM_BITS + RAND_BITS - 1) / RAND_BITS; i++)
    val = (random () ^ (val << RAND_BITS)
           ^ (val >> (EMACS_INT_WIDTH - RAND_BITS)));
  val ^= val >> (EMACS_INT_WIDTH - FIXNUM_BITS);
  return val & INTMASK;
}

void
init_system_name (void)
{
  if (!build_details)
    {
      Vsystem_name = Qnil;
      return;
    }
  char *hostname_alloc = NULL;
  char *hostname;
  char hostname_buf[256];
  ptrdiff_t hostname_size = sizeof hostname_buf;
  hostname = hostname_buf;

  for (;;)
    {
      gethostname (hostname, hostname_size - 1);
      hostname[hostname_size - 1] = '\0';

      if (strlen (hostname) < (unsigned long) hostname_size - 1)
        break;

      hostname = hostname_alloc = xpalloc (hostname_alloc, &hostname_size, 1,
                                           min (PTRDIFF_MAX, SIZE_MAX), 1);
    }
  char *p;
  for (p = hostname; *p; p++)
    if (*p == ' ' || *p == '\t')
      *p = '-';
  if (!(STRINGP (Vsystem_name) && SBYTES (Vsystem_name) == p - hostname
        && strcmp (SSDATA (Vsystem_name), hostname) == 0))
    Vsystem_name = build_string (hostname);
  xfree (hostname_alloc);
}
