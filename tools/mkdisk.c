/*
 * mkdisk - copies a file onto an AshFS disk image
 * Run on the host Linux system, not inside AshOS
 *
 * Usage: ./mkdisk disk.img hello.elf hello.elf
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SECTOR_SIZE        512
#define SUPERBLOCK_SECTOR  0
#define INODE_START_SECTOR 1
#define DATA_START_SECTOR  33
#define MAX_FILES          32
#define MAX_FILE_BLOCKS    32
#define MAX_FILENAME       64
#define FS_MAGIC           0xA5075

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t total_blocks;
    uint32_t inode_start;
    uint32_t data_start;
    uint32_t free_blocks;
} superblock_t;

typedef struct {
    char     name[MAX_FILENAME];
    uint32_t size;
    uint32_t blocks[MAX_FILE_BLOCKS];
    uint8_t  used;
    uint8_t  padding[3];
} inode_t;

static uint8_t sector_buf[SECTOR_SIZE];

static void read_sector(FILE* f, uint32_t lba, uint8_t* buf)
{
    fseek(f, lba * SECTOR_SIZE, SEEK_SET);
    fread(buf, SECTOR_SIZE, 1, f);
}

static void write_sector(FILE* f, uint32_t lba, uint8_t* buf)
{
    fseek(f, lba * SECTOR_SIZE, SEEK_SET);
    fwrite(buf, SECTOR_SIZE, 1, f);
}

int main(int argc, char* argv[])
{
    if (argc != 4) {
        printf("Usage: %s <disk.img> <input_file> <dest_name>\n", argv[0]);
        return 1;
    }

    const char* disk     = argv[1];
    const char* src_file = argv[2];
    const char* dst_name = argv[3];

    /* open disk image */
    FILE* disk_f = fopen(disk, "r+b");
    if (!disk_f) {
        printf("Error: could not open disk image '%s'\n", disk);
        return 1;
    }

    /* read superblock */
    superblock_t sb;
    read_sector(disk_f, SUPERBLOCK_SECTOR, sector_buf);
    memcpy(&sb, sector_buf, sizeof(superblock_t));

    if (sb.magic != FS_MAGIC) {
        printf("Error: no AshFS found on disk image\n");
        fclose(disk_f);
        return 1;
    }

    printf("AshFS found, %d free blocks\n", sb.free_blocks);

    /* read source file */
    FILE* src_f = fopen(src_file, "rb");
    if (!src_f) {
        printf("Error: could not open '%s'\n", src_file);
        fclose(disk_f);
        return 1;
    }

    fseek(src_f, 0, SEEK_END);
    uint32_t file_size = ftell(src_f);
    fseek(src_f, 0, SEEK_SET);

    uint8_t* file_data = malloc(file_size);
    fread(file_data, file_size, 1, src_f);
    fclose(src_f);

    printf("File size: %d bytes\n", file_size);

    uint32_t blocks_needed = (file_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    if (blocks_needed > MAX_FILE_BLOCKS) {
        printf("Error: file too large (max %d blocks)\n", MAX_FILE_BLOCKS);
        free(file_data);
        fclose(disk_f);
        return 1;
    }

    /* find free inode */
    inode_t inode;
    int inode_index = -1;
    int inodes_per_sector = SECTOR_SIZE / sizeof(inode_t);

    for (int i = 0; i < MAX_FILES; i++) {
        uint32_t sector = INODE_START_SECTOR + (i / inodes_per_sector);
        uint32_t offset = i % inodes_per_sector;
        read_sector(disk_f, sector, sector_buf);
        memcpy(&inode, sector_buf + offset * sizeof(inode_t), sizeof(inode_t));
        if (!inode.used) {
            inode_index = i;
            break;
        }
    }

    if (inode_index < 0) {
        printf("Error: no free inodes\n");
        free(file_data);
        fclose(disk_f);
        return 1;
    }

    /* find free data blocks and write file */
    uint8_t used_blocks[1024] = {0};

    /* mark used blocks */
    for (int i = 0; i < MAX_FILES; i++) {
        uint32_t sector = INODE_START_SECTOR + (i / inodes_per_sector);
        uint32_t offset = i % inodes_per_sector;
        read_sector(disk_f, sector, sector_buf);
        inode_t tmp;
        memcpy(&tmp, sector_buf + offset * sizeof(inode_t), sizeof(inode_t));
        if (tmp.used) {
            for (int j = 0; j < MAX_FILE_BLOCKS; j++) {
                if (tmp.blocks[j])
                    used_blocks[tmp.blocks[j] - DATA_START_SECTOR] = 1;
            }
        }
    }

    /* set up new inode */
    memset(&inode, 0, sizeof(inode_t));
    strncpy(inode.name, dst_name, MAX_FILENAME - 1);
    inode.size = file_size;
    inode.used = 1;

    /* allocate blocks and write data */
    uint32_t written = 0;
    for (uint32_t i = 0; i < blocks_needed; i++) {
        /* find free block */
        uint32_t block = 0;
        for (uint32_t b = 0; b < sb.total_blocks; b++) {
            if (!used_blocks[b]) {
                block = DATA_START_SECTOR + b;
                used_blocks[b] = 1;
                sb.free_blocks--;
                break;
            }
        }

        if (!block) {
            printf("Error: disk full\n");
            free(file_data);
            fclose(disk_f);
            return 1;
        }

        inode.blocks[i] = block;

        memset(sector_buf, 0, SECTOR_SIZE);
        uint32_t to_write = file_size - written;
        if (to_write > SECTOR_SIZE) to_write = SECTOR_SIZE;
        memcpy(sector_buf, file_data + written, to_write);
        write_sector(disk_f, block, sector_buf);
        written += to_write;
    }

    /* write inode */
    uint32_t inode_sector = INODE_START_SECTOR + (inode_index / inodes_per_sector);
    uint32_t inode_offset = inode_index % inodes_per_sector;
    read_sector(disk_f, inode_sector, sector_buf);
    memcpy(sector_buf + inode_offset * sizeof(inode_t), &inode, sizeof(inode_t));
    write_sector(disk_f, inode_sector, sector_buf);

    /* update superblock */
    memset(sector_buf, 0, SECTOR_SIZE);
    memcpy(sector_buf, &sb, sizeof(superblock_t));
    write_sector(disk_f, SUPERBLOCK_SECTOR, sector_buf);

    free(file_data);
    fclose(disk_f);

    printf("Successfully copied '%s' to AshFS as '%s'\n", src_file, dst_name);
    return 0;
}
