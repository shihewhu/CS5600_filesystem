/*
 * file:        image.c
 * description: emulated block device for CS 7600 / CS 5600 Homework 3
 *
 * Peter Desnoyers, Northeastern Computer Science, 2011
 */

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "blkdev.h"

struct image_dev {
    char *path;
    int   fd;
    int   nblks;
};

/* The blkdev operations - num_blocks, read, write
 */
static int image_num_blocks(struct blkdev *dev)
{
    struct image_dev *im = dev->private;
    return im->nblks;
}

static void image_read(struct blkdev *dev, int offset, int len, void *buf)
{
    struct image_dev *im = dev->private;
    assert(offset >= 0 && offset+len <= im->nblks);
    int result = pread(im->fd, buf, len*BLOCK_SIZE, offset*BLOCK_SIZE);

    if (result < 0) {           /* shouldn't happen */
        fprintf(stderr, "read error on %s: %s\n", im->path, strerror(errno));
        assert(0);
    }
    if (result != len*BLOCK_SIZE) { /* shouldn't happen */
        fprintf(stderr, "short read on %s: %s\n", im->path, strerror(errno));
        assert(0);
    }
}

static void image_write(struct blkdev * dev, int offset, int len, void *buf)
{
    struct image_dev *im = dev->private;
    assert(offset != 0);        /* over-writing the superblock is an error */
    assert(offset >= 0 && offset+len <= im->nblks);

    int result = pwrite(im->fd, buf, len*BLOCK_SIZE, offset*BLOCK_SIZE);
    if (result != len*BLOCK_SIZE) { /* shouldn't happen */
        fprintf(stderr, "write error on %s: %s\n", im->path, strerror(errno));
        assert(0);
    }
}

struct blkdev_ops image_ops = {
    .num_blocks = image_num_blocks,
    .read = image_read,
    .write = image_write,
};

/* create an image blkdev reading from a specified image file.
 */
struct blkdev *image_create(char *path)
{
    struct blkdev *dev = malloc(sizeof(*dev));
    struct image_dev *im = malloc(sizeof(*im));

    assert(dev != NULL && im != NULL);
    im->path = strdup(path);    /* save a copy for error reporting */

    im->fd = open(path, O_RDWR);
    if (im->fd < 0) {
        fprintf(stderr, "can't open image %s: %s\n", path, strerror(errno));
        assert(0);
    }
    struct stat sb;
    if (fstat(im->fd, &sb) < 0) {
        fprintf(stderr, "can't access image %s: %s\n", path, strerror(errno));
        assert(0);
    }

    /* print a warning if file is not a multiple of the block size -
     * this isn't a fatal error, as extra bytes beyond the last full
     * block will be ignored by read and write.
     */
    if (sb.st_size % BLOCK_SIZE != 0)
        fprintf(stderr, "warning: file %s not a multiple of %d bytes\n",
                path, BLOCK_SIZE);

    im->nblks = sb.st_size / BLOCK_SIZE;
    dev->private = im;
    dev->ops = &image_ops;

    return dev;
}

