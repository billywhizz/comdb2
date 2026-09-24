#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
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

/* STATX_DIOALIGN was added in Linux 6.1 */
static int kernel_has_dioalign(void)
{
    struct utsname u;
    int major, minor;

    if (uname(&u) != 0 || sscanf(u.release, "%d.%d", &major, &minor) != 2)
        return -1;
    return major > 6 || (major == 6 && minor >= 1);
}

/*
 * Ask the block device itself for its logical sector size.  udev maintains
 * /dev/block/MAJ:MIN symlinks, so no procfs or sysfs lookup is needed.
 * Needs read access to the device node (root or the "disk" group).
 * Returns 0 on success, otherwise an errno value.
 */
static int device_sector_size(uint32_t major, uint32_t minor, char *dev,
                              size_t devlen, int *size)
{
    int fd, err = 0;

    snprintf(dev, devlen, "/dev/block/%u:%u", major, minor);
    fd = open(dev, O_RDONLY);
    if (fd < 0)
        return errno;
    if (ioctl(fd, BLKSSZGET, size) != 0)
        err = errno;
    close(fd);
    return err;
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : ".";
    const char *why;
    struct kstatx stx;
    char dev[64];
    int size, err;

    if (syscall(SYS_statx, AT_FDCWD, path, 0, STATX_DIOALIGN, &stx) != 0) {
        perror(path);
        return 1;
    }
    if (stx.stx_mask & STATX_DIOALIGN) {
        printf("%u\n", stx.stx_dio_offset_align);
        return 0;
    }

    /* statx didn't report it: fall back to the block device ioctl */
    err = device_sector_size(stx.stx_dev_major, stx.stx_dev_minor,
                             dev, sizeof dev, &size);
    if (err == 0) {
        printf("%d\n", size);
        return 0;
    }

    switch (kernel_has_dioalign()) {
    case 1:
        why = "not a regular file, or the filesystem doesn't support direct I/O";
        break;
    case 0:
        why = "kernel too old, needs 6.1+";
        break;
    default:
        why = "unknown kernel version";
        break;
    }
    fprintf(stderr, "%s: statx: not reported (%s)\n", path, why);
    fprintf(stderr, "%s: ioctl fallback: %s: %s\n", path, dev, strerror(err));
    return 1;
}
