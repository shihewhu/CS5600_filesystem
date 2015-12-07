/*
 * file:        homework.c
 * description: skeleton file for CS 5600 homework 3
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2015
 */

#define FUSE_USE_VERSION 27
#define _GNU_SOURCE

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fuse.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "fs5600.h"
#include "blkdev.h"

/* 
 * disk access - the global variable 'disk' points to a blkdev
 * structure which has been initialized to access the image file.
 *
 * NOTE - blkdev access is in terms of 1024-byte blocks
 */
extern struct blkdev *disk;

/* by defining bitmaps as 'fd_set' pointers, you can use existing
 * macros to handle them. 
 *   FD_ISSET(##, inode_map);
 *   FD_CLR(##, block_map);
 *   FD_SET(##, block_map);
 */
fd_set *inode_map;              /* = malloc(sb.inode_map_size * FS_BLOCK_SIZE); */
fd_set *block_map;
struct fs5600_inode *inode_region;	/* inodes in memory */

/* init - this is called once by the FUSE framework at startup. Ignore
 * the 'conn' argument.
 * recommended actions:
 *   - read superblock
 *   - allocate memory, read bitmaps and inodes
 */
void* fs_init(struct fuse_conn_info *conn)
{
    struct fs5600_super sb;
    /* here 1 stands for block size, here is 1024 bytes */
    disk->ops->read(disk, 0, 1, &sb);

    /* your code here */
    /* read bitmaps */
    inode_map = malloc(sb.inode_map_sz * FS_BLOCK_SIZE);
    disk->ops->read(disk, 1, sb.inode_map_sz, inode_map);
    
    block_map = malloc(sb.block_map_sz * FS_BLOCK_SIZE);
    disk->ops->read(disk, sb.inode_map_sz + 1, sb.block_map_sz, block_map);

    /* read inodes */
    inode_region = malloc(sb.inode_region_sz * FS_BLOCK_SIZE);
    int inode_region_pos = 1 + sb.inode_map_sz + sb.block_map_sz;
    disk->ops->read(disk, inode_region_pos, sb.inode_region_sz, inode_region);

    return NULL;
}

/* Note on path translation errors:
 * In addition to the method-specific errors listed below, almost
 * every method can return one of the following errors if it fails to
 * locate a file or directory corresponding to a specified path.
 *
 * ENOENT - a component of the path is not present.
 * ENOTDIR - an intermediate component of the path (e.g. 'b' in
 *           /a/b/c) is not a directory
 */

/* note on splitting the 'path' variable:
 * the value passed in by the FUSE framework is declared as 'const',
 * which means you can't modify it. The standard mechanisms for
 * splitting strings in C (strtok, strsep) modify the string in place,
 * so you have to copy the string and then free the copy when you're
 * done. One way of doing this:
 *
 *    char *_path = strdup(path);
 *    int inum = translate(_path);
 *    free(_path);
 */
/* TODO: duplicated name dir and file */
/* translate: return the inode number of given path */
static int translate(const char *path) {
    /* split the path */
    char *_path;
    _path = strdup(path);
    /* traverse to path */
    /* root inode */
    int inode_num = 1;
    struct fs5600_inode *inode;
    struct fs5600_dirent *dir;
    dir = malloc(FS_BLOCK_SIZE);

    struct fs5600_dirent dummy_dir = {
	.valid = 1,
	.isDir = 1,
	.inode = inode_num,
	.name = "/",
    };
    struct fs5600_dirent *current_dir = &dummy_dir;

    char *token;
    char *delim = "/";
    token = strtok(_path, delim);
    while (token != NULL) {
        if (current_dir->valid == 0) {
	        return ENOENT;
	    }
	    if (current_dir->isDir == 0) {
	        token = strtok(NULL, delim);
	        if (token == NULL) {
		    break;
            } else {
		        return ENOTDIR;
	        }
	    }
	    assert(current_dir->isDir);
	    inode = &inode_region[inode_num];
	    int block_pos = inode->direct[0];
	    disk->ops->read(disk, block_pos, 1, dir);
	    int i;
	    int found = 0;
	    for (i = 0; i < 32; i++) {
            if (strcmp(dir[i].name, token) == 0) {
                found = 1;
                inode_num = dir[i].inode;
                current_dir = &dir[i];
            }
	    }
	    if (found == 0) {
	        printf("ENOENT");
	            return ENOENT;
	    }
        token = strtok(NULL, delim);

    }

    /* traverse all the subsides */
    /* if found, return corresponding inode */
    /* else, return error */
    free(dir);
    free(_path);
    return inode_num;
}

/* trancate the last token from path */
int trancate_path (const char *path, char **trancated_path) {
    int i = strlen(path) - 1;
    for (; i >= 0; i--) {
        if (path[i] != '/') {
            break;
        }
    }
    for (; i >= 0; i--) {
    	if (path[i] == '/') {
            *trancated_path = (char*)malloc(sizeof(char) * (i + 2));
            memcpy(*trancated_path, path, i + 1);
            (*trancated_path)[i + 1] = '\0';
            return 0;
    	}
    }
    return 1;
}

static void set_attr(struct fs5600_inode *inode, struct stat *sb) {
    /* set every other bit to zero */
    memset(sb, 0, sizeof(*sb));
    sb->st_mode = inode->mode;
    sb->st_uid = inode->uid;
    sb->st_size = inode->size;
    sb->st_blocks = 1 + ((inode->size - 1) / FS_BLOCK_SIZE);
    sb->st_nlink = 1;
    sb->st_atime = inode->ctime;
    sb->st_ctime = inode->ctime;
    sb->st_mtime = inode->ctime;
}

/* getattr - get file or directory attributes. For a description of
 *  the fields in 'struct stat', see 'man lstat'.
 *
 * Note - fields not provided in CS5600fs are:
 *    st_nlink - always set to 1
 *    st_atime, st_ctime - set to same value as st_mtime
 *
 * errors - path translation, ENOENT
 */
static int fs_getattr(const char *path, struct stat *sb)
{
    int inum = translate(path);
    if (inum == ENOENT) {
    	return ENOENT;
    }

    struct fs5600_inode *inode = &inode_region[inum];
    set_attr(inode, sb);
    /* what should I return if succeeded?
     success (0) */
    return 0;
}

/* check whether this inode is a directory */
int inode_is_dir(int father_inum, int inum) {
    struct fs5600_inode *inode;
    struct fs5600_dirent *dir;
    dir = malloc(FS_BLOCK_SIZE);

    inode = &inode_region[father_inum];
    int block_pos = inode->direct[0];
    disk->ops->read(disk, block_pos, 1, dir);
    int i;
    for (i = 0; i < 32; i++) {
	if (dir[i].valid == 0) {
	    continue;
	}
	if (dir[i].inode == inum) {
	    return dir[i].isDir;
	}
    }
    return 0;
}
/* readdir - get directory contents.
 *
 * for each entry in the directory, invoke the 'filler' function,
 * which is passed as a function pointer, as follows:
 *     filler(buf, <name>, <statbuf>, 0)
 * where <statbuf> is a struct stat, just like in getattr.
 *
 * Errors - path resolution, ENOTDIR, ENOENT
 */
static int fs_readdir(const char *path, void *ptr, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    char *trancated_path;
    int father_inum = 0;
    if (!trancate_path(path, &trancated_path)) {
        father_inum = translate(trancated_path);
    }

    int inum = translate(path);
    if (inum == ENOTDIR || inum == ENOENT) {
    	return inum;
    }

    if (father_inum != 0 && !inode_is_dir(father_inum, inum)) {
    	return ENOTDIR;
    }
    struct fs5600_inode *inode;
    struct fs5600_dirent *dir;
    dir = malloc(FS_BLOCK_SIZE);
    
    inode = &inode_region[inum];
    // assert is dir
    assert(S_ISDIR(inode->mode));
    int block_pos = inode->direct[0];
    disk->ops->read(disk, block_pos, 1, dir);

    int curr_inum;
    struct fs5600_inode *curr_inode;
    char *name;
    struct stat *sb = malloc(sizeof(struct stat));
    int i;
    for (i = 0; i < 32; i++) {
//        printf("name is : %s\n", dir[i].name);
//        printf("valid is : %d\n", dir[i].valid);
    	if (dir[i].valid == 0) {
    	    continue;
    	}
    	curr_inum = dir[i].inode;
    	curr_inode = &inode_region[curr_inum];

    	name = dir[i].name;
    	set_attr(curr_inode, sb);
    	filler(NULL, name, sb, 0);
    }

    free(sb);
    free(dir);
    return 0;
}

/* mknod - create a new file with specified permissions
 *
 * Errors - path resolution, EEXIST
 *          in particular, for mknod("/a/b/c") to succeed,
 *          "/a/b" must exist, and "/a/b/c" must not.
 *
 * If a file or directory of this name already exists, return -EEXIST.
 * If this would result in >32 entries in a directory, return -ENOSPC
 * if !S_ISREG(mode) [i.e. 'mode' specifies a device special
 * file or other non-file object] then return -EINVAL
 */
static int fs_mknod(const char *path, mode_t mode, dev_t dev)
{
    return -EOPNOTSUPP;
}

/* mkdir - create a directory with the given mode.
 * Errors - path resolution, EEXIST
 * Conditions for EEXIST are the same as for create. 
 * If this would result in >32 entries in a directory, return -ENOSPC
 *
 * Note that you may want to combine the logic of fs_mknod and
 * fs_mkdir. 
 */ 
static int fs_mkdir(const char *path, mode_t mode)
{
    return -EOPNOTSUPP;
}

/* truncate - truncate file to exactly 'len' bytes
 * Errors - path resolution, ENOENT, EISDIR, EINVAL
 *    return EINVAL if len > 0.
 */
static int fs_truncate(const char *path, off_t len)
{
    /* We'll cheat by only implementing this for the case of len==0,
     * and an error otherwise, as 99.99% of the time that's how
     * truncate is used.  
     */
    if (len != 0)
	return -EINVAL;		/* invalid argument */

    return -EOPNOTSUPP;
}

/* unlink - delete a file
 *  Errors - path resolution, ENOENT, EISDIR
 * Note that you have to delete (i.e. truncate) all the data.
 */
static int fs_unlink(const char *path)
{
    return -EOPNOTSUPP;
}

/* rmdir - remove a directory
 *  Errors - path resolution, ENOENT, ENOTDIR, ENOTEMPTY
 * Remember that you have to check to make sure that the directory is
 * empty 
 */
static int fs_rmdir(const char *path)
{
    return -EOPNOTSUPP;
}

/* rename - rename a file or directory
 * Errors - path resolution, ENOENT, EINVAL, EEXIST
 *
 * ENOENT - source does not exist
 * EEXIST - destination already exists
 * EINVAL - source and destination are not in the same directory
 *
 * Note that this is a simplified version of the UNIX rename
 * functionality - see 'man 2 rename' for full semantics. In
 * particular, the full version can move across directories, replace a
 * destination file, and replace an empty directory with a full one.
 */
static int fs_rename(const char *src_path, const char *dst_path)
{
    return -EOPNOTSUPP;
}

/* chmod - change file permissions
 *
 * Errors - path resolution, ENOENT.
 */
static int fs_chmod(const char *path, mode_t mode)
{
    return -EOPNOTSUPP;
}

/* utime - change access and modification times (see 'man utime')
 * Errors - path resolution, ENOENT.
 * The utimbuf structure has two fields:
 *   time_t actime;  // access time - ignore
 *   time_t modtime; // modification time, same format as in inode
 */
int fs_utime(const char *path, struct utimbuf *ut)
{
    return -EOPNOTSUPP;
}

//static int fs_read_3rd_level(int inum, int offset, int len, char *buf);
//static int fs_read_2nd_level(int inum, int offset, int len, char *buf);
// given block number, offset, length and return buffer, load corresponding data into buffer
static int fs_read_block(int blknum, int offset, int len, char *buf);
//static int fs_read_file_by_inode();

// return the read in length
static int fs_read_1st_level(char *buf, off_t offset, size_t len, const struct fs5600_inode *inode);

/* read - read data from an open file.
 * should return exactly the number of bytes requested, except:
 *   - if offset >= file len, return 0
 *   - if offset+len > file len, return bytes from offset to EOF
 *   - on error, return <0
 * Errors - path resolution, ENOENT, EISDIR
 */
static int fs_read(const char *path, char *buf, size_t len, off_t offset,
                   struct fuse_file_info *fi)
{
    // first get the inode
    // check it is valid
    // check it is file

    int inum = translate(path);
    const struct fs5600_inode *inode = &inode_region[inum];
    // assert path is a file
    assert(S_ISREG(inode->mode));
    int size = inode->size;
    if (offset >= size) {
        return 0;
    }
    if (offset + len > size) {
        len = size - offset;
    }
    if (offset <= 6 * BLOCK_SIZE) {
        offset += fs_read_1st_level(buf, offset, len, inode);
    }
    if (offset > 6 * BLOCK_SIZE && offset <= BLOCK_SIZE / 4 * BLOCK_SIZE) {

        printf("TODO\n");
        return -EOPNOTSUPP;
    } else
    if (offset > BLOCK_SIZE / 4 * BLOCK_SIZE && offset <= (BLOCK_SIZE / 4) * (BLOCK_SIZE / 4) * BLOCK_SIZE) {
        printf("TODO\n");
        return -EOPNOTSUPP;
    }

    return len;
}

static int fs_read_1st_level(char *buf, off_t offset, size_t len, const struct fs5600_inode *inode) {
    int read_length = 0;
    int block_direct = offset / BLOCK_SIZE;
    int temp_len = len;
    int in_blk_len;
    int temp_offset = offset % BLOCK_SIZE;

    for (; block_direct < 6 && temp_len > 0; temp_offset = 0, block_direct++) {
            if (temp_len > BLOCK_SIZE) {
                in_blk_len = BLOCK_SIZE;
                temp_len -= BLOCK_SIZE;
            } else {
                in_blk_len = temp_len;
                temp_len = 0;
            }
            fs_read_block(inode->direct[block_direct], temp_offset, in_blk_len, buf);
            buf += in_blk_len;
            read_length += in_blk_len;
        }
    return read_length;
}

static int fs_read_block(int blknum, int offset, int len, char *buf) {
    char *blk = (char*) malloc(BLOCK_SIZE);
    disk->ops->read(disk, blknum, 1, blk);
    char *blk_ptr = blk;
    blk_ptr += offset;
    memcpy(buf, blk_ptr, len);
    free(blk);
    return 0;
}

/* write - write data to a file
 * It should return exactly the number of bytes requested, except on
 * error.
 * Errors - path resolution, ENOENT, EISDIR
 *  return EINVAL if 'offset' is greater than current file length.
 *  (POSIX semantics support the creation of files with "holes" in them, 
 *   but we don't)
 */
static int fs_write(const char *path, const char *buf, size_t len,
		     off_t offset, struct fuse_file_info *fi)
{
    return -EOPNOTSUPP;
}

/* statfs - get file system statistics
 * see 'man 2 statfs' for description of 'struct statvfs'.
 * Errors - none.
 */
static int fs_statfs(const char *path, struct statvfs *st)
{
    /* Already implemented. Optional - set the following:
     *  f_blocks - total blocks in file system
     *  f_bfree, f_bavail - unused blocks
     * You could calculate bfree dynamically by scanning the block
     * allocation map. 
     */
    st->f_bsize = FS_BLOCK_SIZE;
    st->f_blocks = 0;
    st->f_bfree = 0;
    st->f_bavail = 0;
    st->f_namemax = 27;

    return 0;
}

/* operations vector. Please don't rename it, as the skeleton code in
 * misc.c assumes it is named 'fs_ops'.
 */
struct fuse_operations fs_ops = {
    .init = fs_init,
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .mknod = fs_mknod,
    .mkdir = fs_mkdir,
    .unlink = fs_unlink,
    .rmdir = fs_rmdir,
    .rename = fs_rename,
    .chmod = fs_chmod,
    .utime = fs_utime,
    .truncate = fs_truncate,
    .read = fs_read,
    .write = fs_write,
    .statfs = fs_statfs,
};

