
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int writefile(int fd, void *buf, size_t len) {

  size_t total_remaining;
  size_t remaining;
  unsigned char *adv_p;
  ssize_t bytes_written;

  size_t chunksz = 4096;
  
  adv_p = buf;
  
  total_remaining = len;
  while (total_remaining > 0) {

    remaining = total_remaining;
    if (remaining > chunksz) remaining = chunksz;

    bytes_written = write(fd, adv_p, remaining);
    if (!bytes_written) break;
    if (bytes_written < 0) {
      return -1;
    }
    remaining -= bytes_written;
    total_remaining -= bytes_written;
    adv_p += bytes_written;
  }

  return (adv_p - ((unsigned char*) buf));
  
}
