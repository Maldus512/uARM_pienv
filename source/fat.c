#include "sd.h"
#include "uart.h"
#include "utils.h"

// get the end of bss segment from linker

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
unsigned char root_dir[512 * 4];

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
    bpb_t *       bpb = (bpb_t *)bios_partition_block;
    // read the partitioning table
    if (sd_transferblock(0, mbr, 1, SD_READBLOCK)) {
        // check magic
        if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
            uart_puts("ERROR: Bad magic in MBR\n");
            return 0;
        }
        // check partition type
        // 0x1C2 is the fourth (4) byte of the first partition entry in the mbr,
        // containing the partition type (fat32, fat16,...)
        if (mbr[0x1C2] != 0xB && mbr[0x1C2] != 0xC /*FAT32 LBA*/) {
            uart_puts("ERROR: Wrong partition type\n");
            return 0;
        }
        // 0x1C6 is the eighth byte of the first partition entry in the mbr,
        // containing the number of sectors between the MBR and the first sector in the partition
        // (over a double word, or 4 bytes)
        partitionlba = mbr[0x1C6] + (mbr[0x1C7] << 8) + (mbr[0x1C8] << 16) + (mbr[0x1C9] << 24);
        // read the boot record
        if (!sd_transferblock(partitionlba, (unsigned char *)bpb, 1, SD_READBLOCK)) {
            uart_puts("ERROR: Unable to read boot record\n");
            return 0;
        }
        // check file system type. We don't use cluster numbers for that, but magic bytes
        if (!(bpb->fst[0] == 'F' && bpb->fst[1] == 'A' && bpb->fst[2] == 'T') &&
            !(bpb->fst2[0] == 'F' && bpb->fst2[1] == 'A' && bpb->fst2[2] == 'T')) {
            uart_puts("ERROR: Unknown file system type\n");
            return 0;
        }
    }
    // find the root directory's LBA
    root_directory_sector = (bpb->spf32 * bpb->nf) + bpb->rsc;
    // add partition LBA
    root_directory_sector += partitionlba;

    return 1;
}

unsigned int fat_get_table_entry(unsigned int index) {
    unsigned int buffer[512 / 4];
    unsigned int entry_sector_offset = (index * 4) / 512;
    bpb_t *      bpb                 = (bpb_t *)bios_partition_block;
    sd_transferblock(partitionlba + bpb->rsc + entry_sector_offset, (unsigned char *)buffer, 1, SD_READBLOCK);
    return buffer[index % (512 / 4)];
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
    if (sd_transferblock(root_directory_sector, (unsigned char *)dir, s / 512 + 1, SD_READBLOCK)) {
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

        strcpy(string, "ERROR: file not found: ");
        strcpy(&string[strlen(string)], fn);
        LOG(WARN, string);
    } else {
        uart_puts("ERROR: Unable to load root directory\n");
    }
    return 0;
}

/**
 * Read a file into memory
 */
int fat_transferfile(unsigned int cluster, unsigned char *data, unsigned int num, readwrite_t readwrite) {
    // BIOS Parameter Block
    bpb_t *bpb = (bpb_t *)bios_partition_block;
    // Data pointers
    unsigned int data_sec, counter;
    int          read = 0, tmp;
    // find the LBA of the first data sector
    data_sec = (bpb->spf32 * bpb->nf) + bpb->rsc;
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
    // end of FAT in memory
    // iterate on cluster chain
    counter = 0;
    while (cluster > 1 && cluster < 0xFFF8) {
        if (counter == num) {
            // load all sectors in a cluster
            tmp = sd_transferblock((cluster - 2) * bpb->spc + data_sec, data, bpb->spc, readwrite);
            if (tmp == 0) {
                uart0_puts("empty transfer block!\n");
                return -1;
                // continue;
            }
            read += tmp;
        } else {
            // get the next cluster in chain
            // cluster = bpb->spf16 > 0 ? fat16[cluster] : fat32[cluster];
            cluster = fat_get_table_entry(cluster);
        }
        counter++;
    }
    return read;
}