#include "lisp.h"

enum { file_name_as_directory_slop = 2 };

static ptrdiff_t
file_name_as_directory (char *dst, const char *src, ptrdiff_t srclen,
                        bool multibyte)
{
    UNUSED(multibyte);
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
        TODO;
    if (! STRINGP (default_directory))
        default_directory = root;
    {
        char *o = SSDATA (default_directory);
        if (! NILP (default_directory) && !EQ (default_directory, name)
            && ! (IS_DIRECTORY_SEP (o[0])))
        {
            default_directory = Fexpand_file_name (default_directory, Qnil);
        }
    }
    multibyte = STRING_MULTIBYTE (name);
    bool defdir_multibyte = STRING_MULTIBYTE (default_directory);
    if (multibyte != defdir_multibyte)
        TODO;
    SAFE_ALLOCA_STRING (nm, name);
    nmlim = nm + SBYTES (name);

    if (IS_DIRECTORY_SEP (nm[0]))
        TODO;
    newdir = newdirlim = 0;
    if (nm[0] == '~')
        TODO;
    if (1 && !newdir)
    {
        newdir = SSDATA (default_directory);
        newdirlim = newdir + SBYTES (default_directory);
    }
    length = newdirlim - newdir;
    while (length > 1 && IS_DIRECTORY_SEP (newdir[length - 1])
        && ! (length == 2 && IS_DIRECTORY_SEP (newdir[0])))
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
        } else {
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
            else if (p[1] == '.'
                && (IS_DIRECTORY_SEP (p[2])
                || p[2] == 0))
            {
                if (o == target && p[2] == '\0')
                    *o++ = *p;
                p += 2;
            }
            else if (p[1] == '.' && p[2] == '.'
                && o != target
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
    SAFE_FREE();
    return result;
}

void syms_of_fileio(void){
    defsubr(&Sexpand_file_name);
}
