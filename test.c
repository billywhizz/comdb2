#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/*
truncate -s 16M 16.img
sudo losetup -f --show -b 4096 16.img 
sudo mkfs.xfs -q -f /dev/loop2
mkdir mnt16
sudo mount /dev/loop2 mnt16/
*/

int main (int argc, char** argv) {
  int read_size = 4096;
  if (argc > 1) read_size = atoi(argv[1]);
  int fd = open("foo.dat", O_CREAT | O_TRUNC | O_RDWR | O_DIRECT, 0666);
  fprintf(stderr, "open %i\n", fd);
  void* buf;
  int rc = posix_memalign(&buf, 4096, 4096);
  fprintf(stderr, "posix_memalign %i\n", rc);
  rc = pwrite(fd, buf, 4096, 0);
  fprintf(stderr, "pwrite %i\n", rc);
  rc = pread(fd, buf, read_size, 0);
  fprintf(stderr, "pread %i\n", rc);
  fprintf(stderr, "errno %i %s\n", errno, strerror(errno));
  return 0;
}
