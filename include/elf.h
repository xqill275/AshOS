#ifndef ELF_H
#define ELF_H

#include <stdint.h>

/* ELF magic number */
#define ELF_MAGIC 0x464C457F  /* 0x7F followed by 'E', 'L', 'F' */

/* ELF types */
#define ET_EXEC   2  /* executable file */

/* ELF machine types */
#define EM_386    3  /* x86 */

/* program header types */
#define PT_LOAD   1  /* loadable segment */

/* ELF header */
typedef struct {
    uint32_t magic;         /* 0x7F 'E' 'L' 'F' */
    uint8_t  bits;          /* 1 = 32-bit, 2 = 64-bit */
    uint8_t  endian;        /* 1 = little, 2 = big */
    uint8_t  elf_version;   /* ELF version (1) */
    uint8_t  os_abi;        /* target OS ABI */
    uint8_t  padding[8];    /* unused */
    uint16_t type;          /* ET_EXEC etc */
    uint16_t machine;       /* EM_386 etc */
    uint32_t version;       /* ELF version */
    uint32_t entry;         /* entry point virtual address */
    uint32_t ph_offset;     /* program header table offset */
    uint32_t sh_offset;     /* section header table offset */
    uint32_t flags;         /* processor flags */
    uint16_t eh_size;       /* ELF header size */
    uint16_t ph_entry_size; /* program header entry size */
    uint16_t ph_count;      /* number of program headers */
    uint16_t sh_entry_size; /* section header entry size */
    uint16_t sh_count;      /* number of section headers */
    uint16_t sh_str_index;  /* section name string table index */
} __attribute__((packed)) elf_header_t;

/* program header */
typedef struct {
    uint32_t type;          /* PT_LOAD etc */
    uint32_t offset;        /* offset in file */
    uint32_t vaddr;         /* virtual address to load at */
    uint32_t paddr;         /* physical address (ignored) */
    uint32_t file_size;     /* size in file */
    uint32_t mem_size;      /* size in memory (>= file_size) */
    uint32_t flags;         /* segment flags */
    uint32_t align;         /* alignment */
} __attribute__((packed)) elf_program_header_t;

int  elf_load(const char* filename, uint32_t* entry_point);

#endif
