#include <errno.h>
#include <fcntl.h>
#include <filename.h>
#include <unistd.h>

#include "lisp.h"
#include "buffer.h"
#include "coding.h"

typedef int emacs_fd;

#define emacs_fd_open emacs_open
#define emacs_fd_close emacs_close
#define emacs_fd_read emacs_read_quit
#define emacs_fd_lseek lseek
#define emacs_fd_fstat sys_fstat
#define emacs_fd_valid_p(fd) ((fd) >= 0)

static void
close_file_unwind_emacs_fd (void *ptr)
{
  emacs_fd *fd;

  fd = ptr;
  emacs_fd_close (*fd);
}

void
fclose_unwind (void *arg)
{
  FILE *stream = arg;
  emacs_fclose (stream);
}

enum
{
  file_name_as_directory_slop = 2
};

static ptrdiff_t
file_name_as_directory (char *dst, const char *src, ptrdiff_t srclen,
                        bool multibyte)
{
  UNUSED (multibyte);
  if (srclen == 0)
    {
      dst[0] = '.';
      dst[1] = '/';
      dst[2] = '\0';
      return 2;
    }

  memcpy (dst, src, srclen);
  if (!IS_DIRECTORY_SEP (dst[srclen - 1]))
    dst[srclen++] = DIRECTORY_SEP;
  dst[srclen] = 0;
  return srclen;
}

Lisp_Object
file_name_directory (Lisp_Object filename)
{
  char *beg = SSDATA (filename);
  char const *p = beg + SBYTES (filename);

  while (p != beg && !IS_DIRECTORY_SEP (p[-1]))
    p--;

  if (p == beg)
    return Qnil;
  return make_specified_string (beg, -1, p - beg, STRING_MULTIBYTE (filename));
}

DEFUN ("file-name-directory", Ffile_name_directory, Sfile_name_directory,
       1, 1, 0,
       doc: /* Return the directory component in file name FILENAME.
Return nil if FILENAME does not include a directory.
Otherwise return a directory name.
Given a Unix syntax file name, returns a string ending in slash.  */)
(Lisp_Object filename)
{
  Lisp_Object handler;

  CHECK_STRING (filename);

#if TODO_NELISP_LATER_AND
  handler = Ffind_file_name_handler (filename, Qfile_name_directory);
  if (!NILP (handler))
    {
      Lisp_Object handled_name
        = call2 (handler, Qfile_name_directory, filename);
      return STRINGP (handled_name) ? handled_name : Qnil;
    }
#endif

  return file_name_directory (filename);
}

char const *
get_homedir (void)
{
  char const *home = egetenv ("HOME");

  if (!home)
    TODO;
  if (IS_ABSOLUTE_FILE_NAME (home))
    return home;
  TODO;
}

DEFUN ("expand-file-name", Fexpand_file_name, Sexpand_file_name, 1, 2, 0,
       doc: /* Convert filename NAME to absolute, and canonicalize it.
Second arg DEFAULT-DIRECTORY is directory to start with if NAME is relative
\(does not start with slash or tilde); both the directory name and
a directory's file name are accepted.  If DEFAULT-DIRECTORY is nil or
missing, the current buffer's value of `default-directory' is used.
NAME should be a string that is a valid file name for the underlying
filesystem.

File name components that are `.' are removed, and so are file name
components followed by `..', along with the `..' itself; note that
these simplifications are done without checking the resulting file
names in the file system.

Multiple consecutive slashes are collapsed into a single slash, except
at the beginning of the file name when they are significant (e.g., UNC
file names on MS-Windows.)

An initial \"~\" in NAME expands to your home directory.

An initial \"~USER\" in NAME expands to USER's home directory.  If
USER doesn't exist, \"~USER\" is not expanded.

To do other file name substitutions, see `substitute-in-file-name'.

For technical reasons, this function can return correct but
non-intuitive results for the root directory; for instance,
\(expand-file-name ".." "/") returns "/..".  For this reason, use
\(directory-file-name (file-name-directory dirname)) to traverse a
filesystem tree, not (expand-file-name ".." dirname).  Note: make
sure DIRNAME in this example doesn't end in a slash, unless it's
the root directory.  */)
(Lisp_Object name, Lisp_Object default_directory)
{
  TODO_NELISP_LATER;

  ptrdiff_t length, nbytes;
  const char *newdir;
  const char *newdirlim;
  bool multibyte;
  char *nm;
  char *nmlim;
  ptrdiff_t tlen;
  char *target;
  Lisp_Object result;
  USE_SAFE_ALLOCA;

  CHECK_STRING (name);
  CHECK_STRING_NULL_BYTES (name);

  Lisp_Object root;
  root = build_string ("/");
  if (NILP (default_directory))
    TODO_NELISP_LATER;
  if (!STRINGP (default_directory))
    default_directory = root;
  {
    char *o = SSDATA (default_directory);
    if (!NILP (default_directory) && !EQ (default_directory, name)
        && !(IS_DIRECTORY_SEP (o[0])))
      {
        default_directory = Fexpand_file_name (default_directory, Qnil);
      }
  }
  multibyte = STRING_MULTIBYTE (name);
  bool defdir_multibyte = STRING_MULTIBYTE (default_directory);
  if (multibyte != defdir_multibyte)
    {
      if (multibyte)
        {
          bool name_ascii_p = SCHARS (name) == SBYTES (name);
          unsigned char *p = SDATA (default_directory);

          if (!name_ascii_p)
            while (*p && ASCII_CHAR_P (*p))
              p++;
          if (name_ascii_p || *p != '\0')
            {
              name = make_unibyte_string (SSDATA (name), SBYTES (name));
              multibyte = 0;
            }
          else
            {
              default_directory
                = make_multibyte_string (SSDATA (default_directory),
                                         SCHARS (default_directory),
                                         SCHARS (default_directory));
            }
        }
      else
        {
          unsigned char *p = SDATA (name);

          while (*p && ASCII_CHAR_P (*p))
            p++;
          if (*p == '\0')
            {
              name = make_multibyte_string (SSDATA (name), SCHARS (name),
                                            SCHARS (name));
              multibyte = 1;
            }
          else
            default_directory
              = make_unibyte_string (SSDATA (default_directory),
                                     SBYTES (default_directory));
        }
    }
  SAFE_ALLOCA_STRING (nm, name);
  nmlim = nm + SBYTES (name);

  if (IS_DIRECTORY_SEP (nm[0]))
    {
      bool lose = 0;
      char *p = nm;

      while (*p)
        {
          if (IS_DIRECTORY_SEP (p[0]) && p[1] == '.'
              && (IS_DIRECTORY_SEP (p[2]) || p[2] == 0
                  || (p[2] == '.' && (IS_DIRECTORY_SEP (p[3]) || p[3] == 0))))
            lose = 1;
          else if (IS_DIRECTORY_SEP (p[0]) && IS_DIRECTORY_SEP (p[1])
                   && (p != nm || IS_DIRECTORY_SEP (p[2])))
            lose = 1;
          p++;
        }
      if (!lose)
        {
          if (strcmp (nm, SSDATA (name)) != 0)
            name = make_specified_string (nm, -1, nmlim - nm, multibyte);
          SAFE_FREE ();
          return name;
        }
    }
  newdir = newdirlim = 0;
  if (nm[0] == '~')
    {
      if (IS_DIRECTORY_SEP (nm[1]) || nm[1] == 0)
        {
          Lisp_Object tem;

          newdir = get_homedir ();
          nm++;
          tem = build_string (newdir);
          newdirlim = newdir + SBYTES (tem);

          if (multibyte && !STRING_MULTIBYTE (tem))
            TODO;
          else if (!multibyte && STRING_MULTIBYTE (tem))
            multibyte = 1;
        }
      else
        TODO;
    }
  if (1 && !newdir)
    {
      newdir = SSDATA (default_directory);
      newdirlim = newdir + SBYTES (default_directory);
    }
  length = newdirlim - newdir;
  while (length > 1 && IS_DIRECTORY_SEP (newdir[length - 1])
         && !(length == 2 && IS_DIRECTORY_SEP (newdir[0])))
    length--;
  tlen = length + file_name_as_directory_slop + (nmlim - nm) + 1;
  eassert (tlen >= file_name_as_directory_slop + 1);
  target = SAFE_ALLOCA (tlen);
  *target = 0;
  nbytes = 0;
  if (newdir)
    {
      if (nm[0] == 0 || IS_DIRECTORY_SEP (nm[0]))
        {
          memcpy (target, newdir, length);
          target[length] = 0;
          nbytes = length;
        }
      else
        {
          nbytes = file_name_as_directory (target, newdir, length, multibyte);
        }
    }

  memcpy (target + nbytes, nm, nmlim - nm + 1);

  {
    char *p = target;
    char *o = target;

    while (*p)
      {
        if (!IS_DIRECTORY_SEP (*p))
          {
            *o++ = *p++;
          }
        else if (p[1] == '.' && (IS_DIRECTORY_SEP (p[2]) || p[2] == 0))
          {
            if (o == target && p[2] == '\0')
              *o++ = *p;
            p += 2;
          }
        else if (p[1] == '.' && p[2] == '.' && o != target
                 && (IS_DIRECTORY_SEP (p[3]) || p[3] == 0))
          {
            while (o != target && (--o, !IS_DIRECTORY_SEP (*o)))
              continue;
            if (o == target && IS_ANY_SEP (*o) && p[3] == 0)
              ++o;
            p += 3;
          }
        else if (IS_DIRECTORY_SEP (p[1])
                 && (p != target || IS_DIRECTORY_SEP (p[2])))
          p++;
        else
          {
            *o++ = *p++;
          }
      }

    result = make_specified_string (target, -1, o - target, multibyte);
  }
  SAFE_FREE ();
  return result;
}

Lisp_Object
expand_and_dir_to_file (Lisp_Object filename)
{
  Lisp_Object absname = Fexpand_file_name (filename, Qnil);

  if (SCHARS (absname) > 1
      && IS_DIRECTORY_SEP (SREF (absname, SBYTES (absname) - 1))
      && !IS_DEVICE_SEP (SREF (absname, SBYTES (absname) - 2)))
    TODO;
  return absname;
}

bool
file_access_p (char const *file, int amode)
{
  if (faccessat (AT_FDCWD, file, amode, AT_EACCESS) == 0)
    return true;
  return false;
}
bool
file_accessible_directory_p (Lisp_Object file)
{
  const char *data = SSDATA (file);
  ptrdiff_t len = SBYTES (file);
  char const *dir;
  bool ok;
  USE_SAFE_ALLOCA;

  if (!len)
    dir = data;
  else
    {
      static char const appended[] = "/./";
      char *buf = SAFE_ALLOCA (len + sizeof appended);
      memcpy (buf, data, len);
      strcpy (buf + len, &appended[data[len - 1] == '/']);
      dir = buf;
    }

  ok = file_access_p (dir, F_OK);
  SAFE_FREE ();
  return ok;
}
bool
file_directory_p (Lisp_Object file)
{
  if (file_accessible_directory_p (file))
    return true;
  if (errno != EACCES)
    return false;
  struct stat st;
  if (emacs_fstatat (AT_FDCWD, SSDATA (file), &st, 0) != 0)
    return errno == EOVERFLOW;
  if (S_ISDIR (st.st_mode))
    return true;
  errno = ENOTDIR;
  return false;
}

static Lisp_Object
check_file_access (Lisp_Object file, Lisp_Object operation, int amode)
{
  file = Fexpand_file_name (file, Qnil);
#if TODO_NELISP_LATER_AND
  Lisp_Object handler = Ffind_file_name_handler (file, operation);
  if (!NILP (handler))
    {
      Lisp_Object ok = call2 (handler, operation, file);

      errno = 0;
      return ok;
    }
#endif

  char *encoded_file = SSDATA (ENCODE_FILE (file));
  return file_access_p (encoded_file, amode) ? Qt : Qnil;
}

DEFUN ("file-exists-p", Ffile_exists_p, Sfile_exists_p, 1, 1, 0,
       doc: /* Return t if file FILENAME exists (whether or not you can read it).
Return nil if FILENAME does not exist, or if there was trouble
determining whether the file exists.
See also `file-readable-p' and `file-attributes'.
This returns nil for a symlink to a nonexistent file.
Use `file-symlink-p' to test for such links.  */)
(Lisp_Object filename)
{
  return check_file_access (filename, Qfile_exists_p, F_OK);
}

DEFUN ("file-directory-p", Ffile_directory_p, Sfile_directory_p, 1, 1, 0,
       doc: /* Return t if FILENAME names an existing directory.
Return nil if FILENAME does not name a directory, or if there
was trouble determining whether FILENAME is a directory.

As a special case, this function will also return t if FILENAME is the
empty string (\"\").  This quirk is due to Emacs interpreting the
empty string (in some cases) as the current directory.

Symbolic links to directories count as directories.
See `file-symlink-p' to distinguish symlinks.  */)
(Lisp_Object filename)
{
  Lisp_Object absname = expand_and_dir_to_file (filename);

#if TODO_NELISP_LATER_AND
  Lisp_Object handler = Ffind_file_name_handler (absname, Qfile_directory_p);
  if (!NILP (handler))
    return call2 (handler, Qfile_directory_p, absname);
#endif

  return file_directory_p (ENCODE_FILE (absname)) ? Qt : Qnil;
}

DEFUN ("car-less-than-car", Fcar_less_than_car, Scar_less_than_car, 2, 2, 0,
       doc: /* Return t if (car A) is numerically less than (car B).  */)
(Lisp_Object a, Lisp_Object b)
{
  Lisp_Object ca = Fcar (a), cb = Fcar (b);
  if (FIXNUMP (ca) && FIXNUMP (cb))
    return XFIXNUM (ca) < XFIXNUM (cb) ? Qt : Qnil;
  return arithcompare (ca, cb, ARITH_LESS);
}

static void
restore_point_unwind (void *location)
{
  SET_PT (*(ptrdiff_t *) location);
}

enum
{
  READ_BUF_SIZE = MAX_ALLOCA
};

DEFUN ("insert-file-contents", Finsert_file_contents, Sinsert_file_contents,
       1, 5, 0,
       doc: /* Insert contents of file FILENAME after point.
Returns list of absolute file name and number of characters inserted.
If second argument VISIT is non-nil, the buffer's visited filename and
last save file modtime are set, and it is marked unmodified.  If
visiting and the file does not exist, visiting is completed before the
error is signaled.

The optional third and fourth arguments BEG and END specify what portion
of the file to insert.  These arguments count bytes in the file, not
characters in the buffer.  If VISIT is non-nil, BEG and END must be nil.

When inserting data from a special file (e.g., /dev/urandom), you
can't specify VISIT or BEG, and END should be specified to avoid
inserting unlimited data into the buffer from some special files
which otherwise could supply infinite amounts of data.

If optional fifth argument REPLACE is non-nil and FILENAME names a
regular file, replace the current buffer contents (in the accessible
portion) with the file's contents.  This is better than simply
deleting and inserting the whole thing because (1) it preserves some
marker positions (in unchanged portions at the start and end of the
buffer) and (2) it puts less data in the undo list.  When REPLACE is
non-nil, the second element of the return value is the number of
characters that replace the previous buffer contents.

If FILENAME is not a regular file and REPLACE is `if-regular', erase
the accessible portion of the buffer and insert the new contents.  Any
other non-nil value of REPLACE will signal an error if FILENAME is not
a regular file.

This function does code conversion according to the value of
`coding-system-for-read' or `file-coding-system-alist', and sets the
variable `last-coding-system-used' to the coding system actually used.

In addition, this function decodes the inserted text from known formats
by calling `format-decode', which see.  */)
(Lisp_Object filename, Lisp_Object visit, Lisp_Object beg, Lisp_Object end,
 Lisp_Object replace)
{
  struct stat st;
  specpdl_ref count = SPECPDL_INDEX ();
  Lisp_Object val, orig_filename;
  ptrdiff_t inserted = 0;
  emacs_fd fd;
  ptrdiff_t total = 0;
  char read_buf[READ_BUF_SIZE];
  ptrdiff_t read_quit = 0;
  val = Qnil;

  TODO_NELISP_LATER;

  if (!NILP (visit) || !NILP (beg) || !NILP (end) || !NILP (replace))
    TODO;

#if TODO_NELISP_LATER_AND
  handler = Ffind_file_name_handler (filename, Qinsert_file_contents);
  if (!NILP (handler))
    {
      val = call6 (handler, Qinsert_file_contents, filename, visit, beg, end,
                   replace);
      if (CONSP (val) && CONSP (XCDR (val))
          && RANGED_FIXNUMP (0, XCAR (XCDR (val)), ZV - PT))
        inserted = XFIXNUM (XCAR (XCDR (val)));
      goto handled;
    }
#endif

  CHECK_STRING (filename);
  filename = Fexpand_file_name (filename, Qnil);

  orig_filename = filename;
  filename = ENCODE_FILE (filename);

  fd = emacs_fd_open (SSDATA (filename), O_RDONLY, 0);
  if (!emacs_fd_valid_p (fd))
    TODO;

  specpdl_ref fd_index = SPECPDL_INDEX ();
  record_unwind_protect_ptr (close_file_unwind_emacs_fd, &fd);

  if (emacs_fd_fstat (fd, &st) != 0)
    TODO;

  if (!(S_ISREG (st.st_mode) || S_ISDIR (st.st_mode)))
    TODO;

  total = st.st_size;
  if (total < 0)
    TODO;

  if (total == 0)
    total = READ_BUF_SIZE;

  ptrdiff_t point = PT;
  record_unwind_protect_ptr (restore_point_unwind, &point);

  while (inserted < total)
    {
      ptrdiff_t this;
      ptrdiff_t trytry = READ_BUF_SIZE;
      this = emacs_fd_read (fd, read_buf, READ_BUF_SIZE);
      insert (read_buf, this);
      if (this <= 0)
        {
          read_quit = this;
          break;
        }
      inserted += this;
    }

  if (read_quit < 0)
    TODO;

  if (NILP (val))
    val = list2 (orig_filename, make_fixnum (inserted));

  return unbind_to (count, val);
}

void
syms_of_fileio (void)
{
  DEFSYM (Qfile_exists_p, "file-exists-p");

  defsubr (&Sfile_name_directory);
  defsubr (&Sexpand_file_name);
  defsubr (&Sfile_exists_p);
  defsubr (&Sfile_directory_p);
  defsubr (&Scar_less_than_car);
  defsubr (&Sinsert_file_contents);

  DEFVAR_LISP ("file-name-handler-alist", Vfile_name_handler_alist,
        doc: /* Alist of elements (REGEXP . HANDLER) for file names handled specially.
If a file name matches REGEXP, all I/O on that file is done by calling
HANDLER.  If a file name matches more than one handler, the handler
whose match starts last in the file name gets precedence.  The
function `find-file-name-handler' checks this list for a handler for
its argument.

HANDLER should be a function.  The first argument given to it is the
name of the I/O primitive to be handled; the remaining arguments are
the arguments that were passed to that primitive.  For example, if you
do (file-exists-p FILENAME) and FILENAME is handled by HANDLER, then
HANDLER is called like this:

  (funcall HANDLER \\='file-exists-p FILENAME)

Note that HANDLER must be able to handle all I/O primitives; if it has
nothing special to do for a primitive, it should reinvoke the
primitive to handle the operation \"the usual way\".
See Info node `(elisp)Magic File Names' for more details.  */);
  Vfile_name_handler_alist = Qnil;
}
