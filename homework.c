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
struct fs5600_inode *inode_region;	/* inodes in memory */
void update_bitmap(void);

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
    printf("DEBUG: inode map size is: %d\n", sb.inode_map_sz);

    block_map = malloc(sb.block_map_sz * FS_BLOCK_SIZE);
    disk->ops->read(disk, sb.inode_map_sz + 1, sb.block_map_sz, block_map);
    block_map_sz = sb.block_map_sz;
    printf("DEBUG: block map size is: %d\n", sb.block_map_sz);

    /* read inodes */
    inode_region = malloc(sb.inode_region_sz * FS_BLOCK_SIZE);
    int inode_region_pos = 1 + sb.inode_map_sz + sb.block_map_sz;
    disk->ops->read(disk, inode_region_pos, sb.inode_region_sz, inode_region);

    printf("DEBUG: inode_region_sz is: %d\n", sb.inode_region_sz);
    inode_region_sz = sb.inode_region_sz;

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
    /* traverse all the subsides */
    /* if found, return corresponding inode */
    /* else, return error */
    while (token != NULL) {
        if (current_dir->valid == 0) {
	        return -ENOENT;
	    }
	    if (current_dir->isDir == 0) {
	        token = strtok(NULL, delim);
	        if (token == NULL) {
		    break;
            } else {
		        return -ENOTDIR;
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
            return -ENOENT;
	    }
        token = strtok(NULL, delim);

    }

    free(dir);
    free(_path);
//    printf("DEBUG: inum inside translation: %d\n", inode_num);
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
    sb->st_size = inode.size;
    sb->st_blocks = 1 + ((inode.size - 1) / FS_BLOCK_SIZE);
    sb->st_nlink = 1;
    sb->st_atime = inode.ctime;
    sb->st_ctime = inode.ctime;
    sb->st_mtime = inode.ctime;
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
    if (inum == -ENOENT) {
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
    printf("DEBUG: path is %s\n", path);
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
    dir = malloc(FS_BLOCK_SIZE);
    printf("-----------------------------------\n");
    printf("translated and 2nd ls\n");
//    sleep(1);
//    fs_init(NULL);
    inode = &inode_region[inum];
    // assert is dir
    printf("DEBUG: inode mode is %d\n", (int)inode->mode);
//    sleep(2);
    assert(S_ISDIR(inode->mode));
    int block_pos = inode->direct[0];
    disk->ops->read(disk, block_pos, 1, dir);
    printf("read into block\n");
    int curr_inum;
    struct fs5600_inode curr_inode;

//    int *crazy;
    struct stat sb;
//            = malloc(sizeof(struct stat));

    int i;
    for (i = 0; i < 32; i++) {
//        printf("DEBUG: name: %s, valid: %d, isDir: %d, inum: %d\n",
//               dir[i].name, dir[i].valid, dir[i].isDir, dir[i].inode);
    	if (dir[i].valid == 0) {
    	    continue;
    	}

//        fs_init(NULL);
//        inode = &inode_region[inum];
//        // assert is dir
//        assert(S_ISDIR(inode->mode));
//        int block_pos = inode->direct[0];
//        disk->ops->read(disk, block_pos, 1, dir);


    	curr_inum = dir[i].inode;
        printf("curr inum is : %d\n", curr_inum);
//    	update_inode(curr_inum);
        curr_inode = inode_region[curr_inum];

//
//        printf("current inode info is:\n mode: %d, uid: %d, size: %d, ctime: %d\n",
//                                            curr_inode->mode, curr_inode->uid, curr_inode->size, curr_inode->ctime);
        printf("current inode name is: %s\n", dir[i].name);
    	set_attr(curr_inode, &sb);
        printf("before calling filler\n");
//        sleep(1);
//    	filler(NULL, dir[i].name, &sb, 0);
//        char *name = NULL;
//        strcpy(name, dir[i].name);
//        printf("DEBUG: the name is %s\n", name);
    	filler(ptr, dir[i].name, &sb, 0);
//        crazy = malloc(1024000);
//        crazy[0] = 0;
    }
    printf("before freeing sb\n");
//    free(sb);
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
    // printf("DEBUG: entering fs_mknod\n");
    // printf("DEBUG: path is %s\n", path);
    // check permission, !S_ISREG(mode)

    if (!S_ISREG(mode)) {
        return -EINVAL;
    }
    // check father dir exist
    char *father_path;
    if (!trancate_path(path, &father_path)) {
        // this means there is no nod to make, path is "/"
        return -1;
    }
    // printf("DEBUG: trancated path is : %s\n", father_path);
    int dir_inum = translate(father_path);
    // printf("DEBUG: father_inode number is %d\n", dir_inum);
    if (dir_inum == -ENOENT || dir_inum == -ENOTDIR) {
        // printf("DEBUG: a component is not present or intermediate component is not directory");
        return -EEXIST;
    }
    // check if dest file exists
    int inum = translate(path);
    // printf("DEBUG: inum : %d\n", inum);
    if (inum > 0) {
        return -EEXIST;
    }
    // check entries in father dir not excceed 32
    // printf("DEBUG: checking excceeds 32\n");
    struct fs5600_inode *father_inode = &inode_region[dir_inum];
    int free_dirent_num = find_free_dirent_num(father_inode);
    if(free_dirent_num < 0) {
        return -ENOSPC;
    }

    // printf("DEBUG: after check excceeds 32\n");
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
    // printf("DEBUG: before find free father_inode map\n");
    int free_inum = find_free_inode_map_bit();
    // printf("DEBUG: after find free father_inode map\n");
    if (free_inum < 0) {
        return -ENOSPC;
    }
    FD_SET(free_inum, inode_map);
    update_bitmap();

    // printf("DEBUG: before written back\n");
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
    // printf("DEBUG: before write, dir_blk name is: %s\n", dir_blk[free_dirent_num].name);
    // printf("DEBUG: before write, father_inode->direct[0] is: %d\n", father_inode->direct[0]);
    disk->ops->write(disk, father_inode->direct[0], 1, dir_blk);
    free(dir_blk);
    free(_path);
    // printf("DEBUG: father path is: %s\n", father_path);
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
    printf("DEBUG: result is %s\n", result);
    return result;
}

int find_free_inode_map_bit() {// find a free inode_region
    int inode_capacity = inode_map_sz * FS_BLOCK_SIZE * 8;
    // printf("DEBUG: capa is: %d\n", inode_capacity);
    int i;
    for (i = 2; i < inode_capacity; i++) {
        if (!FD_ISSET(i, inode_map)) {
            return i;
        }
    }
    // printf("DEBUG: inode map full.\n");
    return -ENOSPC;
}

int find_free_dirent_num(struct fs5600_inode *inode) {
    struct fs5600_dirent *dir = (struct fs5600_dirent *)malloc(BLOCK_SIZE);
    disk->ops->read(disk, (inode->direct)[0], 1, dir);

    int free_dirent_num = -1;
    int i;
    for (i = 0; i < 32; i++) {
//        printf("%dth dirent in root directory is %d\n", i, dir->valid);
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
    // printf("DEBUG: entering fs_mkdir\n");
    // printf("DEBUG: path is %s\n", path);
    // check permission, !S_ISREG(mode)
    mode = mode | S_IFDIR;
    if (!S_ISDIR(mode)) {
    	// printf("DEBUG: mode is %d\n", (int)mode);
        return -EINVAL;
    }
    /*check father dir exist*/
    char *father_path;
    if (!trancate_path(path, &father_path)) {
        // this means there is no nod to make, path is "/"
        return -1;
    }
    // printf("DEBUG: trancated path is : %s\n", father_path);
    int dir_inum = translate(father_path);
    // printf("DEBUG: father_inode number is %d\n", dir_inum);
    if (dir_inum == -ENOENT || dir_inum == -ENOTDIR) {
        // printf("DEBUG: a component is not present or intermediate component is not directory");
        return -EEXIST;
    }
    // check if dest file exists
    int inum = translate(path);
    // printf("DEBUG: inum : %d\n", inum);
    if (inum > 0) {
        return -EEXIST;
    }
    // check entries in father dir not excceed 32
    // printf("DEBUG: checking excceeds 32\n");
    struct fs5600_inode *father_inode = &inode_region[dir_inum];
    int free_dirent_num = find_free_dirent_num(father_inode);
    if(free_dirent_num < 0) {
        return -ENOSPC;
    }

    // printf("DEBUG: after check excceeds 32\n");
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
    // printf("DEBUG: before find free father_inode map\n");
    int free_inum = find_free_inode_map_bit();
    // printf("DEBUG: after find free father_inode map\n");
    if (free_inum < 0) {
        return -ENOSPC;
    }
    FD_SET(free_inum, inode_map);
    update_bitmap();

    // printf("DEBUG: before written back\n");
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
    // printf("DEBUG: before write, dir_blk name is: %s\n", dir_blk[free_dirent_num].name);
    // printf("DEBUG: before write, father_inode->direct[0] is: %d\n", father_inode->direct[0]);
    disk->ops->write(disk, father_inode->direct[0], 1, dir_blk);

    free(dir_blk);
    free(_path);
    // printf("DEBUG: father path is: %s\n", father_path);
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
    for (i = 0; i < 6; i++) {
        temp_blk_num = inode->direct[i];
        if (temp_blk_num != 0) {
            FD_CLR(temp_blk_num, block_map);
            update_bitmap();
        } else {
            break;
        }
    }
    if (inode->size >= (BLOCK_SIZE / 4) * BLOCK_SIZE) {
        truncate_2nd_level(inode->indir_1);
    }

    if (inode->size >= (BLOCK_SIZE / 4) * (BLOCK_SIZE / 4) * BLOCK_SIZE) {
        truncate_3rd_level(inode->indir_2);
    }

    // set the size of inode as 0
    inode_region[inum].size = 0;
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
            update_bitmap();
        } else {
            break;
        }
    }
}

void truncate_3rd_level(int h2t_root_blk_num) {
    int h2t_blk[256];
    disk->ops->read(disk, h2t_root_blk_num, 1, h2t_blk);
    int i;
    for (i = 0; i < 256; ++i) {
        truncate_2nd_level(h2t_blk[i]);
    }
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
            printf("DEBUG: father dir order is: %d\n", i);
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
    printf("DEBUG: name is %s\n", name);
    int father_inum = translate(father_path);
    free(father_path);
    struct fs5600_inode *father_inode = &inode_region[father_inum];
    struct fs5600_dirent *father_dirent = malloc(FS_BLOCK_SIZE);
    disk->ops->read(disk, father_inode->direct[0], 1, father_dirent);
    for (i = 0; i < 32; ++i) {
        printf("DEBUG: %dth name is %s\n", i, father_dirent[i].name);
        if (strcmp(father_dirent[i].name, name) == 0) {
            printf("DEBUG: father dir order is: %d\n", i);
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
	// printf("DEBUG: src_path is %s\n", src_path);
	// printf("DEBUG: dst_path is %s\n", dst_path);
	src_inum = translate(src_path);
	dst_inum = translate(dst_path);
	// printf("DEBUG: src_inum is %d\n", src_inum);
	// printf("DEBUG: dst_inum is %d\n", dst_inum);
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
   		return -ENOENT;
   	}
   	if (strcmp(src_father_path, dst_father_path) != 0) {
   		return -EINVAL;
   	}
   	int father_inum;
   	if (!(father_inum = translate(src_father_path))) {
   		return father_inum;
   	}
   	// printf("DEBUG: father_inum is %d\n", father_inum);
   	/*get the name of the src and dst path*/
   	char *_src_path = strdup(src_path);
   	char *src_name = get_name(_src_path);
   	// printf("DEBUG: src_name is %s\n", src_name);
   	char *_dst_path = strdup(dst_path);
   	char *dst_name = get_name(_dst_path);

   	/*load dirent block to memory to search src file name*/
   	struct fs5600_inode *inode;
    struct fs5600_dirent *dir;
    dir = malloc(FS_BLOCK_SIZE);

    inode = &inode_region[father_inum];
    // assert is dir
    assert(S_ISDIR(inode->mode));
    int block_pos = inode->direct[0];
    disk->ops->read(disk, block_pos, 1, dir);
    int i;
    int result = -ENOENT;
    // printf("DEBUG: start search for name and change name\n");
    /*traverse the drient block to find the dirent with the same name*/
    for (i = 0;i < 32; i++) {
    	// printf("DEBUG: %dth iteration, dir.name is %s, file name is %s\n", i, dir[i].name, src_name);
    	if (dir[i].valid == 1 && strcmp(dir[i].name, src_name) == 0) {
    		// printf("DEBUG: got the file and changing name, %s\n", dir[i].name);
    		strncpy(dir[i].name, dst_name, strlen(dst_name));
    		// printf("DEBUG: change the name to %s\n", dir[i].name);
    		dir[i].name[strlen(dst_name)] = '\0';
    		disk->ops->write(disk, block_pos, 1, dir);
    		// printf("DEBUG: we have finished rename\n");
    		result = 0;
    	}
    }
    assert(result == 0);
    free(_src_path);
    free(_dst_path);
    // free(dst_name);
    // free(src_name);
   	free(src_father_path);
   	free(dst_father_path);
   	// printf("DEBUG: returning %d\n", result);
    return result;
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
    return -EOPNOTSUPP;
}


/*
 * given block number, offset, length and return buffer, load corresponding data into buffer
 *
 */
static int fs_read_block(int blknum, int offset, int len, char *buf);
//static int fs_read_file_by_inode();

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

    int inum = translate(path);
    const struct fs5600_inode *inode = &inode_region[inum];
    assert(S_ISREG(inode->mode));// assert path is a file
    int size = inode->size;
    if (offset >= size) {
        return 0;
    }
    if (offset + len > size) {
        len = size - offset;
    }
    int tmp_len = len;
    if (offset < 6 * BLOCK_SIZE) {
        int read_len = fs_read_1st_level(inode, offset, tmp_len, buf);
        offset += read_len;
        tmp_len -= read_len;
        buf += read_len;
    }
    if (offset >= 6 * BLOCK_SIZE && offset < BLOCK_SIZE / 4 * BLOCK_SIZE + 6 * BLOCK_SIZE) {
        int read_len = fs_read_2nd_level(inode->indir_1, offset, tmp_len, buf);
        offset += read_len;
        tmp_len -= read_len;
        buf += read_len;
    }
    if (offset >= BLOCK_SIZE / 4 * BLOCK_SIZE + 6 * BLOCK_SIZE && offset <= (BLOCK_SIZE / 4) * (BLOCK_SIZE / 4) * BLOCK_SIZE) {
        fs_read_3rd_level(inode->indir_2, offset, tmp_len, buf);
    }
    return len;
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
    for (; block_direct < 6 && temp_len > 0; in_blk_offset = 0, block_direct++) {
        if (temp_len + in_blk_offset > BLOCK_SIZE) {
            in_blk_len = BLOCK_SIZE - in_blk_offset;
            temp_len -= in_blk_len;
        } else {
            in_blk_len = temp_len;
            temp_len = 0;
        }
        fs_read_block(inode->direct[block_direct], in_blk_offset, in_blk_len, buf);
        buf += in_blk_len;
        read_length += in_blk_len;
    }
    return read_length;
}

static int fs_read_2nd_level(size_t root_blk, int offset, int len, char *buf){
    int read_length = 0;
    int h1t_offset = offset - 6 * BLOCK_SIZE; // height 1 tree offset
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
        fs_read_block(h1t_blk[block_direct], in_blk_offset, in_blk_len, buf);
        buf += in_blk_len;
        read_length += in_blk_len;
    }
    return read_length;
}

static int fs_read_3rd_level(size_t root_blk, int offset, int len, char *buf){
    int read_length = 0;
    int h1t_block_num = BLOCK_SIZE / sizeof(int *);
    int h1t_block_size = (BLOCK_SIZE * h1t_block_num);
    int h2t_offset = offset - 6 * BLOCK_SIZE - h1t_block_size;// height 2 tree offset
    int block_direct = h2t_offset / h1t_block_size;
    int temp_len = len;
    int in_blk_len;
    int in_blk_offset = h2t_offset % h1t_block_size;


    int h2t_blk[256];
    disk->ops->read(disk, root_blk, 1, h2t_blk);

    for (; block_direct < 256 && temp_len > 0; in_blk_offset = 0, block_direct++) {
        if (temp_len + in_blk_offset > h1t_block_size) {
            in_blk_len = h1t_block_size - in_blk_offset;
            temp_len -= in_blk_len;
        } else {
            in_blk_len = temp_len;
            temp_len = 0;
        }
        fs_read_2nd_level(h2t_blk[block_direct], in_blk_offset + 6 * BLOCK_SIZE, in_blk_len, buf);
        buf += in_blk_len;
        read_length += in_blk_len;
    }
    return read_length;
}

static int fs_read_block(int blknum, int offset, int len, char *buf) {
    char *blk = (char*) malloc(BLOCK_SIZE);
    assert(blknum > 0);
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
    if (tmp_offset < 6 * BLOCK_SIZE) {
        int written_len = fs_write_1st_level(inum, tmp_offset, tmp_len, buf);
        tmp_offset += written_len;
        tmp_len -= written_len;
        buf += written_len;
    }
    if (tmp_offset >= 6 * BLOCK_SIZE && tmp_offset < BLOCK_SIZE / 4 * BLOCK_SIZE + 6 * BLOCK_SIZE) {

        if (inode->indir_1 == 0) {// if indir_1 not set, set it
            // find a free block
            int blk_num = find_free_block_number();
            if (blk_num < 0) {
                assert(blk_num > 0);
                return -ENOSPC;
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
    if (tmp_offset >= BLOCK_SIZE / 4 * BLOCK_SIZE + 6 * BLOCK_SIZE && tmp_offset <= (BLOCK_SIZE / 4) * (BLOCK_SIZE / 4) * BLOCK_SIZE) {
        if (inode->indir_2 == 0) { // if indir_2 not set, set it
            // find a free block
            int blk_num = find_free_block_number();
            if (blk_num < 0) {
                assert(blk_num > 0);
                return -ENOSPC;
            }
            /*change the inode*/
            inode->indir_2 = blk_num;
            update_inode(inum);
            /*set the block bitmap*/
            FD_SET(blk_num, block_map);
            update_bitmap();
        }
        fs_write_3rd_level(inode->indir_2, tmp_offset, tmp_len, buf);
    }
    /* update inode size */
    if (offset + len > inode->size) {
        inode->size = offset + len;
        update_inode(inum);
    }
    return len;
    return -EOPNOTSUPP;
}

static int fs_write_1st_level(int inum, off_t offset, size_t len, const char *buf) {
    int written_length = 0;
    struct fs5600_inode *inode = &inode_region[inum];
    int block_direct = offset / BLOCK_SIZE;
    int temp_len = len;
    int in_blk_len;
    int in_blk_offset = offset % BLOCK_SIZE;

    for (; block_direct < 6 && temp_len > 0; in_blk_offset = 0, block_direct++) {
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
                assert(blk_num > 0);
                return -ENOSPC;
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
        buf += in_blk_len;
        written_length += in_blk_len;
    }
    return written_length;
}

static int fs_write_2nd_level(size_t root_blk, int offset, int len, const char *buf){
    int written_length = 0;
    int h1t_offset = offset - 6 * BLOCK_SIZE; // height 1 tree offset
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
                assert(blk_num > 0);
                return -ENOSPC;
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
    int h1t_block_num = BLOCK_SIZE / sizeof(int *);
    int h1t_block_size = (BLOCK_SIZE * h1t_block_num);
    int h2t_offset = offset - 6 * BLOCK_SIZE - h1t_block_size;// height 2 tree offset
    int block_direct = h2t_offset / h1t_block_size;
    int temp_len = len;
    int in_blk_len;
    int in_blk_offset = h2t_offset % h1t_block_size;

    int h2t_blk[256];
    disk->ops->read(disk, root_blk, 1, h2t_blk);

    for (; block_direct < 256 && temp_len > 0; in_blk_offset = 0, block_direct++) {
        if (temp_len + in_blk_offset > h1t_block_size) {
            in_blk_len = h1t_block_size - in_blk_offset;
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
                assert(blk_num > 0);
                return -ENOSPC;
            }

            // change the h2t block and write back
            h2t_blk[block_direct] = blk_num;
            disk->ops->write(disk, root_blk, 1, h2t_blk);

            // set the block bitmap
            FD_SET(blk_num, block_map);
            update_bitmap();
        }
        fs_write_2nd_level(h2t_blk[block_direct], in_blk_offset + 6 * BLOCK_SIZE, in_blk_len, buf);
        buf += in_blk_len;
        written_length += in_blk_len;
    }

    return written_length;
}

void update_bitmap() {
    disk->ops->write(disk, 1, inode_map_sz, inode_map);
    disk->ops->write(disk, 1 + inode_map_sz, block_map_sz, block_map);
}


int find_free_block_number() {
    int i;
    for (i = 0; i < block_map_sz * BLOCK_SIZE * sizeof(char); i++) {
                if (!FD_ISSET(i, block_map)) {
                    return i;
                }
            }
    return -ENOSPC;
}

//static int fs_write_block(int blknum, int offset, int len, char *buf) {
//    char *blk = (char*) malloc(BLOCK_SIZE);
//    assert(blknum > 0);
//    disk->ops->read(disk, blknum, 1, blk);
//    char *blk_ptr = blk;
//    blk_ptr += offset;
//    memcpy(buf, blk_ptr, len);
//    free(blk);
//    return 0;
//}

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

