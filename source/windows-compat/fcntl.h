#pragma once
/* fcntl.h wrapper for Windows builds of uam2.
 * Provides O_CLOEXEC which is missing on Windows UCRT.
 * uam2 never opens DRM devices (offline compiler), value unused. */
#include_next <fcntl.h>
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
