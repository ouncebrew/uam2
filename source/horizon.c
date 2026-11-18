#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <switch/kernel/random.h>

long sysconf(int name)
{
	switch (name)
	{
		case _SC_PAGESIZE:
			return 0x1000;
		case _SC_NPROCESSORS_ONLN:
		case _SC_NPROCESSORS_CONF:
			return 1;
		default:
			errno = EINVAL;
			return -1;
	}
}

ssize_t getrandom(void* buf, size_t len, unsigned int flags)
{
	(void)flags;
	randomGet(buf, len);
	return len;
}
