#include "../include/fs.h"
#include "../include/ata.h"
#include "../include/heap.h"
#include <stdint.h>
#include <stddef.h>

extern void kprintf(const char* fmt, ...);

/* layout on disk:
 * sector 0        - superblock
 * sectors 1-32    - inode table (32 inodes)
 * sectors 33+     - data blocks
 */
#define SUPERBLOCK_SECTOR  0
#define INODE_START_SECTOR 1
#define DATA_START_SECTOR  33
#define INODES_PER_SECTOR  (512 / sizeof(inode_t))

static superblock_t superblock;
static uint8_t sector_buf[512];

/* ============================================================
   String helpers (no stdlib)
   ============================================================ */

static int fs_strcmp(const char* a, const char* b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static void fs_strcpy(char* dst, const char* src, size_t max)
{
    size_t i = 0;
    while (i < max - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static void fs_memset(void* ptr, uint8_t val, size_t size)
{
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < size; i++)
        p[i] = val;
}

static void fs_memcpy(void* dst, const void* src, size_t size)
{
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < size; i++)
        d[i] = s[i];
}

/* ============================================================
   Inode helpers
   ============================================================ */

/*
 * Read inode at index from disk.
 */
static void inode_read(uint32_t index, inode_t* inode)
{
    uint32_t sector = INODE_START_SECTOR + (index / INODES_PER_SECTOR);
    uint32_t offset = index % INODES_PER_SECTOR;

    ata_read_sector(sector, sector_buf);
    fs_memcpy(inode, sector_buf + offset * sizeof(inode_t), sizeof(inode_t));
}

/*
 * Write inode at index to disk.
 */
static void inode_write(uint32_t index, inode_t* inode)
{
    uint32_t sector = INODE_START_SECTOR + (index / INODES_PER_SECTOR);
    uint32_t offset = index % INODES_PER_SECTOR;

    ata_read_sector(sector, sector_buf);
    fs_memcpy(sector_buf + offset * sizeof(inode_t), inode, sizeof(inode_t));
    ata_write_sector(sector, sector_buf);
}

/*
 * Find inode by name. Returns index or -1 if not found.
 */
static int inode_find(const char* name)
{
    inode_t inode;
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        inode_read(i, &inode);
        if (inode.used && fs_strcmp(inode.name, name) == 0)
            return (int)i;
    }
    return -1;
}

/*
 * Find a free inode slot. Returns index or -1 if full.
 */
static int inode_alloc(void)
{
    inode_t inode;
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        inode_read(i, &inode);
        if (!inode.used)
            return (int)i;
    }
    return -1;
}

/*
 * Allocate a free data block. Returns block number or 0 if full.
 */
static uint32_t block_alloc(void)
{
    if (superblock.free_blocks == 0) return 0;

    /* simple linear scan for a free block */
    /* in a real fs we'd use a bitmap */
    inode_t inode;
    uint8_t used[superblock.total_blocks];
    fs_memset(used, 0, superblock.total_blocks);

    /* mark all used blocks */
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        inode_read(i, &inode);
        if (inode.used) {
            for (int j = 0; j < MAX_FILE_BLOCKS; j++) {
                if (inode.blocks[j])
                    used[inode.blocks[j] - DATA_START_SECTOR] = 1;
            }
        }
    }

    /* find first free block */
    for (uint32_t i = 0; i < superblock.total_blocks; i++) {
        if (!used[i]) {
            superblock.free_blocks--;
            return DATA_START_SECTOR + i;
        }
    }
    return 0;
}

/* ============================================================
   Public API
   ============================================================ */

void fs_init(void)
{
    /* read superblock */
    ata_read_sector(SUPERBLOCK_SECTOR, sector_buf);
    fs_memcpy(&superblock, sector_buf, sizeof(superblock_t));

    if (superblock.magic != 0xA5075) {
        /* no filesystem found - format the disk */
        kprintf("FS: no AshFS found, formatting...\n");

        fs_memset(&superblock, 0, sizeof(superblock_t));
        superblock.magic        = 0xA5075;
        superblock.version      = 1;
        superblock.total_blocks = 1024; /* 512KB of data space */
        superblock.inode_start  = INODE_START_SECTOR;
        superblock.data_start   = DATA_START_SECTOR;
        superblock.free_blocks  = 1024;

        /* write superblock */
        fs_memset(sector_buf, 0, 512);
        fs_memcpy(sector_buf, &superblock, sizeof(superblock_t));
        ata_write_sector(SUPERBLOCK_SECTOR, sector_buf);

        /* zero out inode table */
        fs_memset(sector_buf, 0, 512);
        for (uint32_t i = 0; i < 32; i++)
            ata_write_sector(INODE_START_SECTOR + i, sector_buf);

        kprintf("FS: AshFS formatted successfully\n");
    } else {
        kprintf("FS: AshFS found, version %d, %d free blocks\n",
                superblock.version, superblock.free_blocks);
    }
}

int fs_create(const char* name)
{
    if (inode_find(name) >= 0) {
        kprintf("FS: file '%s' already exists\n", name);
        return -1;
    }

    int index = inode_alloc();
    if (index < 0) {
        kprintf("FS: no free inodes\n");
        return -1;
    }

    inode_t inode;
    fs_memset(&inode, 0, sizeof(inode_t));
    fs_strcpy(inode.name, name, MAX_FILENAME);
    inode.size = 0;
    inode.used = 1;

    inode_write((uint32_t)index, &inode);
    kprintf("FS: created '%s'\n", name);
    return 0;
}

int fs_write(const char* name, const uint8_t* data, uint32_t size)
{
    int index = inode_find(name);
    if (index < 0) {
        kprintf("FS: file '%s' not found\n", name);
        return -1;
    }

    inode_t inode;
    inode_read((uint32_t)index, &inode);

    uint32_t blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (blocks_needed > MAX_FILE_BLOCKS) {
        kprintf("FS: file too large\n");
        return -1;
    }

    /* allocate blocks and write data */
    uint32_t written = 0;
    for (uint32_t i = 0; i < blocks_needed; i++) {
        uint32_t block = block_alloc();
        if (!block) {
            kprintf("FS: disk full\n");
            return -1;
        }

        inode.blocks[i] = block;

        /* copy data into sector buffer */
        fs_memset(sector_buf, 0, 512);
        uint32_t to_write = size - written;
        if (to_write > BLOCK_SIZE) to_write = BLOCK_SIZE;
        fs_memcpy(sector_buf, data + written, to_write);
        ata_write_sector(block, sector_buf);
        written += to_write;
    }

    inode.size = size;
    inode_write((uint32_t)index, &inode);

    /* update superblock */
    fs_memset(sector_buf, 0, 512);
    fs_memcpy(sector_buf, &superblock, sizeof(superblock_t));
    ata_write_sector(SUPERBLOCK_SECTOR, sector_buf);

    kprintf("FS: wrote %d bytes to '%s'\n", size, name);
    return 0;
}

int fs_read(const char* name, uint8_t* buf, uint32_t size)
{
    int index = inode_find(name);
    if (index < 0) {
        kprintf("FS: file '%s' not found\n", name);
        return -1;
    }

    inode_t inode;
    inode_read((uint32_t)index, &inode);

    uint32_t to_read = size < inode.size ? size : inode.size;
    uint32_t read    = 0;

    for (int i = 0; i < MAX_FILE_BLOCKS && read < to_read; i++) {
        if (!inode.blocks[i]) break;

        ata_read_sector(inode.blocks[i], sector_buf);
        uint32_t chunk = to_read - read;
        if (chunk > BLOCK_SIZE) chunk = BLOCK_SIZE;
        fs_memcpy(buf + read, sector_buf, chunk);
        read += chunk;
    }

    return (int)read;
}

int fs_delete(const char* name)
{
    int index = inode_find(name);
    if (index < 0) {
        kprintf("FS: file '%s' not found\n", name);
        return -1;
    }

    inode_t inode;
    inode_read((uint32_t)index, &inode);

    /* free blocks */
    for (int i = 0; i < MAX_FILE_BLOCKS; i++) {
        if (inode.blocks[i]) {
            superblock.free_blocks++;
            inode.blocks[i] = 0;
        }
    }

    inode.used = 0;
    inode_write((uint32_t)index, &inode);

    /* update superblock */
    fs_memset(sector_buf, 0, 512);
    fs_memcpy(sector_buf, &superblock, sizeof(superblock_t));
    ata_write_sector(SUPERBLOCK_SECTOR, sector_buf);

    kprintf("FS: deleted '%s'\n", name);
    return 0;
}

void fs_list(void)
{
    inode_t inode;
    int found = 0;

    kprintf("\nFiles on AshFS:\n");
    kprintf("Name                             Size\n");
    kprintf("----                             ----\n");

    for (uint32_t i = 0; i < MAX_FILES; i++) {
        inode_read(i, &inode);
        if (inode.used) {
            kprintf("  %s (%d bytes)\n", inode.name, inode.size);
            found++;
        }
    }

    if (!found)
        kprintf("(no files)\n");
}
