#ifndef __LIBUARMV2_H__
#define __LIBUARMV2_H__

/* Exception cause codes */
#define EC_SVC 0x15     // 010101

/* System call codes */
#define SYS_GETARMCLKFRQ 0x01
#define SYS_GETARMCOUNTER 0x02
#define SYS_ENABLEIRQ 0x03
#define SYS_GETCURRENTEL 0x04
#define SYS_INITARMTIMER 0x05
#define SYS_GETSPEL0 0x06
#define SYS_LAUNCHSTATE 0x07
#define SYS_SETNEXTTIMER 0x08
#define SYS_INITMMU 0x09
#define SYS_GETCURRENTSTATUS 0x0A
#define SYS_GETTTBR0 0x0B


#endif