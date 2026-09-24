#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#define STATX_DIOALIGN 0x2000U

/* Mirror of the kernel's struct statx (include/uapi/linux/stat.h, Linux 6.8) */
struct kstatx_timestamp {
    int64_t  tv_sec;
    uint32_t tv_nsec;
    int32_t  __reserved;
};

struct kstatx {
    uint32_t stx_mask;
    uint32_t stx_blksize;
    uint64_t stx_attributes;
    uint32_t stx_nlink;
    uint32_t stx_uid;
    uint32_t stx_gid;
    uint16_t stx_mode;
    uint16_t __spare0[1];
    uint64_t stx_ino;
    uint64_t stx_size;
    uint64_t stx_blocks;
    uint64_t stx_attributes_mask;
    struct kstatx_timestamp stx_atime;
    struct kstatx_timestamp stx_btime;
    struct kstatx_timestamp stx_ctime;
    struct kstatx_timestamp stx_mtime;
    uint32_t stx_rdev_major;
    uint32_t stx_rdev_minor;
    uint32_t stx_dev_major;
    uint32_t stx_dev_minor;
    uint64_t stx_mnt_id;
    uint32_t stx_dio_mem_align;
    uint32_t stx_dio_offset_align;
    uint64_t __spare3[12];
};

_Static_assert(sizeof(struct kstatx) == 256, "struct statx must be 256 bytes");

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : ".";
    struct kstatx stx;

    if (syscall(SYS_statx, AT_FDCWD, path, 0, STATX_DIOALIGN, &stx) != 0) {
        perror(path);
        return 1;
    }
    if (!(stx.stx_mask & STATX_DIOALIGN)) {
        fprintf(stderr, "%s: not reported (old kernel or unsupported fs)\n", path);
        return 1;
    }
    printf("%u\n", stx.stx_dio_offset_align);
    return 0;
}
