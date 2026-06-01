#include "../include/ata.h"
#include <stdint.h>

extern void kprintf(const char* fmt, ...);

/* ATA primary bus ports */
#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECTOR_CNT  0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

/* ATA status bits */
#define ATA_SR_BSY  0x80  /* busy */
#define ATA_SR_DRQ  0x08  /* data request ready */
#define ATA_SR_ERR  0x01  /* error */

/* ATA commands */
#define ATA_CMD_READ  0x20
#define ATA_CMD_WRITE 0x30

static inline uint8_t inb(uint16_t port)
{
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outw(uint16_t port, uint16_t value)
{
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

/* wait until drive is not busy */
static void ata_wait(void)
{
    while (inb(ATA_STATUS) & ATA_SR_BSY);
}

/* wait until drive is ready for data */
static void ata_wait_drq(void)
{
    while (!(inb(ATA_STATUS) & ATA_SR_DRQ));
}

void ata_init(void)
{
    /* select master drive */
    outb(ATA_DRIVE_HEAD, 0xA0);
    ata_wait();
    kprintf("ATA: initialised primary bus\n");
}

void ata_read_sector(uint32_t lba, uint8_t* buf)
{
    ata_reset();
    ata_wait();

    /* set up LBA addressing */
    outb(ATA_DRIVE_HEAD,  0xE0 | ((lba >> 24) & 0x0F)); /* LBA mode, master */
    outb(ATA_SECTOR_CNT,  1);                             /* read 1 sector */
    outb(ATA_LBA_LOW,     lba & 0xFF);
    outb(ATA_LBA_MID,     (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH,    (lba >> 16) & 0xFF);
    outb(ATA_COMMAND,     ATA_CMD_READ);

    ata_wait_drq();

    /* read 256 words = 512 bytes */
    uint16_t* buf16 = (uint16_t*)buf;
    for (int i = 0; i < 256; i++)
        buf16[i] = inw(ATA_DATA);
}

void ata_write_sector(uint32_t lba, uint8_t* buf)
{
    ata_wait();

    outb(ATA_DRIVE_HEAD,  0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_CNT,  1);
    outb(ATA_LBA_LOW,     lba & 0xFF);
    outb(ATA_LBA_MID,     (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH,    (lba >> 16) & 0xFF);
    outb(ATA_COMMAND,     ATA_CMD_WRITE);

    ata_wait_drq();

    /* write 256 words = 512 bytes */
    uint16_t* buf16 = (uint16_t*)buf;
    for (int i = 0; i < 256; i++)
        outw(ATA_DATA, buf16[i]);

    /* flush write cache */
    outb(ATA_COMMAND, 0xE7);
    ata_wait();
}

void ata_reset(void)
{
    outb(ATA_DRIVE_HEAD, 0xA0);
    /* wait */
    for (int i = 0; i < 10000; i++)
        __asm__ volatile ("nop");
    ata_wait();
}
