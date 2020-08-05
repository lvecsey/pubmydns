
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int readfile(int fd, void *p, size_t len) {

  unsigned char *adv_p = (unsigned char*) p;
  size_t remaining = len;
  ssize_t bytes_read;

  while (remaining > 0) {
    bytes_read = read(fd, adv_p + len - remaining, remaining);
    if (bytes_read <= 0) return -1;
    remaining -= bytes_read;
  }

  return (len - remaining);

}


