#pragma once
#include <stdint.h>

#define _IOC_NRBITS 8
#define _IOC_TYPEBITS 8
#define _IOC_SIZEBITS 14
#define _IOC_DIRBITS 2

#define _IOC_NRMASK ((1 << _IOC_NRBITS) - 1)
#define _IOC_TYPEMASK ((1 << _IOC_TYPEBITS) - 1)
#define _IOC_SIZEMASK ((1 << _IOC_SIZEBITS) - 1)
#define _IOC_DIRMASK ((1 << _IOC_DIRBITS) - 1)

#define _IOC_NONE 0U
#define _IOC_WRITE 1U
#define _IOC_READ 2U

#define _IOC_NRSHIFT 0
#define _IOC_TYPESHIFT (_IOC_NRSHIFT + _IOC_NRBITS)
#define _IOC_SIZESHIFT (_IOC_TYPESHIFT + _IOC_TYPEBITS)
#define _IOC_DIRSHIFT (_IOC_SIZESHIFT + _IOC_SIZEBITS)

#define _IOC(dir, group, nr, size) \
    (((dir) << _IOC_DIRSHIFT) | \
     ((group) << _IOC_TYPESHIFT) | \
     ((nr) << _IOC_NRSHIFT) | \
     ((size) << _IOC_SIZESHIFT))

#define _IO(g, n) _IOC(_IOC_NONE, (g), (n), 0)
#define _IOR(g, n, t) _IOC(_IOC_READ, (g), (n), sizeof(t))
#define _IOW(g, n, t) _IOC(_IOC_WRITE, (g), (n), sizeof(t))
#define _IOWR(g, n, t) _IOC(_IOC_READ | _IOC_WRITE, (g), (n), sizeof(t))
