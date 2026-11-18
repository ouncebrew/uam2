#pragma once
// Mesa's compiler never maps memory, but util references these still.
#include <stddef.h>
#include <sys/types.h>

#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20
#define MAP_ANON      MAP_ANONYMOUS
#define MAP_FAILED    ((void*)-1)

static inline void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	(void)addr; (void)length; (void)prot; (void)flags; (void)fd; (void)offset;
	return MAP_FAILED;
}
static inline int munmap(void* addr, size_t length) { (void)addr; (void)length; return -1; }
static inline int mprotect(void* addr, size_t length, int prot) { (void)addr; (void)length; (void)prot; return -1; }
