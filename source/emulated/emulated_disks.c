#include "uart.h"
#include "utils.h"
#include "emulated_disks.h"
#include "sd.h"
#include "fat.h"
#include "interrupts.h"
#include "emulated_timers.h"

disk_internal_state_t emulated_disks[MAX_DISKS];

void init_emulated_disks() {
    int           i;
    char          nome[] = "DISKn      ";
    char          string[128];
    unsigned int  buffer[8];
    unsigned int  diskfileid;
    unsigned int *diskinfo;
    uint8_t *     device_installed = (uint8_t *)DEVICE_INSTALLED;
    uint8_t       tmp;

    device_installed[IL_DISK] = 0x00;
    for (i = 0; i < MAX_DISKS; i++) {
        nome[4]                         = '0' + i;
        emulated_disks[i].fat32_cluster = fat_getcluster(nome);

        if (emulated_disks[i].fat32_cluster) {
            fat_readfile(emulated_disks[i].fat32_cluster, (unsigned char *)buffer, 0, 8 * 4);
            diskfileid = buffer[0];
            diskinfo   = &buffer[1];
            if (diskfileid != DISKFILEID || diskinfo[0] > MAX_CYLINDERS || diskinfo[1] > MAX_HEADS ||
                diskinfo[2] > MAX_SECTORS) {
                emulated_disks[i].internal_registers.status = DEVICE_NOT_INSTALLED;
                emulated_disks[i].internal_registers.data1  = 0;
                strcpy(string, "Invalid disk file ");
                strcpy(&string[strlen(string)], nome);
                LOG(WARN, string);
            } else {
                strcpy(string, "Found disk device ");
                strcpy(&string[strlen(string)], nome);
                strcpy(&string[strlen(string)], ": ");
                itoa(diskinfo[0], &string[strlen(string)], 10);
                strcpy(&string[strlen(string)], " cylinders, ");
                itoa(diskinfo[1], &string[strlen(string)], 10);
                strcpy(&string[strlen(string)], " heads, ");
                itoa(diskinfo[2], &string[strlen(string)], 10);
                strcpy(&string[strlen(string)], " sectors, ");
                itoa(diskinfo[3], &string[strlen(string)], 10);
                strcpy(&string[strlen(string)], " RPM, ");
                itoa(diskinfo[4], &string[strlen(string)], 10);
                strcpy(&string[strlen(string)], " seek time, ");
                itoa(diskinfo[5], &string[strlen(string)], 10);
                strcpy(&string[strlen(string)], "% occupation");
                LOG(INFO, string);
                tmp                                         = device_installed[IL_DISK] | ((uint8_t)(1 << i));
                device_installed[IL_DISK]                   = tmp;
                emulated_disks[i].internal_registers.status = DEVICE_READY;
                emulated_disks[i].cylinders                 = diskinfo[0];
                emulated_disks[i].heads                     = diskinfo[1];
                emulated_disks[i].sectors                   = diskinfo[2];
                emulated_disks[i].rpm                       = diskinfo[3];
                emulated_disks[i].seektime                  = diskinfo[4];
                emulated_disks[i].dataoccupation            = diskinfo[5];
                emulated_disks[i].internal_registers.data1  = emulated_disks[i].sectors;
                emulated_disks[i].internal_registers.data1 |= emulated_disks[i].heads << 8;
                emulated_disks[i].internal_registers.data1 |= emulated_disks[i].cylinders << 16;
                emulated_disks[i].current_cylinder = emulated_disks[i].current_head = emulated_disks[i].current_sector =
                    0;
            }
        } else {
            emulated_disks[i].internal_registers.status = DEVICE_NOT_INSTALLED;
            emulated_disks[i].internal_registers.data1  = 0;
        }
        emulated_disks[i].internal_registers.command = RESET;
    }
}

unsigned long time_to_destination(int disk, int cyl, int head, int sect) {
    unsigned int currentcyl, currentsect, rotationspeed;
    unsigned long sectortime, cylindertime;
    // Sectors in 1 second
    rotationspeed = (emulated_disks[disk].rpm / 60) * emulated_disks[disk].sectors;
    // us to move 1 sector
    rotationspeed = 1000000UL/rotationspeed;

    currentcyl   = emulated_disks[disk].current_cylinder;
    currentsect  = emulated_disks[disk].current_sector;
    cylindertime = currentcyl > cyl ? currentcyl - cyl : cyl - currentcyl;
    // us to reach the destination cylinder
    cylindertime *= emulated_disks[disk].seektime;

    sectortime = currentsect > sect ? currentsect - sect : sect - currentsect;
    sectortime *= rotationspeed;

    return sectortime+cylindertime;
}

unsigned int seek_disk_block(int disk) {
    return 7 * 4 + ((emulated_disks[disk].current_head * emulated_disks[disk].cylinders) +
                    (emulated_disks[disk].current_cylinder * emulated_disks[disk].sectors) +
                    emulated_disks[disk].current_sector) *
                       DISK_BLOCKSIZE;
}

int read_disk_block(int disk, unsigned char *buffer) {
    unsigned int disk_start;
    if (emulated_disks[disk].fat32_cluster == 0)
        return 0;

    disk_start = seek_disk_block(disk);

    return fat_readfile(emulated_disks[disk].fat32_cluster, buffer, disk_start, DISK_BLOCKSIZE);
}

unsigned int write_disk_block(int disk, unsigned char *buffer) {
    unsigned int disk_start;
    if (emulated_disks[disk].fat32_cluster == 0)
        return 0;

    disk_start = seek_disk_block(disk);

    return fat_writefile(emulated_disks[disk].fat32_cluster, buffer, disk_start, DISK_BLOCKSIZE);
}


void manage_emulated_disk(int i) {
    uint8_t *interrupt_lines = (uint8_t *)INTERRUPT_LINES;
    uint8_t  intline;

    if (emulated_disks[i].internal_registers.status == DEVICE_NOT_INSTALLED)
        return;

    if (emulated_disks[i].internal_registers.status == DEVICE_BUSY) {
        switch (GET_COMMAND(emulated_disks[i].internal_registers.command)) {
            case SEEKCYL:
                emulated_disks[i].current_cylinder = GET_COMMAND_CYLNUM(emulated_disks[i].internal_registers.command);
                emulated_disks[i].internal_registers.status = DEVICE_READY;
                intline                                     = interrupt_lines[IL_DISK] | ((uint8_t)(1 << i));
                interrupt_lines[IL_DISK]                    = intline;
                memcpy(emulated_disks[i].mailbox_registers, &emulated_disks[i].internal_registers, sizeof(diskreg_t));
                emulated_disks[i].mailbox_registers = NULL;
                break;
            case READBLK:
                emulated_disks[i].current_head   = GET_COMMAND_HEADNUM(emulated_disks[i].internal_registers.command);
                emulated_disks[i].current_sector = GET_COMMAND_SECTNUM(emulated_disks[i].internal_registers.command);

                if (read_disk_block(i, (unsigned char *)(uint64_t)emulated_disks[i].internal_registers.data0) < 0) {
                    emulated_disks[i].internal_registers.status = READ_ERROR;
                } else {
                    emulated_disks[i].internal_registers.status = DEVICE_READY;
                    intline                                     = interrupt_lines[IL_DISK] | ((uint8_t)(1 << i));
                    interrupt_lines[IL_DISK]                    = intline;
                }
                memcpy(emulated_disks[i].mailbox_registers, &emulated_disks[i].internal_registers, sizeof(diskreg_t));
                emulated_disks[i].mailbox_registers = NULL;
                break;

            case WRITEBLK:
                emulated_disks[i].current_head   = GET_COMMAND_HEADNUM(emulated_disks[i].internal_registers.command);
                emulated_disks[i].current_sector = GET_COMMAND_SECTNUM(emulated_disks[i].internal_registers.command);
                if (write_disk_block(i, (unsigned char *)(uint64_t)emulated_disks[i].internal_registers.data0) < 0) {
                    emulated_disks[i].internal_registers.status = WRITE_ERROR;
                } else {
                    emulated_disks[i].internal_registers.status = DEVICE_READY;
                    intline                                     = interrupt_lines[IL_DISK] | ((uint8_t)(1 << i));
                    interrupt_lines[IL_DISK]                    = intline;
                }
                memcpy(emulated_disks[i].mailbox_registers, &emulated_disks[i].internal_registers, sizeof(diskreg_t));
                emulated_disks[i].mailbox_registers = NULL;

                break;

            default:
                break;
        }
    }
}



void emulated_disk_mailbox(int i, diskreg_t *registers) {
    uint8_t *interrupt_lines = (uint8_t *)INTERRUPT_LINES;
    uint8_t  intline, head, sect;
    uint16_t cyl;
    unsigned long timetodest;

    if (registers == NULL)
        return;

    emulated_disks[i].internal_registers.mailbox = 1;

    if (emulated_disks[i].internal_registers.status == DEVICE_NOT_INSTALLED && registers->command != READ_REGISTERS) {
        memcpy(registers, &emulated_disks[i].internal_registers, sizeof(diskreg_t));
        return;
    }

    switch (GET_COMMAND(registers->command)) {
        case RESET:
            emulated_disks[i].internal_registers.command = RESET;
            emulated_disks[i].internal_registers.status  = DEVICE_READY;
            emulated_disks[i].current_cylinder           = 0;
            emulated_disks[i].current_head               = 0;
            emulated_disks[i].current_sector             = 0;
            intline                                      = interrupt_lines[IL_DISK] | ((uint8_t)(1 << i));
            interrupt_lines[IL_DISK]                     = intline;
            memcpy(registers, &emulated_disks[i].internal_registers, sizeof(diskreg_t));
            break;

        case ACK:
            if (emulated_disks[i].internal_registers.status != DEVICE_BUSY) {
                emulated_disks[i].internal_registers.command = ACK;
                emulated_disks[i].internal_registers.status  = DEVICE_READY;
                intline                                      = interrupt_lines[IL_DISK] & ((uint8_t)(~(1 << i)));
                interrupt_lines[IL_DISK]                     = intline;
                memcpy(registers, &emulated_disks[i].internal_registers, sizeof(diskreg_t));
            }
            break;

        case READ_REGISTERS:
            memcpy(registers, &emulated_disks[i].internal_registers, sizeof(diskreg_t));
            break;

        case READBLK:
        case WRITEBLK:
            head = GET_COMMAND_HEADNUM(registers->command);
            sect = GET_COMMAND_SECTNUM(registers->command);
            if (emulated_disks[i].internal_registers.status != DEVICE_BUSY) {
                if (head > emulated_disks[i].heads || sect > emulated_disks[i].sectors) {
                    emulated_disks[i].internal_registers.status = ILLEGAL_OPERATION;
                } else {
                    emulated_disks[i].internal_registers.command = registers->command;
                    emulated_disks[i].internal_registers.data0   = registers->data0;
                    emulated_disks[i].internal_registers.status  = DEVICE_BUSY;
                    emulated_disks[i].mailbox_registers          = registers;
                    timetodest = time_to_destination(i, emulated_disks[i].current_cylinder, head, sect);
                    set_device_timer(timetodest, DISK, i);
                }
            }
            memcpy(registers, &emulated_disks[i].internal_registers, sizeof(diskreg_t));
            break;

        case SEEKCYL:
            cyl = GET_COMMAND_CYLNUM(registers->command);
            if (emulated_disks[i].internal_registers.status != DEVICE_BUSY) {
                if (cyl > emulated_disks[i].cylinders) {
                    emulated_disks[i].internal_registers.status = ILLEGAL_OPERATION;
                } else {
                    emulated_disks[i].internal_registers.command = registers->command;
                    emulated_disks[i].internal_registers.data0   = registers->data0;
                    emulated_disks[i].internal_registers.status  = DEVICE_BUSY;
                    emulated_disks[i].mailbox_registers          = registers;
                    timetodest = time_to_destination(i, cyl, emulated_disks[i].current_head, emulated_disks[i].current_sector);
                    set_device_timer(timetodest, DISK, i);
                }
            }
            memcpy(registers, &emulated_disks[i].internal_registers, sizeof(diskreg_t));
            break;

        default:
            emulated_disks[i].internal_registers.status = ILLEGAL_OPERATION;
            memcpy(registers, &emulated_disks[i].internal_registers, sizeof(diskreg_t));
            break;
    }
}