/*
 * Hardware Abstraction Layer for Raspberry Pi 3
 *
 * Copyright (C) 2018 Mattia Maldini
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/******************************************************************************
 * This module contains a small FAT32 filesystem library
 ******************************************************************************/

#include "sd.h"
#include "uart.h"
#include "utils.h"

// TODO: increase this if necessary
#define CACHEDFATSIZE (512 / 4)

static unsigned int partitionlba          = 0;
static unsigned int root_directory_sector = 0;
static unsigned int bytes_per_sector;

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
unsigned char root_dir[512 * 4];
unsigned int  cachedFAT[CACHEDFATSIZE];
unsigned int  last_entry_sector_offset = 0;

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
    unsigned char mbr[512];
    char          string[128];
    bpb_t *       bpb = (bpb_t *)bios_partition_block;
    // read the partitioning table
    if (sd_transferblock(0, mbr, 1, SD_READBLOCK)) {
        // check magic
        if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
            LOG(ERROR, "Bad magic in MBR");
            return 0;
        }
        // check partition type
        // 0x1C2 is the fourth (4) byte of the first partition entry in the mbr,
        // containing the partition type (fat32, fat16,...)
        if (mbr[0x1C2] != 0xB && mbr[0x1C2] != 0xC /*FAT32 LBA*/) {
            LOG(ERROR, "Wrong partition type");
            return 0;
        }
        // 0x1C6 is the eighth byte of the first partition entry in the mbr,
        // containing the number of sectors between the MBR and the first sector in the partition
        // (over a double word, or 4 bytes)
        partitionlba = mbr[0x1C6] + (mbr[0x1C7] << 8) + (mbr[0x1C8] << 16) + (mbr[0x1C9] << 24);
        // read the boot record
        if (!sd_transferblock(partitionlba, (unsigned char *)bpb, 1, SD_READBLOCK)) {
            LOG(ERROR, "Unable to read boot record");
            return 0;
        }
        // check file system type. We don't use cluster numbers for that, but magic bytes
        if (!(bpb->fst[0] == 'F' && bpb->fst[1] == 'A' && bpb->fst[2] == 'T') &&
            !(bpb->fst2[0] == 'F' && bpb->fst2[1] == 'A' && bpb->fst2[2] == 'T')) {
            LOG(ERROR, "Unknown file system type");
            return 0;
        }
    }
    // find the root directory's LBA
    root_directory_sector = (bpb->spf32 * bpb->nf) + bpb->rsc;
    // add partition LBA
    root_directory_sector += partitionlba;

    bytes_per_sector = bpb->bps0 + (bpb->bps1 << 8);

    if (bytes_per_sector != 512) {
        // TODO: support other sizes
        LOG(ERROR, "Wrong bytes per sector number");
        return 0;
    }

    // dump important properties
    strcpy(string, "FAT Bytes per Sector: ");
    itoa(bpb->bps0 + (bpb->bps1 << 8), &string[strlen(string)], 10);
    LOG(INFO, string);
    strcpy(string, "FAT Sectors per Cluster: ");
    itoa(bpb->spc, &string[strlen(string)], 10);
    LOG(INFO, string);
    strcpy(string, "FAT Number of FAT: ");
    itoa(bpb->nf, &string[strlen(string)], 10);
    LOG(INFO, string);
    strcpy(string, "FAT Sectors per FAT: ");
    itoa(bpb->spf32, &string[strlen(string)], 10);
    LOG(INFO, string);
    strcpy(string, "FAT Reserved Sectors Count: ");
    itoa(bpb->rsc, &string[strlen(string)], 10);
    LOG(INFO, string);
    strcpy(string, "FAT First data sector: ");
    itoa(root_directory_sector, &string[strlen(string)], 10);
    LOG(INFO, string);

    sd_transferblock(partitionlba + bpb->rsc, (unsigned char *)cachedFAT, (CACHEDFATSIZE * 4) / bytes_per_sector,
                     SD_READBLOCK);
    return 1;
}

/*
 * This function returns the next cluster pointed by index in the file allocation
 * table. I can't read the whole FAT in ram because it can get arbitrarily big,
 * but if I read it every time bigger files take forever to access. I partially
 * cache the table as a result
 */
unsigned int fat_get_table_entry(unsigned int index) {
    bpb_t *       bpb                 = (bpb_t *)bios_partition_block;
    unsigned long entry_sector_offset = (index * 4) / bytes_per_sector;
    unsigned long cached_blocks       = (CACHEDFATSIZE * 4) / bytes_per_sector;

    // Keep a cached FAT table to avoid reading the SD multiple times for every file
    if (entry_sector_offset < last_entry_sector_offset ||
        entry_sector_offset >= last_entry_sector_offset + cached_blocks) {
        sd_transferblock(partitionlba + bpb->rsc + entry_sector_offset, (unsigned char *)cachedFAT,
                         (CACHEDFATSIZE * 4) / bytes_per_sector, SD_READBLOCK);
    }
    last_entry_sector_offset = entry_sector_offset;
    return cachedFAT[index % CACHEDFATSIZE];
}

/**
 * Find a file in root directory entries
 */
unsigned int fat_getcluster(char *fn) {
    bpb_t *      bpb = (bpb_t *)bios_partition_block;
    fatdir_t *   dir = (fatdir_t *)root_dir;
    char         string[128];
    unsigned int s;
    // find the root directory's LBA
    // This is only for FAT16
    s = (bpb->nr0 + (bpb->nr1 << 8)) * sizeof(fatdir_t);
    // load the root directory
    if (sd_transferblock(root_directory_sector, (unsigned char *)dir, s / bytes_per_sector + 1, SD_READBLOCK)) {
        // iterate on each entry and check if it's the one we're looking for
        for (; dir->name[0] != 0; dir++) {
            // is it a valid entry?
            if (dir->name[0] == 0xE5 || dir->attr[0] == 0xF)
                continue;
            // filename match?
            if (!memcmp((unsigned char *)dir->name, (unsigned char *)fn, 11)) {
                strcpy(string, "FAT File ");
                strcpy(&string[strlen(string)], fn);
                strcpy(&string[strlen(string)], " starts at cluster: ");
                itoa(((unsigned int)dir->ch) << 16 | dir->cl, &string[strlen(string)], 16);
                LOG(INFO, string);
                // if so, return starting cluster
                return ((unsigned int)dir->ch) << 16 | dir->cl;
            }
        }

        strcpy(string, "File not found: ");
        strcpy(&string[strlen(string)], fn);
        LOG(WARN, string);
    } else {
        LOG(ERROR, "Unable to load root directory");
    }
    return 0;
}

/**
 * List root directory entries in a FAT file system
 */
void fat_listdirectory(void) {
    bpb_t *      bpb = (bpb_t *)bios_partition_block;
    fatdir_t *   dir = (fatdir_t *)root_dir;
    int          i   = 0;
    unsigned int s;
    char         string[128];
    s = (bpb->nr0 + (bpb->nr1 << 8));
    strcpy(string, "FAT number of root diretory entries: ");
    itoa(s, &string[strlen(string)], 10);
    LOG(INFO, string);

    s *= sizeof(fatdir_t);
    // load the root directory
    if (sd_transferblock(root_directory_sector, (unsigned char *)dir, s / bytes_per_sector + 1, SD_READBLOCK)) {
        LOG(INFO, "Attrib\tCluster\tSize\t\tName");
        // iterate on each entry and print out
        for (; dir->name[0] != 0; dir++) {
            // is it a valid entry?
            if (dir->name[0] == 0xE5 || dir->attr[0] == 0xF)
                continue;

            i = 0;
            // decode attributes
            string[i++] = dir->attr[0] & 1 ? 'R' : '.';      // read-only
            string[i++] = dir->attr[0] & 2 ? 'H' : '.';      // hidden
            string[i++] = dir->attr[0] & 4 ? 'S' : '.';      // system
            string[i++] = dir->attr[0] & 8 ? 'L' : '.';      // volume label
            string[i++] = dir->attr[0] & 16 ? 'D' : '.';     // directory
            string[i++] = dir->attr[0] & 32 ? 'A' : '.';     // archive
            string[i++] = '\t';
            // staring cluster
            itoa(((unsigned int)dir->ch) << 16 | dir->cl, &string[i], 16);
            i           = strlen(string);
            string[i++] = '\t';
            // size
            itoa(dir->size, &string[i], 16);
            i           = strlen(string);
            string[i++] = '\t';
            string[i++] = '\t';
            // filename
            dir->attr[0] = 0;
            strcpy(&string[i], dir->name);
            LOG(INFO, string);
        }
    } else {
        LOG(ERROR, "Unable to load root directory");
    }
}

/**
 * Read or write a file into memory
 */
int fat_transferfile(unsigned int cluster, unsigned char *data, unsigned int num, readwrite_t readwrite) {
    // BIOS Parameter Block
    bpb_t *bpb = (bpb_t *)bios_partition_block;
    // unsigned int *  fat32 = (unsigned int *)FAT32;
    // Data pointers
    unsigned int data_sec, counter;
    int          read = 0, tmp;
    // find the LBA of the first data sector
    data_sec = (bpb->spf32 * bpb->nf) + bpb->rsc;
    // add partition LBA
    data_sec += partitionlba;
    // end of FAT in memory
    // iterate on cluster chain
    counter = 0;
    while (cluster > 1 && cluster < 0xFFF8) {
        if (counter == num) {
            // load all sectors in a cluster
            tmp = sd_transferblock((cluster - 2) * bpb->spc + data_sec, data, bpb->spc, readwrite);
            if (tmp <= 0) {
                LOG(WARN, "Empty transfer block!");
                return -1;
                // continue;
            }
            read += tmp;
            break;
        } else {
            // get the next cluster in chain
            cluster     = fat_get_table_entry(cluster);
        }
        counter++;
    }
    if (read <= 0) {
        LOG(WARN, "Could not find cluster");
    }
    return read;
}

/*
 * Read a specified amount of bytes from a given offset in a file. Handles
 * reading in clusters and returns only the requested data.
 */
int fat_readfile(unsigned int cluster, unsigned char *data, unsigned int seek, unsigned int length) {
    bpb_t *       bpb = (bpb_t *)bios_partition_block;
    unsigned char buffer[4096];
    unsigned int  cluster_num, start, index, effectiveLen;
    int           size;

    cluster_num = seek / (bytes_per_sector * bpb->spc);
    start       = seek % (bytes_per_sector * bpb->spc);
    index       = 0;

    while (length > 0) {
        size = fat_transferfile(cluster, buffer, cluster_num, SD_READBLOCK);
        if (size <= 0) {
            LOG(WARN, "Empty read");
            return index;
        }
        effectiveLen = size-start < length ? size-start : length;
        memcpy(&data[index], &buffer[start], effectiveLen);
        start = 0;
        index += effectiveLen;
        length -= effectiveLen;
        cluster_num++;
    }
    return index;
}

/*
 * Writes a specified amount of bytes at a given offset in a file. Handles
 * reading the file first and modifying only the needed data.
 */
int fat_writefile(unsigned int cluster, unsigned char *data, unsigned int seek, unsigned int length) {
    bpb_t *       bpb = (bpb_t *)bios_partition_block;
    unsigned char buffer[4096];
    unsigned int  cluster_num, start, index, effectiveLen;
    int           size;

    cluster_num = seek / (bytes_per_sector * bpb->spc);
    start       = seek % (bytes_per_sector * bpb->spc);
    index       = 0;

    while (length > 0) {
        // Read the original content
        size = fat_transferfile(cluster, buffer, cluster_num, SD_READBLOCK);
        if (size <= 0) {
            LOG(WARN, "Empty read");
            return index;
        }
        effectiveLen = size < length ? size : length;
        // Modify and then write again the changed content
        memcpy(&buffer[start], &data[index], effectiveLen);
        fat_transferfile(cluster, buffer, cluster_num, SD_WRITEBLOCK);
        start = 0;
        index += effectiveLen;
        length -= effectiveLen;
        cluster_num++;
    }
    return index;
}