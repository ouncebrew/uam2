#pragma once

#define RTLD_LAZY   0x0001
#define RTLD_NOW    0x0002
#define RTLD_LOCAL  0x0000
#define RTLD_GLOBAL 0x0100

static inline void* dlopen(const char* file, int mode) { (void)file; (void)mode; return 0; }
static inline void* dlsym(void* handle, const char* name) { (void)handle; (void)name; return 0; }
static inline int dlclose(void* handle) { (void)handle; return 0; }
static inline char* dlerror(void) { return (char*)"dynamic loading is not supported (yet)"; }
