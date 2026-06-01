#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stddef.h>

#define FS_MAGIC        0xA5H05     /* AshFS magic number */
#define MAX_FILES       32          /* max files on disk */
#define MAX_FILE_BLOCKS 32           /* max data blocks per file */
#define BLOCK_SIZE      512         /* one sector per block */
#define MAX_FILENAME    64          /* max filename length */

/* superblock - lives at sector 0 */
typedef struct {
    uint32_t magic;         /* identifies this as AshFS */
    uint32_t version;       /* fs version */
    uint32_t total_blocks;  /* total blocks on disk */
    uint32_t inode_start;   /* sector where inode table starts */
    uint32_t data_start;    /* sector where data blocks start */
    uint32_t free_blocks;   /* number of free data blocks */
} __attribute__((packed)) superblock_t;

/* inode - describes a single file */
typedef struct {
    char     name[MAX_FILENAME]; /* file name */
    uint32_t size;               /* file size in bytes */
    uint32_t blocks[MAX_FILE_BLOCKS]; /* data block numbers */
    uint8_t  used;               /* 1 = in use, 0 = free */
    uint8_t  padding[3];         /* align to 4 bytes */
} __attribute__((packed)) inode_t;

/* file handle for reading */
typedef struct {
    inode_t  inode;     /* copy of the inode */
    uint32_t pos;       /* current read position */
} file_t;

void     fs_init(void);
int      fs_create(const char* name);
int      fs_write(const char* name, const uint8_t* data, uint32_t size);
int      fs_read(const char* name, uint8_t* buf, uint32_t size);
int      fs_delete(const char* name);
void     fs_list(void);

#endif
