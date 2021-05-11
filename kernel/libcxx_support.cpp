#include <cerrno>
#include <new>

extern "C" int posix_memalign(void**, size_t, size_t) { return ENOMEM; }
