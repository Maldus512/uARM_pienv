/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "sd.h"
#include "uart.h"
#include "utils.h"

// get the end of bss segment from linker
extern unsigned char _end;

static unsigned int partitionlba          = 0;
static unsigned int root_directory_sector = 0;

// the BIOS Parameter Block (in Volume Boot Record)
// FAT32 boot record information
typedef struct {
    char           jmp[3];     // Jump code
    char           oem[8];     // OEM name (Original Equipment Manufacturer)
    unsigned char  bps0;       // Bytes per Logical Sector (should always be 512)
    unsigned char  bps1;
    unsigned char  spc;     // Logical Sectors per Cluster
    unsigned short rsc;     // Number of Reserved Logical Sectors
    unsigned char  nf;      // Number of FAT tables (should always be 2)
    unsigned char  nr0;     // Maximum number of directories entries (NA for fat32)
    unsigned char  nr1;
    unsigned short ts16;      // number of sectors in partitions smaller than 32MB (NA for fat32)
    unsigned char  media;     // media descriptor
    unsigned short spf16;     // Sectors per fat (NA for fat32)
    unsigned short spt;       // Sectors per track
    unsigned short nh;        // Number of heads
    unsigned int   hs;        // Number of hidden sectors in partition
    unsigned int   ts32;      // Number of sectors in partition
    unsigned int   spf32;     // Sectors per FAT
    unsigned int   flg;       // Flags
    unsigned int   rc;        // root directory first cluster
    char           vol[6];
    char           fst[8];
    char           dmy[20];
    char           fst2[8];
} __attribute__((packed)) bpb_t;

// directory entry structure
typedef struct {
    char           name[8];
    char           ext[3];
    char           attr[9];
    unsigned short ch;     // Cluster high
    unsigned int   attr2;
    unsigned short cl;     // Cluster low
    unsigned int   size;
} __attribute__((packed)) fatdir_t;

unsigned char bios_partition_block[512];
unsigned char root_dir[512*4];

//TODO: very big File Allocation Table; check how it grows with bigger partitions
unsigned int FAT32[0x80000];

static inline unsigned int get_lba_address(unsigned int cluster) {
    bpb_t *bpb = (bpb_t *)bios_partition_block;
    return partitionlba + (cluster - 2) * bpb->spc;
}


/**
 * Get the starting LBA address of the first partition
 * so that we know where our FAT file system starts, and
 * read that volume's BIOS Parameter Block
 */
int fat_getpartition(void) {
    // unsigned char *mbr=&_end;
    unsigned char mbr[512];
    bpb_t *       bpb = (bpb_t *)bios_partition_block;
    // read the partitioning table
    if (sd_readblock(0, mbr, 1)) {
        // check magic
        if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
            uart_puts("ERROR: Bad magic in MBR\n");
            return 0;
        }
        // check partition type
        // 0x1C2 is the fourth (4) byte of the first partition entry in the mbr,
        // containing the partition type (fat32, fat16,...)
        if (mbr[0x1C2] != 0xB && mbr[0x1C2] != 0xE /*FAT16 LBA*/ && mbr[0x1C2] != 0xC /*FAT32 LBA*/) {
            uart_puts("ERROR: Wrong partition type\n");
            return 0;
        }
        // 0x1C6 is the eighth byte of the first partition entry in the mbr,
        // containing the number of sectors between the MBR and the first sector in the partition
        // (over a double word, or 4 bytes)
        partitionlba = mbr[0x1C6] + (mbr[0x1C7] << 8) + (mbr[0x1C8] << 16) + (mbr[0x1C9] << 24);
        // read the boot record
        if (!sd_readblock(partitionlba, bpb, 1)) {
            uart_puts("ERROR: Unable to read boot record\n");
            return 0;
        }
        /*if(!sd_readblock(partitionlba,&_end,1)) {
            uart_puts("ERROR: Unable to read boot record\n");
            return 0;
        }*/
        // check file system type. We don't use cluster numbers for that, but magic bytes
        if (!(bpb->fst[0] == 'F' && bpb->fst[1] == 'A' && bpb->fst[2] == 'T') &&
            !(bpb->fst2[0] == 'F' && bpb->fst2[1] == 'A' && bpb->fst2[2] == 'T')) {
            uart_puts("ERROR: Unknown file system type\n");
            return 0;
        }
    }
    // find the root directory's LBA
    root_directory_sector = ((bpb->spf16 ? bpb->spf16 : bpb->spf32) * bpb->nf) + bpb->rsc;
    // add partition LBA
    root_directory_sector += partitionlba;

    if (bpb->spf16 == 0) {
        // adjust for FAT32
        // Cluster begin numbering at 2
        root_directory_sector += (bpb->rc - 2) * bpb->spc;
    }

    sd_readblock(partitionlba + bpb->rsc, (unsigned char *)FAT32, (bpb->spf16 ? bpb->spf16 : bpb->spf32) );

    return 1;
}

/**
 * Find a file in root directory entries
 */
unsigned int fat_getcluster(char *fn) {
    bpb_t *bpb = (bpb_t *)bios_partition_block;
    fatdir_t *   dir = (fatdir_t *)root_dir;
    unsigned int s;
    // find the root directory's LBA
    // This is only for FAT16
    s = (bpb->nr0 + (bpb->nr1 << 8)) * sizeof(fatdir_t);
    // load the root directory
    if (sd_readblock(root_directory_sector, (unsigned char *)dir, s / 512 + 1)) {
        // iterate on each entry and check if it's the one we're looking for
        for (; dir->name[0] != 0; dir++) {
            // is it a valid entry?
            if (dir->name[0] == 0xE5 || dir->attr[0] == 0xF)
                continue;
            // filename match?
            if (!memcmp((unsigned char *)dir->name, (unsigned char *)fn, 11)) {
                uart_puts("FAT File ");
                uart_puts(fn);
                uart_puts(" starts at cluster: ");
                uart_hex(((unsigned int)dir->ch) << 16 | dir->cl);
                uart_puts("\n");
                // if so, return starting cluster
                return ((unsigned int)dir->ch) << 16 | dir->cl;
            }
        }
        uart_puts("ERROR: file not found\n");
    } else {
        uart_puts("ERROR: Unable to load root directory\n");
    }
    return 0;
}

unsigned char readfile[512];

/**
 * Read a file into memory
 */
int fat_readfile(unsigned int cluster, unsigned char *data) {
    // BIOS Parameter Block
    bpb_t *bpb = (bpb_t *)bios_partition_block;
    // File allocation tables. We choose between FAT16 and FAT32 dynamically
    unsigned int *  fat32 = (unsigned int *)FAT32;//(&_end);
    unsigned short *fat16 = (unsigned short *)fat32;
    // Data pointers
    unsigned int   data_sec, s;
    int            read = 0;
    // find the LBA of the first data sector
    data_sec = ((bpb->spf16 ? bpb->spf16 : bpb->spf32) * bpb->nf) + bpb->rsc;
    s        = (bpb->nr0 + (bpb->nr1 << 8)) * sizeof(fatdir_t);
    if (bpb->spf16 > 0) {
        // adjust for FAT16
        data_sec += (s + 511) >> 9;
    }
    // add partition LBA
    data_sec += partitionlba;
    // dump important properties
    uart_puts("FAT Bytes per Sector: ");
    uart_hex(bpb->bps0 + (bpb->bps1 << 8));
    uart_puts("\nFAT Sectors per Cluster: ");
    uart_hex(bpb->spc);
    uart_puts("\nFAT Number of FAT: ");
    uart_hex(bpb->nf);
    uart_puts("\nFAT Sectors per FAT: ");
    uart_hex((bpb->spf16 ? bpb->spf16 : bpb->spf32));
    uart_puts("\nFAT Reserved Sectors Count: ");
    uart_hex(bpb->rsc);
    uart_puts("\nFAT First data sector: ");
    uart_hex(data_sec);
    uart_puts("\n");
    // load FAT table
    // TODO: does this do anything? Yes, it loads the FAT. I don't see a difference because I'm always reading 1 cluster files
    //s = sd_readblock(partitionlba + bpb->rsc, (unsigned char *)fat32, (bpb->spf16 ? bpb->spf16 : bpb->spf32) );
    // end of FAT in memory
    // iterate on cluster chain
    while (cluster > 1 && cluster < 0xFFF8) {
        // load all sectors in a cluster
        read += sd_readblock((cluster - 2) * bpb->spc + data_sec, data, bpb->spc);
        // move pointer, sector per cluster * bytes per sector
        //ptr += bpb->spc * (bpb->bps0 + (bpb->bps1 << 8));
        data += read;
        // get the next cluster in chain
        cluster = bpb->spf16 > 0 ? fat16[cluster] : fat32[cluster];
    }
    return read;
}

/**
 * List root directory entries in a FAT file system
 */
void fat_listdirectory(void) {
    bpb_t *      bpb = (bpb_t *)&_end;
    fatdir_t *   dir = (fatdir_t *)&_end;
    unsigned int s;
    // find the root directory's LBA
    s = (bpb->nr0 + (bpb->nr1 << 8));
    uart_puts("FAT number of root diretory entries: ");
    uart_hex(s);
    s *= sizeof(fatdir_t);
    // add partition LBA
    uart_puts("\nFAT root directory LBA: ");
    uart_hex(root_directory_sector);
    uart_puts("\n");
    // load the root directory
    if (sd_readblock(root_directory_sector, (unsigned char *)&_end, s / 512 + 1)) {
        uart_puts("\nAttrib Cluster  Size     Name\n");
        // iterate on each entry and print out
        for (; dir->name[0] != 0; dir++) {
            // is it a valid entry?
            if (dir->name[0] == 0xE5 || dir->attr[0] == 0xF)
                continue;
            // decode attributes
            uart_send(dir->attr[0] & 1 ? 'R' : '.');      // read-only
            uart_send(dir->attr[0] & 2 ? 'H' : '.');      // hidden
            uart_send(dir->attr[0] & 4 ? 'S' : '.');      // system
            uart_send(dir->attr[0] & 8 ? 'L' : '.');      // volume label
            uart_send(dir->attr[0] & 16 ? 'D' : '.');     // directory
            uart_send(dir->attr[0] & 32 ? 'A' : '.');     // archive
            uart_send(' ');
            // staring cluster
            uart_hex(((unsigned int)dir->ch) << 16 | dir->cl);
            uart_send(' ');
            // size
            uart_hex(dir->size);
            uart_send(' ');
            // filename
            dir->attr[0] = 0;
            uart_puts(dir->name);
            uart_puts("\n");
        }
    } else {
        uart_puts("ERROR: Unable to load root directory\n");
    }
}
