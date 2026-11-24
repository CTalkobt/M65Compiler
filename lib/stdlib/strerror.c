/* strerror.c — Return string description of error number */

#include <errno.h>

char *strerror(int errnum) {
    switch (errnum) {
        case 0:      return "Success";
        case ERANGE: return "Result out of range";
        case ENOMEM: return "Out of memory";
        case EINVAL: return "Invalid argument";
        case EDOM:   return "Math domain error";
        case ESPIPE: return "Illegal seek";
        case ENOENT: return "No such file or directory";
        case EEXIST: return "File exists";
        case EACCES: return "Permission denied";
        case EIO:    return "Input/output error";
        default:     return "Unknown error";
    }
}
