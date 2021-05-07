#include <errno.h>
#include <sys/types.h>

void _exit(void) {
  while (1) __asm__("hlt");
}

caddr_t sbrk(int incr) {
  errno = ENOMEM;
  return (caddr_t)-1;
}

int getpid(void) { return 1; }

int kill(int pid, int sig) {
  errno = EINVAL;
  return -1;
}

int isatty(int fd) { return 0; }
int close(int fd) { return 0; }
off_t lseek(int fd, off_t offset, int whence) { return 0; }

ssize_t read(int fd, void *buf, size_t count) { return 0; }

ssize_t write(int fd, const void *buf, size_t count) { return 0; }
int fstat(int fd, struct stat *buf) { return 0; }

int *__errno_location() { return 0; }
