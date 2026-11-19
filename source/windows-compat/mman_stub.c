#include <sys/mman.h>
#include <errno.h>

void *mmap(void *addr, size_t length, int prot, int flags, int fd, int64_t offset)
{
    (void)addr; (void)length; (void)prot; (void)flags; (void)fd; (void)offset;
    errno = 38; /* ENOSYS */
    return MAP_FAILED;
}

int munmap(void *addr, size_t length)
{
    (void)addr; (void)length;
    errno = 38;
    return -1;
}
