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
 * This module is the EMMC control library
 ******************************************************************************/

#include "gpio.h"
#include "uart.h"
#include "sd.h"
#include "timers.h"
#include "utils.h"

// command flags
#define CMD_NEED_APP 0x80000000
#define CMD_RSPNS_48 0x00020000
#define CMD_ERRORS_MASK 0xfff9c004
#define CMD_RCA_MASK 0xffff0000

// COMMANDs
#define CMD_GO_IDLE 0x00000000
#define CMD_ALL_SEND_CID 0x02010000
#define CMD_SEND_REL_ADDR 0x03020000
#define CMD_CARD_SELECT 0x07030000
#define CMD_SEND_IF_COND 0x08020000
#define CMD_STOP_TRANS 0x0C030000
#define CMD_READ_SINGLE 0x11220010
#define CMD_READ_MULTI 0x12220032

#define CMD_WRITE_SINGLE 0x18220000
#define CMD_WRITE_MULTI 0x19220022

#define CMD_SET_BLOCKCNT 0x17020000
#define CMD_APP_CMD 0x37000000
#define CMD_SET_BUS_WIDTH (0x06020000 | CMD_NEED_APP)
#define CMD_SEND_OP_COND (0x29020000 | CMD_NEED_APP)
#define CMD_SEND_SCR (0x33220010 | CMD_NEED_APP)

// STATUS register settings
#define SR_READ_AVAILABLE 0x00000800
#define SR_DAT_INHIBIT 0x00000002
#define SR_CMD_INHIBIT 0x00000001
#define SR_APP_CMD 0x00000020

// INTERRUPT register settings
#define INT_DATA_TIMEOUT 0x00100000
#define INT_CMD_TIMEOUT 0x00010000
#define INT_READ_RDY 0x00000020
#define INT_WRITE_RDY 0x00000010
#define INT_CMD_DONE 0x00000001

#define INT_ERROR_MASK 0x017E8000

// CONTROL register settings
#define C0_SPI_MODE_EN 0x00100000
#define C0_HCTL_HS_EN 0x00000004
#define C0_HCTL_DWITDH 0x00000002

#define C1_SRST_DATA 0x04000000
#define C1_SRST_CMD 0x02000000
#define C1_SRST_HC 0x01000000
#define C1_TOUNIT_DIS 0x000f0000
#define C1_TOUNIT_MAX 0x000e0000
#define C1_CLK_GENSEL 0x00000020
#define C1_CLK_EN 0x00000004
#define C1_CLK_STABLE 0x00000002
#define C1_CLK_INTLEN 0x00000001

// SLOTISR_VER values
#define HOST_SPEC_NUM 0x00ff0000
#define HOST_SPEC_NUM_SHIFT 16
#define HOST_SPEC_V3 2
#define HOST_SPEC_V2 1
#define HOST_SPEC_V1 0

// SCR flags
#define SCR_SD_BUS_WIDTH_4 0x00000400
#define SCR_SUPP_SET_BLKCNT 0x02000000
// added by my driver
#define SCR_SUPP_CCS 0x00000001

#define ACMD41_VOLTAGE 0x00ff8000
#define ACMD41_CMD_COMPLETE 0x80000000
#define ACMD41_CMD_CCS 0x40000000
#define ACMD41_ARG_HC 0x51ff8000

unsigned long sd_scr[2], sd_ocr, sd_rca, sd_err, sd_hv;

static inline void wait_cycles(unsigned int n) {
    while (n-- > 0)
        nop();
}

/**
 * Wait for data or command ready
 */
int sd_status(unsigned int mask) {
    int cnt = 500000;
    while ((EMMC->STATUS & mask) && !(EMMC->INTERRUPT & INT_ERROR_MASK) && cnt--)
        wait_msec(1);
    return (cnt <= 0 || (EMMC->INTERRUPT & INT_ERROR_MASK)) ? SD_ERROR : SD_OK;
}

/**
 * Wait for interrupt
 */
int sd_int(unsigned int mask) {
    unsigned int r, m = mask | INT_ERROR_MASK;
    int          cnt = 2000;
    while (!(EMMC->INTERRUPT & m) && cnt--)
        wait_msec(1);
    r = EMMC->INTERRUPT;
    if (cnt <= 0 || (r & INT_CMD_TIMEOUT) || (r & INT_DATA_TIMEOUT)) {
        LOG(ERROR, "sd command timeout");
        EMMC->INTERRUPT = r;
        return SD_TIMEOUT;
    } else if (r & INT_ERROR_MASK) {
        EMMC->INTERRUPT = r;
        LOG(ERROR, "sd command error");
        return SD_ERROR;
    }
    EMMC->INTERRUPT = mask;
    return 0;
}

/**
 * Send a command
 */
int sd_cmd(unsigned int code, unsigned int arg) {
    int r  = 0;
    sd_err = SD_OK;
    if (code & CMD_NEED_APP) {
        r = sd_cmd(CMD_APP_CMD | (sd_rca ? CMD_RSPNS_48 : 0), sd_rca);
        if (sd_rca && !r) {
            LOG(ERROR, "Failed to send SD APP command");
            sd_err = SD_ERROR;
            return 0;
        }
        code &= ~CMD_NEED_APP;
    }
    if (sd_status(SR_CMD_INHIBIT)) {
        LOG(ERROR, "EMMC busy");
        sd_err = SD_TIMEOUT;
        return 0;
    }
    EMMC->INTERRUPT = EMMC->INTERRUPT;
    EMMC->ARG1      = arg;
    EMMC->CMDTM     = code;
    if (code == CMD_SEND_OP_COND)
        wait_msec(1000);
    else if (code == CMD_SEND_IF_COND || code == CMD_APP_CMD)
        wait_msec(100);
    if ((r = sd_int(INT_CMD_DONE))) {
        LOG(ERROR, "Failed to send EMMC command");
        sd_err = r;
        return 0;
    }
    r = EMMC->RESP0;
    if (code == CMD_GO_IDLE || code == CMD_APP_CMD)
        return 0;
    else if (code == (CMD_APP_CMD | CMD_RSPNS_48))
        return r & SR_APP_CMD;
    else if (code == CMD_SEND_OP_COND)
        return r;
    else if (code == CMD_SEND_IF_COND)
        return r == arg ? SD_OK : SD_ERROR;
    else if (code == CMD_ALL_SEND_CID) {
        r |= EMMC->RESP3;
        r |= EMMC->RESP2;
        r |= EMMC->RESP1;
        return r;
    } else if (code == CMD_SEND_REL_ADDR) {
        sd_err = (((r & 0x1fff)) | ((r & 0x2000) << 6) | ((r & 0x4000) << 8) | ((r & 0x8000) << 8)) & CMD_ERRORS_MASK;
        return r & CMD_RCA_MASK;
    }
    return r & CMD_ERRORS_MASK;
    // make gcc happy
    return 0;
}

/*
 * Reads or writes a block of memory to the sd card
 */
int sd_transferblock(unsigned int lba, unsigned char *buffer, unsigned int num, readwrite_t readwrite) {
    int          r, c = 0, d;
    char         string[64];
    unsigned int cmd_single = readwrite == SD_READBLOCK ? CMD_READ_SINGLE : CMD_WRITE_SINGLE;
    unsigned int cmd_multi  = readwrite == SD_READBLOCK ? CMD_READ_MULTI : CMD_WRITE_MULTI;
    unsigned int mask       = readwrite == SD_READBLOCK ? INT_READ_RDY : INT_WRITE_RDY;
    if (num < 1)
        num = 1;
    if (sd_status(SR_DAT_INHIBIT)) {
        sd_err = SD_TIMEOUT;
        return 0;
    }
    unsigned int *buf = (unsigned int *)buffer;
    if (sd_scr[0] & SCR_SUPP_CCS) {
        if (num > 1 && (sd_scr[0] & SCR_SUPP_SET_BLKCNT)) {
            sd_cmd(CMD_SET_BLOCKCNT, num);
            if (sd_err) {
                LOG(ERROR, "Unable to set block count");
                return 0;
            }
        }
        EMMC->BLKSIZECNT = (num << 16) | 512;
        sd_cmd(num == 1 ? cmd_single : cmd_multi, lba);
        if (sd_err) {
            LOG(ERROR, "Unable to start operation");
            return 0;
        }
    } else {
        EMMC->BLKSIZECNT = (1 << 16) | 512;
    }
    while (c < num) {
        if (!(sd_scr[0] & SCR_SUPP_CCS)) {
            sd_cmd(cmd_single, (lba + c) * 512);
            if (sd_err) {
                strcpy(string, "Unable to carry on operation for block ");
                itoa(c, &string[strlen(string)], 10);
                strcpy(&string[strlen(string)], " of ");
                itoa(num, &string[strlen(string)], 10);
                LOG(WARN, string);
                return 0;
            }
        }
        if ((r = sd_int(mask))) {
            LOG(ERROR, "Timeout waiting for ready to read");
            sd_err = r;
            return 0;
        }
        for (d = 0; d < 128; d++) {
            if (readwrite == SD_READBLOCK)
                buf[d] = EMMC->DATA;
            else
                EMMC->DATA = buf[d];
        }
        c++;
        buf += 128;
    }
    if (num > 1 && !(sd_scr[0] & SCR_SUPP_SET_BLKCNT) && (sd_scr[0] & SCR_SUPP_CCS))
        sd_cmd(CMD_STOP_TRANS, 0);
    return sd_err != SD_OK || c != num ? 0 : num * 512;
}

/**
 * set SD clock to frequency in Hz
 */
int sd_clk(unsigned int f) {
    char         string[128];
    unsigned int d, c = 41666666 / f, x, s = 32, h = 0;
    int          cnt = 100000;
    while ((EMMC->STATUS & (SR_CMD_INHIBIT | SR_DAT_INHIBIT)) && cnt--)
        wait_msec(1);
    if (cnt <= 0) {
        LOG(ERROR, "timeout waiting for inhibit flag\n");
        return SD_ERROR;
    }

    EMMC->CONTROL1 &= ~C1_CLK_EN;
    wait_msec(10);
    x = c - 1;
    if (!x)
        s = 0;
    else {
        if (!(x & 0xffff0000u)) {
            x <<= 16;
            s -= 16;
        }
        if (!(x & 0xff000000u)) {
            x <<= 8;
            s -= 8;
        }
        if (!(x & 0xf0000000u)) {
            x <<= 4;
            s -= 4;
        }
        if (!(x & 0xc0000000u)) {
            x <<= 2;
            s -= 2;
        }
        if (!(x & 0x80000000u)) {
            x <<= 1;
            s -= 1;
        }
        if (s > 0)
            s--;
        if (s > 7)
            s = 7;
    }
    if (sd_hv > HOST_SPEC_V2)
        d = c;
    else
        d = (1 << s);
    if (d <= 2) {
        d = 2;
        s = 0;
    }
    strcpy(string, "sd_clk divisor ");
    itoa(d, &string[strlen(string)], 16);
    strcpy(&string[strlen(string)], ", shift ");
    itoa(s, &string[strlen(string)], 16);
    LOG(INFO, string);
    if (sd_hv > HOST_SPEC_V2)
        h = (d & 0x300) >> 2;
    d              = (((d & 0x0ff) << 8) | h);
    EMMC->CONTROL1 = (EMMC->CONTROL1 & 0xffff003f) | d;
    wait_msec(10);
    EMMC->CONTROL1 |= C1_CLK_EN;
    wait_msec(10);
    cnt = 10000;
    while (!(EMMC->CONTROL1 & C1_CLK_STABLE) && cnt--)
        wait_msec(10);
    if (cnt <= 0) {
        LOG(ERROR, "failed to get stable clock");
        return SD_ERROR;
    }
    return SD_OK;
}

/**
 * initialize EMMC to read SDHC card
 */
int sd_init() {
    char          string[128];
    unsigned long r;
    long          cnt, ccs = 0;
    // Setup the GPIOs that act as interface for the MMC
    // GPIO_CD
    setupGpio(47, GPIO_ALTFUNC3);
    setPullUpDown(47, GPIO_PULL_UP);
    setHighDetect(47);

    // GPIO_CLK, GPIO_CMD
    setupGpio(48, GPIO_ALTFUNC3);
    setupGpio(49, GPIO_ALTFUNC3);
    setPullUpDown(48, GPIO_PULL_UP);
    setPullUpDown(49, GPIO_PULL_UP);

    // GPIO_DAT0, GPIO_DAT1, GPIO_DAT2, GPIO_DAT3
    setupGpio(50, GPIO_ALTFUNC3);
    setupGpio(51, GPIO_ALTFUNC3);
    setupGpio(52, GPIO_ALTFUNC3);
    setupGpio(53, GPIO_ALTFUNC3);
    setPullUpDown(50, GPIO_PULL_UP);
    setPullUpDown(51, GPIO_PULL_UP);
    setPullUpDown(52, GPIO_PULL_UP);
    setPullUpDown(53, GPIO_PULL_UP);

    // Read host specification version number
    sd_hv = (EMMC->SLOTISR_VER & HOST_SPEC_NUM) >> HOST_SPEC_NUM_SHIFT;
    LOG(INFO, "EMMC: GPIO set up");
    // Reset the card.
    EMMC->CONTROL0 = 0;
    EMMC->CONTROL1 |= C1_SRST_HC;
    cnt = 10000;
    do {
        wait_msec(10);
    } while ((EMMC->CONTROL1 & C1_SRST_HC) && cnt--);
    if (cnt <= 0) {
        LOG(ERROR, "failed to reset EMMC");
        return SD_ERROR;
    }
    LOG(INFO, "EMMC: reset OK");
    EMMC->CONTROL1 |= C1_CLK_INTLEN | C1_TOUNIT_MAX;
    wait_msec(10);
    // Set clock to setup frequency.
    if ((r = sd_clk(400000)))
        return r;
    EMMC->IRPT_EN   = 0xffffffff;
    EMMC->IRPT_MASK = 0xffffffff;
    sd_scr[0] = sd_scr[1] = sd_rca = sd_err = 0;
    sd_cmd(CMD_GO_IDLE, 0);
    if (sd_err)
        return sd_err;

    sd_cmd(CMD_SEND_IF_COND, 0x000001AA);
    if (sd_err)
        return sd_err;
    cnt = 6;
    r   = 0;
    while (!(r & ACMD41_CMD_COMPLETE) && cnt--) {
        wait_cycles(400);
        r = sd_cmd(CMD_SEND_OP_COND, ACMD41_ARG_HC);
        strcpy(string, "EMMC: CMD_SEND_OP_COND returned ");
        if (r & ACMD41_CMD_COMPLETE)
            strcpy(&string[strlen(string)], "COMPLETE ");
        if (r & ACMD41_VOLTAGE)
            strcpy(&string[strlen(string)], "VOLTAGE ");
        if (r & ACMD41_CMD_CCS)
            strcpy(&string[strlen(string)], "CCS ");

        itoa(r, &string[strlen(string)], 16);
        LOG(INFO, string);
        if (sd_err != SD_TIMEOUT && sd_err != SD_OK) {
            LOG(ERROR, "EMMC ACMD41 returned error");
            return sd_err;
        }
    }
    if (!(r & ACMD41_CMD_COMPLETE) || !cnt)
        return SD_TIMEOUT;
    if (!(r & ACMD41_VOLTAGE))
        return SD_ERROR;
    if (r & ACMD41_CMD_CCS)
        ccs = SCR_SUPP_CCS;

    sd_cmd(CMD_ALL_SEND_CID, 0);

    sd_rca = sd_cmd(CMD_SEND_REL_ADDR, 0);
    strcpy(string, "EMMC: CMD_SEND_REL_ADDR returned ");
    itoa(sd_rca, &string[strlen(string)], 16);
    LOG(INFO, string);
    if (sd_err)
        return sd_err;

    if ((r = sd_clk(25000000)))
        return r;

    sd_cmd(CMD_CARD_SELECT, sd_rca);
    if (sd_err)
        return sd_err;

    if (sd_status(SR_DAT_INHIBIT))
        return SD_TIMEOUT;
    EMMC->BLKSIZECNT = (1 << 16) | 8;
    sd_cmd(CMD_SEND_SCR, 0);
    if (sd_err)
        return sd_err;
    if (sd_int(INT_READ_RDY))
        return SD_TIMEOUT;

    r   = 0;
    cnt = 100000;
    while (r < 2 && cnt) {
        if (EMMC->STATUS & SR_READ_AVAILABLE)
            sd_scr[r++] = EMMC->DATA;
        else
            wait_msec(1);
    }
    if (r != 2)
        return SD_TIMEOUT;
    if (sd_scr[0] & SCR_SD_BUS_WIDTH_4) {
        sd_cmd(CMD_SET_BUS_WIDTH, sd_rca | 2);
        if (sd_err)
            return sd_err;
        EMMC->CONTROL0 |= C0_HCTL_DWITDH;
    }
    // add software flag
    strcpy(string, "EMMC: supports ");
    if (sd_scr[0] & SCR_SUPP_SET_BLKCNT)
        strcpy(&string[strlen(string)], "SET_BLKCNT ");
    if (ccs)
        strcpy(&string[strlen(string)], "CCS ");
    LOG(INFO, string);
    sd_scr[0] &= ~SCR_SUPP_CCS;
    sd_scr[0] |= ccs;
    return SD_OK;
}
