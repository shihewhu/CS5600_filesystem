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
#include <time.h>

#include "fs5600.h"
#include "blkdev.h"

/*
 * disk access - the global variable 'disk' points to a blkdev
 *
 * structure which has been initialized to access the image file.
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
int inode_map_sz;
int block_map_sz;
int num_of_blocks;
struct fs5600_inode *inode_region;	/* inodes in memory */
void update_bitmap(void);

// some constants
int file_in_inode_sz = N_DIRECT * BLOCK_SIZE;
int file_1st_level_sz = BLOCK_SIZE / sizeof(int) * BLOCK_SIZE;
int file_2nd_level_sz = (BLOCK_SIZE / sizeof(int)) * (BLOCK_SIZE / sizeof(int)) * BLOCK_SIZE;


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
    inode_map_sz = sb.inode_map_sz;
    // printf("%d\n", inode_map_sz);

    block_map = malloc(sb.block_map_sz * FS_BLOCK_SIZE);
    disk->ops->read(disk, sb.inode_map_sz + 1, sb.block_map_sz, block_map);
    block_map_sz = sb.block_map_sz;

    // printf("%d\n", block_map_sz);
    /* read inodes */
    inode_region = malloc(sb.inode_region_sz * FS_BLOCK_SIZE);
    int inode_region_pos = 1 + sb.inode_map_sz + sb.block_map_sz;
    disk->ops->read(disk, inode_region_pos, sb.inode_region_sz, inode_region);
    // printf("%d\n", sb.inode_region_sz);
    num_of_blocks = sb.num_blocks;
    // printf("%d\n", num_of_blocks);

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
/* translate: return the inode number of given path */
static int translate(const char *path) {
    /* split the path */
    char *_path;
    _path = strdup(path);
    /* traverse to path */
    /* root father_inode */
    int inode_num = 1;
    struct fs5600_inode *father_inode;
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
    int error = 0;
    /* traverse all the subsides */
    /* if found, return corresponding father_inode */
    /* else, return error */
    while (token != NULL) {
        if (current_dir->valid == 0) {
	        error = -ENOENT;
            break;
	    }
	    if (current_dir->isDir == 0) {
	        if (token != NULL) {
                error = -ENOTDIR;
            }
            break;
	    }
	    father_inode = &inode_region[inode_num];
	    int block_pos = father_inode->direct[0];
	    disk->ops->read(disk, block_pos, 1, dir);
	    int i;
	    int found = 0;
	    for (i = 0; i < 32; i++) {
            if (strcmp(dir[i].name, token) == 0 && dir[i].valid == 1) {
                found = 1;
                inode_num = dir[i].inode;
                current_dir = &dir[i];
            }
	    }
	    if (found == 0) {
            error = -ENOENT;
            break;
	    }
        token = strtok(NULL, delim);
    }

    free(dir);
    free(_path);
    if (error != 0) {
        return error;
    }
    return inode_num;
}
int trancate_path (const char *path, char **trancated_path);
/* trancate the last token from path
 * return 1 if succeed, 0 if not*/
int trancate_path (const char *path, char **trancated_path) {
    int i = strlen(path) - 1;
    // strip the tailling '/'
    // deal with '///' case
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
            return 1;
    	}
    }
    return 0;
}

static void set_attr(struct fs5600_inode inode, struct stat *sb) {
    /* set every other bit to zero */
    memset(sb, 0, sizeof(struct stat));
    sb->st_mode = inode.mode;
    sb->st_uid = inode.uid;
    sb->st_gid = inode.gid;
    sb->st_size = inode.size;
    sb->st_blocks = 1 + ((inode.size - 1) / FS_BLOCK_SIZE);
    sb->st_nlink = 1;
    sb->st_atime = inode.mtime;
    sb->st_ctime = inode.ctime;
    sb->st_mtime = inode.mtime;
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
    fs_init(NULL);
    int inum = translate(path);
    if (inum == -ENOENT || inum == -ENOTDIR) {
    	return -ENOENT;
    }

    struct fs5600_inode inode = inode_region[inum];
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
        int result = dir[i].inode;
        free(dir);
	    return result;
	}
    }
    free(dir);
    return 0;
}
/* readdir - get directory contents.
 *
 * for each entry in the directory, invoke the 'filler' function,
 * which is passed as a function pointer, as follows:
 *     filler(ptr, <name>, <statbuf>, 0)
 * where <statbuf> is a struct stat, just like in getattr.
 *
 * Errors - path resolution, ENOTDIR, ENOENT
 */
static int fs_readdir(const char *path, void *ptr, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    char *trancated_path;
    int father_inum = 0;
    // if succeeded in trancating path
    if (trancate_path(path, &trancated_path)) {
        father_inum = translate(trancated_path);
    }

    int inum = translate(path);
    if (inum == -ENOTDIR || inum == -ENOENT) {
    	return inum;
    }

    if (father_inum != 0 && !inode_is_dir(father_inum, inum)) {
    	return -ENOTDIR;
    }


    struct fs5600_inode *inode;
    struct fs5600_dirent *dir;
    inode = &inode_region[inum];
    // check is dir
    if(!S_ISDIR(inode->mode)) {
        return -ENOTDIR;
    }

    dir = malloc(FS_BLOCK_SIZE);
    int block_pos = inode->direct[0];
    disk->ops->read(disk, block_pos, 1, dir);
    int curr_inum;
    struct fs5600_inode curr_inode;

    struct stat sb;

    int i;
    for (i = 0; i < 32; i++) {
    	if (dir[i].valid == 0) {
    	    continue;
    	}

    	curr_inum = dir[i].inode;
        curr_inode = inode_region[curr_inum];
    	set_attr(curr_inode, &sb);
    	filler(ptr, dir[i].name, &sb, 0);
    }
    free(dir);
    return 0;
}

int find_free_dirent_num(struct fs5600_inode *inode);

int find_free_inode_map_bit();

static char *get_name(char *path);
static void strip(char *path);

void update_inode(int inum);

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
    if (!S_ISREG(mode)) {
        return -EINVAL;
    }
    // check father dir exist
    char *father_path;
    if (!trancate_path(path, &father_path)) {
        // this means there is no nod to make, path is "/"
        return -1;
    }
    int dir_inum = translate(father_path);
    if (dir_inum == -ENOENT || dir_inum == -ENOTDIR) {
        return -EEXIST;
    }
    // check if dest file exists
    int inum = translate(path);
    if (inum > 0) {
        return -EEXIST;
    }
    // check entries in father dir not excceed 32
    struct fs5600_inode *father_inode = &inode_region[dir_inum];
    int free_dirent_num = find_free_dirent_num(father_inode);
    if(free_dirent_num < 0) {
        return -ENOSPC;
    }

    // here allocate inode region, i.e. set inode region bitmap
    time_t time_raw_format;
    time( &time_raw_format );
    struct fs5600_inode new_inode = {
            .uid = getuid(),
            .gid = getgid(),
            .mode = mode,
            .ctime = time_raw_format,
            .mtime = time_raw_format,
            .size = 0,
    };
    int free_inum = find_free_inode_map_bit();
    if (free_inum < 0) {
        return -ENOSPC;
    }
    FD_SET(free_inum, inode_map);
    update_bitmap();

    // write father_inode to the allocated pos in father_inode region
    memcpy(&inode_region[free_inum], &new_inode, sizeof(struct fs5600_inode));
    update_inode(free_inum);


    // set valid, isDir, father_inode, name in father father_inode dirent
    // then write dirent to image
    char *_path = strdup(path);
    char *tmp_name = get_name(_path);
    struct fs5600_dirent new_dirent = {
            .valid = 1,
            .isDir = 0,
            .inode = free_inum,
            .name = "",
    };
    assert(strlen(tmp_name) < 28);
    strncpy(new_dirent.name, tmp_name, strlen(tmp_name));
    new_dirent.name[strlen(tmp_name)] = '\0';

    struct fs5600_dirent *dir_blk = (struct fs5600_dirent *)calloc(BLOCK_SIZE / sizeof(int), sizeof(int));
    disk->ops->read(disk, (father_inode->direct)[0], 1, dir_blk);
    memcpy(&dir_blk[free_dirent_num], &new_dirent, sizeof(struct fs5600_dirent));
    disk->ops->write(disk, father_inode->direct[0], 1, dir_blk);
    free(dir_blk);
    free(_path);
    return 0;
}

void update_inode(int inum) {
    int offset = 1 + inode_map_sz + block_map_sz + (inum / 16);
    disk->ops->write(disk, offset, 1, &inode_region[inum - (inum % 16)]);
}

static void strip(char *path) {
	if (path[strlen(path) - 1] == '/') {
		path[strlen(path) - 1] = '\0';
	}
}
static char *get_name(char *path) {
    int i = strlen(path) - 1;
    for (; i >= 0; i--) {
        if (path[i] == '/') {
            i++;
            break;
        }
    }
    char *result = &path[i];
    return result;
}

int find_free_inode_map_bit() {// find a free inode_region
    int inode_capacity = inode_map_sz * FS_BLOCK_SIZE * 8;
    int i;
    for (i = 2; i < inode_capacity; i++) {
        if (!FD_ISSET(i, inode_map)) {
            return i;
        }
    }
    return -ENOSPC;
}

int find_free_dirent_num(struct fs5600_inode *inode) {
    struct fs5600_dirent *dir = (struct fs5600_dirent *)malloc(BLOCK_SIZE);
    disk->ops->read(disk, (inode->direct)[0], 1, dir);

    int free_dirent_num = -1;
    int i;
    for (i = 0; i < 32; i++) {
        if (!dir[i].valid) {
            free_dirent_num = i;
            break;
        }
    }
    free(dir);
    return free_dirent_num;
}

int find_free_block_number();
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
    mode = mode | S_IFDIR;
    if (!S_ISDIR(mode)) {
        return -EINVAL;
    }
    /*check father dir exist*/
    char *father_path;
    if (!trancate_path(path, &father_path)) {
        // this means there is no nod to make, path is "/"
        return -1;
    }
    int dir_inum = translate(father_path);
    if (dir_inum == -ENOENT || dir_inum == -ENOTDIR) {
        return -EEXIST;
    }


    // check if dest file exists
    int inum = translate(path);
    if (inum > 0) {
        return -EEXIST;
    }
    // check entries in father dir not excceed 32
    struct fs5600_inode *father_inode = &inode_region[dir_inum];
    int free_dirent_num = find_free_dirent_num(father_inode);
    if(free_dirent_num < 0) {
        return -ENOSPC;
    }
    //check father is a dir
    if (!S_ISDIR(father_inode->mode)) {
        return -EEXIST;
    }

    // here allocate inode region, i.e. set inode region bitmap
    time_t time_raw_format;
    time( &time_raw_format );
    struct fs5600_inode new_inode = {
            .uid = getuid(),
            .gid = getgid(),
            .mode = mode,
            .ctime = time_raw_format,
            .mtime = time_raw_format,
            .size = 0,
            .direct = {0, 0, 0, 0, 0, 0},
    };
    int free_blk_num = find_free_block_number();
    FD_SET(free_blk_num, block_map);
    update_bitmap();
    new_inode.direct[0] = free_blk_num;
    int *clear_block = (int *)calloc(BLOCK_SIZE, sizeof(int));
    disk->ops->write(disk, new_inode.direct[0], 1, clear_block);
    int free_inum = find_free_inode_map_bit();
    if (free_inum < 0) {
        free(clear_block);
        return -ENOSPC;
    }
    FD_SET(free_inum, inode_map);
    update_bitmap();

    // write father_inode to the allocated pos in father_inode region
    memcpy(&inode_region[free_inum], &new_inode, sizeof(struct fs5600_inode));
    update_inode(free_inum);


    // set valid, isDir, father_inode, name in father father_inode dirent
    // then write dirent to image
    char *_path = strdup(path);
    char *tmp_name = get_name(_path);
    struct fs5600_dirent new_dirent = {
            .valid = 1,
            .isDir = 1,
            .inode = free_inum,
            .name = "",
    };
    assert(strlen(tmp_name) < 28);
    memcpy(new_dirent.name, tmp_name, strlen(tmp_name));

    struct fs5600_dirent *dir_blk = (struct fs5600_dirent *)malloc(BLOCK_SIZE);
    disk->ops->read(disk, (father_inode->direct)[0], 1, dir_blk);
    memcpy(&dir_blk[free_dirent_num], &new_dirent, sizeof(struct fs5600_dirent));
    disk->ops->write(disk, father_inode->direct[0], 1, dir_blk);

    free(clear_block);
    free(dir_blk);
    free(_path);
    return 0;
    // return -EOPNOTSUPP;
}

void truncate_2nd_level(int h1t_root_blk_num);

void truncate_3rd_level(int h2t_root_blk_num);

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

    int inum = translate(path);
    if (inum == -ENOENT || inum == -ENOTDIR) {
        return -ENOENT;
    }
    struct fs5600_inode *inode = &inode_region[inum];
    if  (S_ISDIR(inode->mode)) {
        return -EISDIR;
    }

    // clear the block bit map of this inode
    int temp_blk_num;
    int i;
    for (i = 0; i < N_DIRECT; i++) {
        temp_blk_num = inode->direct[i];
        inode->direct[i] = 0;
        if (temp_blk_num != 0) {
            FD_CLR(temp_blk_num, block_map);
            update_bitmap();
        } else {
            break;
        }
    }
    if (inode->size > N_DIRECT * BLOCK_SIZE) {
        truncate_2nd_level(inode->indir_1);
    }

    if (inode->size > (BLOCK_SIZE / 4) * BLOCK_SIZE + N_DIRECT * BLOCK_SIZE) {
        truncate_3rd_level(inode->indir_2);
    }

    // set the size of inode as 0
    inode_region[inum].size = 0;
    inode_region[inum].indir_1 = 0;
    inode_region[inum].indir_2 = 0;
    update_inode(inum);
    return 0;
}


void truncate_2nd_level(int h1t_root_blk_num) {
    int h1t_blk[256];
    disk->ops->read(disk, h1t_root_blk_num, 1, h1t_blk);
    int i;
    for (i = 0; i < 256; ++i) {
        int temp_blk_num = h1t_blk[i];
        if (temp_blk_num != 0) {
            FD_CLR(temp_blk_num, block_map);
        } else {
            break;
        }
    }
    FD_CLR(h1t_root_blk_num, block_map);
    update_bitmap();
}

void truncate_3rd_level(int h2t_root_blk_num) {
    sleep(0.1);
    int h2t_blk[256];
    disk->ops->read(disk, h2t_root_blk_num, 1, h2t_blk);
    int i;
    for (i = 0; i < 256; ++i) {
        int temp_blk_num = h2t_blk[i];
        if (temp_blk_num == 0) {
            break;
        }
        truncate_2nd_level(h2t_blk[i]);
    }
    FD_CLR(h2t_root_blk_num, block_map);
    update_bitmap();
}

/* unlink - delete a file
 *  Errors - path resolution, ENOENT, EISDIR
 * Note that you have to delete (i.e. truncate) all the data.
 */
static int fs_unlink(const char *path)
{
    int inum = translate(path);
    if (inum == -ENOENT || inum == -ENOTDIR) {
        return -ENOENT;
    }
    struct fs5600_inode *inode = &inode_region[inum];
    if  (S_ISDIR(inode->mode)) {
        return -EISDIR;
    }

    // truncate all the data
    int truncate_result = fs_truncate(path, 0);
    if (truncate_result != 0) {
        return truncate_result;
    }

    char *father_path;
    trancate_path(path, &father_path);
    int father_inum = translate(father_path);
    free(father_path);
    struct fs5600_inode *father_inode = &inode_region[father_inum];


    // remove inode, i.e. clear inode_map corresponding bit
    FD_CLR(inum, inode_map);
    update_bitmap();

    // remove entry from father dir
    char *_path = strdup(path);
    char *name = get_name(_path);

    struct fs5600_dirent *father_dir = malloc(FS_BLOCK_SIZE);
    disk->ops->read(disk, father_inode->direct[0], 1, father_dir);
    int found = 0;
    int i;
    for (i = 0; i < 32; ++i) {
        if (strcmp(father_dir[i].name, name) == 0) {
            if (father_dir[i].valid == 1 ) {
                father_dir[i].valid = 0;
                found = 1;
            }
        }
    }
    disk->ops->write(disk, father_inode->direct[0], 1, father_dir);
    if (!found) {
        return -ENOENT;
    }
    free(father_dir);
    free(_path);
    return 0;
}

/* rmdir - remove a directory
 *  Errors - path resolution, ENOENT, ENOTDIR, ENOTEMPTY
 * Remember that you have to check to make sure that the directory is
 * empty
 */
static int fs_rmdir(const char *path)
{
    // check dir is dir
    int inum = translate(path);
    if (inum == -ENOENT || inum == -ENOTDIR) {
        return -ENOENT;
    }
    struct fs5600_inode *inode = &inode_region[inum];
    if  (S_ISREG(inode->mode)) {
        return -ENOTDIR;
    }

    // check not root dir
    char *father_path;
    int succeed = trancate_path(path, &father_path);
    if (!succeed) {
        printf("Attempting to delete root directory");
        assert(0);
    }

    // check dir is empty
    struct fs5600_dirent *dirent = malloc(FS_BLOCK_SIZE);
    disk->ops->read(disk, inode->direct[0], 1, dirent);
    int empty = 1;
    int i;
    for (i = 0; i < 32; ++i) {
        if (dirent[i].valid) {
            empty = 0;
            break;
        }
    }
    free(dirent);
    if (!empty) {
        return -ENOTEMPTY;
    }

    // block map remove the block of this dir
    FD_SET(inode->direct[0], inode_map);
    update_bitmap();

    // inode map remove this dir
    FD_SET(inum, inode_map);
    update_bitmap();

    // then unlink this dir
    char *_path = strdup(path);
    strip(_path);
    char *name = get_name(_path);
    int father_inum = translate(father_path);
    free(father_path);
    struct fs5600_inode *father_inode = &inode_region[father_inum];
    struct fs5600_dirent *father_dirent = malloc(FS_BLOCK_SIZE);
    disk->ops->read(disk, father_inode->direct[0], 1, father_dirent);
    for (i = 0; i < 32; ++i) {
        if (strcmp(father_dirent[i].name, name) == 0) {
            if (father_dirent[i].valid == 1 ) {
                father_dirent[i].valid = 0;
            }
        }
    }
    disk->ops->write(disk, father_inode->direct[0], 1, father_dirent);

    free(_path);
    return -0;
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

 /*TODO: finished: compile succeeds, simple test passed, need more test*/
static int fs_rename(const char *src_path, const char *dst_path)
{
	/*check exists of src file and dst file*/
	int src_inum, dst_inum;
	src_inum = translate(src_path);
	dst_inum = translate(dst_path);
   	if (src_inum < 0)  {
   		return -ENOENT;
   	}
   	if (dst_inum > 0) {
   		return -EEXIST;
   	}
   	/*check exists of father dir*/
   	char *src_father_path;
   	char *dst_father_path;
   	if (!trancate_path(src_path, &src_father_path) || !trancate_path(dst_path, &dst_father_path)) {
        free(src_father_path);
        free(dst_father_path);
        return -ENOENT;
   	}
   	if (strcmp(src_father_path, dst_father_path) != 0) {
        free(src_father_path);
        free(dst_father_path);
   		return -EINVAL;
   	}
   	int father_inum;
   	if (!(father_inum = translate(src_father_path))) {
        free(src_father_path);
        free(dst_father_path);
   		return father_inum;
   	}
   	/*get the name of the src and dst path*/
   	char *_src_path = strdup(src_path);
   	char *src_name = get_name(_src_path);
   	char *_dst_path = strdup(dst_path);
   	char *dst_name = get_name(_dst_path);

   	/*load dirent block to memory to search src file name*/
   	struct fs5600_inode *father_inode;
    struct fs5600_dirent *dir;
    dir = malloc(FS_BLOCK_SIZE);

    father_inode = &inode_region[father_inum];
    int block_pos = father_inode->direct[0];
    disk->ops->read(disk, block_pos, 1, dir);
    int i;
    /*traverse the drient block to find the dirent with the same name*/
    for (i = 0;i < 32; i++) {
    	if (dir[i].valid == 1 && strcmp(dir[i].name, src_name) == 0) {
    		strncpy(dir[i].name, dst_name, strlen(dst_name));
    		dir[i].name[strlen(dst_name)] = '\0';
    		disk->ops->write(disk, block_pos, 1, dir);
    	}
    }
    free(_src_path);
    free(_dst_path);
   	free(src_father_path);
   	free(dst_father_path);
    return 0;
}

/* chmod - change file permissions
 *
 * Errors - path resolution, ENOENT.
 */
 /*TODO: finished: simple test passed but need more test*/
static int fs_chmod(const char *path, mode_t mode)
{
    int inum = translate(path);
    if (inum < 0) {
    	return inum;
    }
    struct fs5600_inode *inode;
    inode = &inode_region[inum];
    inode->mode = mode;
    update_inode(inum);
    return 0;
}

/* utime - change access and modification times (see 'man utime')
 * Errors - path resolution, ENOENT.
 * The utimbuf structure has two fields:
 *   time_t actime;  // access time - ignore
 *   time_t modtime; // modification time, same format as in inode
 */
int fs_utime(const char *path, struct utimbuf *ut)
{
    int inum = translate(path);
    if (inum < 0) {
    	return inum;
    }
    struct fs5600_inode *inode;
    inode = &inode_region[inum];
    inode->mtime = ut->modtime;
    update_inode(inum);
    return 0;
}


/*
 * given block number, offset, length and return buffer, load corresponding data into buffer
 *
 */
static int fs_read_block(int blknum, int offset, int len, char *buf);

// return the read in length
static int fs_read_1st_level(const struct fs5600_inode *inode, off_t offset, size_t len, char *buf);
// return the read in length
static int fs_read_2nd_level(size_t root_blk, int offset, int len, char *buf);
static int fs_read_3rd_level(size_t root_blk, int offset, int len, char *buf);

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
    /*first get the inode
    check it is valid
    check it is file*/

    int tmp_offset = offset;
    int inum = translate(path);
    if (inum == -ENOENT || inum == -ENOTDIR) {
        return -ENOENT;
    }
    const struct fs5600_inode *inode = &inode_region[inum];
    if(!S_ISREG(inode->mode)) {
        return -EISDIR;
    }
    int size = inode->size;
    if (tmp_offset >= size) {
        return 0;
    }
    if (tmp_offset + len > size) {
        len = size - tmp_offset;
    }
    int tmp_len = len;
    if (tmp_offset < file_in_inode_sz) {
        int read_len = fs_read_1st_level(inode, tmp_offset, tmp_len, buf);
        tmp_offset += read_len;
        tmp_len -= read_len;
        buf += read_len;
    }
    if (tmp_offset >= file_in_inode_sz &&
        tmp_offset < file_in_inode_sz + file_1st_level_sz) {
        int read_len = fs_read_2nd_level(inode->indir_1, tmp_offset, tmp_len, buf);
        tmp_offset += read_len;
        tmp_len -= read_len;
        buf += read_len;
    }
    if (tmp_offset >= file_in_inode_sz + file_1st_level_sz &&
        tmp_offset <= file_in_inode_sz + file_1st_level_sz + file_2nd_level_sz) {
        int read_len = fs_read_3rd_level(inode->indir_2, tmp_offset, tmp_len, buf);
        tmp_offset += read_len;
        tmp_len -= read_len;
        buf += read_len;
    }
    return tmp_offset - offset;
}
/*
*
*/
static int fs_read_1st_level(const struct fs5600_inode *inode, off_t offset, size_t len, char *buf) {
    int read_length = 0;
    int block_direct = offset / BLOCK_SIZE;
    int temp_len = len;
    int in_blk_len;
    int in_blk_offset = offset % BLOCK_SIZE;
    for (; block_direct < N_DIRECT && temp_len > 0; in_blk_offset = 0, block_direct++) {
        if (temp_len + in_blk_offset > BLOCK_SIZE) {
            in_blk_len = BLOCK_SIZE - in_blk_offset;
            temp_len -= in_blk_len;
        } else {
            in_blk_len = temp_len;
            temp_len = 0;
        }
        if (inode->direct[block_direct] == 0) {
            break;
        }
        fs_read_block(inode->direct[block_direct], in_blk_offset, in_blk_len, buf);
        buf += in_blk_len;
        read_length += in_blk_len;
    }
    return read_length;
}

static int fs_read_2nd_level(size_t root_blk, int offset, int len, char *buf){
    int read_length = 0;
    int h1t_offset = offset - file_in_inode_sz; // height 1 tree offset
    int block_direct = h1t_offset / BLOCK_SIZE;
    int temp_len = len;
    int in_blk_len;
    int in_blk_offset = h1t_offset % BLOCK_SIZE;
    int h1t_blk[256];
    disk->ops->read(disk, root_blk, 1, h1t_blk);

    for (; block_direct < 256 && temp_len > 0; in_blk_offset = 0, block_direct++) {
        if (temp_len + in_blk_offset > BLOCK_SIZE) {
            in_blk_len = BLOCK_SIZE - in_blk_offset;
            temp_len -= in_blk_len;
        } else {
            in_blk_len = temp_len;
            temp_len = 0;
        }
        if (h1t_blk[block_direct] == 0) {
            break;
        }
        fs_read_block(h1t_blk[block_direct], in_blk_offset, in_blk_len, buf);
        buf += in_blk_len;
        read_length += in_blk_len;
    }
    return read_length;
}

static int fs_read_3rd_level(size_t root_blk, int offset, int len, char *buf){
    int read_length = 0;
    int h2t_offset = offset - file_1st_level_sz - file_in_inode_sz;// height 2 tree offset
    int block_direct = h2t_offset / file_1st_level_sz;
    int temp_len = len;
    int in_blk_len;
    int in_blk_offset = h2t_offset % file_1st_level_sz;


    int h2t_blk[256];
    disk->ops->read(disk, root_blk, 1, h2t_blk);

    for (; block_direct < 256 && temp_len > 0; in_blk_offset = 0, block_direct++) {
        if (temp_len + in_blk_offset > file_1st_level_sz) {
            in_blk_len = file_1st_level_sz - in_blk_offset;
            temp_len -= in_blk_len;
        } else {
            in_blk_len = temp_len;
            temp_len = 0;
        }
        fs_read_2nd_level(h2t_blk[block_direct], in_blk_offset + file_in_inode_sz, in_blk_len, buf);
        buf += in_blk_len;
        read_length += in_blk_len;
    }
    return read_length;
}

static int fs_read_block(int blknum, int offset, int len, char *buf) {
    assert(blknum > 0);
    char *blk = (char*) malloc(BLOCK_SIZE);


    disk->ops->read(disk, blknum, 1, blk);
    char *blk_ptr = blk;
    blk_ptr += offset;
    memcpy(buf, blk_ptr, len);
    free(blk);
    return 0;
}
static int fs_write_1st_level(int inode, off_t offset, size_t len, const char *buf);
static int fs_write_2nd_level(size_t root_blk, int offset, int len, const char *buf);
static int fs_write_3rd_level(size_t root_blk, int offset, int len, const char *buf);



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
    int inum;
    if (!(inum = translate(path))) { // here checked path resolution
        return inum;
    }
    struct fs5600_inode *inode = &inode_region[inum];
    if (offset > inode->size) {// check tmp_offset is no larger than file size
        return -EINVAL;
    }

    int tmp_offset = offset;
    int tmp_len = len;

    if (tmp_offset < file_in_inode_sz) {
        int written_len = fs_write_1st_level(inum, tmp_offset, tmp_len, buf);
        tmp_offset += written_len;
        tmp_len -= written_len;
        buf += written_len;
    }
    if (tmp_offset >= file_in_inode_sz &&
            tmp_offset < file_in_inode_sz + file_1st_level_sz) {

        if (inode->indir_1 == 0) {// if indir_1 not set, set it
            // find a free block
            int blk_num = find_free_block_number();
            if (blk_num < 0) {
                return tmp_offset - offset;
            }
            /*change the inode*/
            inode->indir_1 = blk_num;
            update_inode(inum);
            /*set the block bitmap*/
            FD_SET(blk_num, block_map);
            update_bitmap();
        }
        int written_len = fs_write_2nd_level(inode->indir_1, tmp_offset, tmp_len, buf);
        tmp_offset += written_len;
        tmp_len -= written_len;
        buf += written_len;
    }
    if (tmp_offset >= file_in_inode_sz + file_1st_level_sz &&
            tmp_offset <= file_in_inode_sz + file_1st_level_sz + file_2nd_level_sz) {
        if (inode->indir_2 == 0) { // if indir_2 not set, set it
            // find a free block
            int blk_num = find_free_block_number();
            if (blk_num < 0) {
                return tmp_offset - offset;
            }
            /*change the inode*/
            inode->indir_2 = blk_num;
            update_inode(inum);
            /*set the block bitmap*/
            FD_SET(blk_num, block_map);
            update_bitmap();
        }
        int written_len = fs_write_3rd_level(inode->indir_2, tmp_offset, tmp_len, buf);
        tmp_offset += written_len;
        tmp_len -= written_len;
        buf += written_len;
    }
    /* update inode size */
    if (tmp_offset > inode->size) {
        inode->size = tmp_offset;
        update_inode(inum);
    }
    // printf("written length: %d\n", (int)(tmp_offset - offset));
    return tmp_offset - offset;
}

static int fs_write_1st_level(int inum, off_t offset, size_t len, const char *buf) {
    int written_length = 0;
    struct fs5600_inode *inode = &inode_region[inum];
    int block_direct = offset / BLOCK_SIZE;
    int temp_len = len;
    int in_blk_len;
    int in_blk_offset = offset % BLOCK_SIZE;

    for (; block_direct < N_DIRECT && temp_len > 0; in_blk_offset = 0, block_direct++) {
        if (temp_len + in_blk_offset > BLOCK_SIZE) {
            in_blk_len = BLOCK_SIZE - in_blk_offset;
            temp_len -= in_blk_len;
        } else {
            in_blk_len = temp_len;
            temp_len = 0;
        }
        // if there is already allocated
        if (inode->direct[block_direct] == 0) {
            // find a free block
            int blk_num = find_free_block_number();
            if (blk_num < 0) {
                return written_length;
            }

            // change the inode
            inode->direct[block_direct] = blk_num;
            update_inode(inum);

            // set the block bitmap
            FD_SET(blk_num, block_map);
            update_bitmap();
        }
        // write data to the found or given block
        char *blk = (char*) malloc(BLOCK_SIZE);
        disk->ops->read(disk, inode->direct[block_direct], 1, blk);
        memcpy(blk + in_blk_offset, buf, in_blk_len);
        disk->ops->write(disk, inode->direct[block_direct], 1, blk);
        free(blk);
        buf += in_blk_len;
        written_length += in_blk_len;
    }

    return written_length;
}

static int fs_write_2nd_level(size_t root_blk, int offset, int len, const char *buf){
    int written_length = 0;
    int h1t_offset = offset - file_in_inode_sz; // height 1 tree offset
    int block_direct = h1t_offset / BLOCK_SIZE;
    int temp_len = len;
    int in_blk_len;
    int in_blk_offset = h1t_offset % BLOCK_SIZE;


    int h1t_blk[256];
    disk->ops->read(disk, root_blk, 1, h1t_blk);

    for (; block_direct < 256 && temp_len > 0; in_blk_offset = 0, block_direct++) {
        if (temp_len + in_blk_offset > BLOCK_SIZE) {
            in_blk_len = BLOCK_SIZE - in_blk_offset;
            temp_len -= in_blk_len;
        } else {
            in_blk_len = temp_len;
            temp_len = 0;
        }
        if (h1t_blk[block_direct] == 0) { // if h1t_blk block direct is not used, allocate a block
            /*find a free block*/
            int blk_num = find_free_block_number();
            if (blk_num < 0) {
                return written_length;
            }
            /*change the h1t block and write back*/
            h1t_blk[block_direct] = blk_num;
            disk->ops->write(disk, root_blk, 1, h1t_blk);
            /*set the block bitmap*/
            FD_SET(blk_num, block_map);
            update_bitmap();
        }
        char *blk = (char*) malloc(BLOCK_SIZE);
        disk->ops->read(disk, h1t_blk[block_direct], 1, blk);
        memcpy(blk + in_blk_offset, buf, in_blk_len);

        disk->ops->write(disk, h1t_blk[block_direct], 1, blk);
        free(blk);
        buf += in_blk_len;
        written_length += in_blk_len;
    }

    return written_length;
}

static int fs_write_3rd_level(size_t root_blk, int offset, int len, const char *buf) {
    int written_length = 0;
    int h2t_offset = offset - file_in_inode_sz - file_1st_level_sz;// height 2 tree offset
    int block_direct = h2t_offset / file_1st_level_sz;
    int temp_len = len;
    int in_blk_len;
    int in_blk_offset = h2t_offset % file_1st_level_sz;

    int h2t_blk[256];
    disk->ops->read(disk, root_blk, 1, h2t_blk);

    for (; block_direct < 256 && temp_len > 0; in_blk_offset = 0, block_direct++) {
        if (temp_len + in_blk_offset > file_1st_level_sz) {
            in_blk_len = file_1st_level_sz - in_blk_offset;
            temp_len -= in_blk_len;
        } else {
            in_blk_len = temp_len;
            temp_len = 0;
        }
        // if h2t_blk block direct is not used, allocate a block
        if (h2t_blk[block_direct] == 0) {
            // find a free block
            int blk_num = find_free_block_number();
            if (blk_num < 0) {
                return written_length;
            }

            // change the h2t block and write back
            h2t_blk[block_direct] = blk_num;
            disk->ops->write(disk, root_blk, 1, h2t_blk);

            // set the block bitmap
            FD_SET(blk_num, block_map);
            update_bitmap();
        }
        int written_length_next_level = fs_write_2nd_level(h2t_blk[block_direct], in_blk_offset + N_DIRECT * BLOCK_SIZE, in_blk_len, buf);
        if(written_length_next_level == 0) {
            break;
        }
        buf += written_length_next_level;
        written_length += written_length_next_level;
    }

    return written_length;
}

void update_bitmap() {
    disk->ops->write(disk, 1, inode_map_sz, inode_map);
    disk->ops->write(disk, 1 + inode_map_sz, block_map_sz, block_map);
}


int find_free_block_number() {
    int i;
    for (i = 0; i < block_map_sz * BLOCK_SIZE * 8 && i < num_of_blocks; i++) {
        if (!FD_ISSET(i, block_map)) {
            int *clear_blk = calloc(1, BLOCK_SIZE);
            disk->ops->write(disk, i, 1, clear_blk);
            free(clear_blk);
            return i;
        }
    }

    return -ENOSPC;
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

