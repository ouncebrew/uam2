#include <xf86drm.h>
#include <errno.h>

int drmCommandWrite(int fd, unsigned long drmCommandIndex, void *data, unsigned long size)
{
    (void)fd; (void)drmCommandIndex; (void)data; (void)size;
    errno = 38;
    return -1;
}

int drmCommandRead(int fd, unsigned long drmCommandIndex, void *data, unsigned long size)
{
    (void)fd; (void)drmCommandIndex; (void)data; (void)size;
    errno = 38;
    return -1;
}

int drmCommandWriteRead(int fd, unsigned long drmCommandIndex, void *data, unsigned long size)
{
    (void)fd; (void)drmCommandIndex; (void)data; (void)size;
    errno = 38;
    return -1;
}

int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd)
{
    (void)fd; (void)handle; (void)flags; (void)prime_fd;
    errno = 38;
    return -1;
}

int drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle)
{
    (void)fd; (void)prime_fd; (void)handle;
    errno = 38;
    return -1;
}

int drmCloseBufferHandle(int fd, uint32_t handle)
{
    (void)fd; (void)handle;
    errno = 38;
    return -1;
}

drmVersionPtr drmGetVersion(int fd)
{
    (void)fd;
    return 0;
}

void drmFreeVersion(drmVersionPtr v)
{
    (void)v;
}

int drmGetDevices(drmDevicePtr devices[], int max_devices)
{
    (void)devices; (void)max_devices;
    return 0;
}

void drmFreeDevices(drmDevicePtr devices[], int count)
{
    (void)devices; (void)count;
}

int drmIoctl(int fd, unsigned long request, void *arg)
{
    (void)fd; (void)request; (void)arg;
    errno = 38;
    return -1;
}

int drmSyncobjCreate(int fd, uint32_t flags, uint32_t *handle)
{
    (void)fd; (void)flags; (void)handle;
    errno = 38;
    return -1;
}

int drmSyncobjDestroy(int fd, uint32_t handle)
{
    (void)fd; (void)handle;
    errno = 38;
    return -1;
}

int drmSyncobjTimelineWait(int fd, uint32_t *handles, uint64_t *points,
                           unsigned num_handles,
                           int64_t timeout_nsec, unsigned flags,
                           uint32_t *first_signaled)
{
    (void)fd; (void)handles; (void)points; (void)num_handles;
    (void)timeout_nsec; (void)flags; (void)first_signaled;
    errno = 38;
    return -1;
}
