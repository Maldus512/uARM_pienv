#include "arch.h"
#include "mailbox.h"

volatile unsigned int __attribute__((aligned(16))) mbox[36];


void wait_mailbox_write(Mailbox *m) {
    while (m->status & MBOX_FULL)
        nop();
}

void wait_mailbox_read(Mailbox *m) {
    while (m->status & MBOX_EMPTY)
        nop();
}


void writeMailbox0(uint32_t *data, uint8_t channel) {
    wait_mailbox_write(MAILBOX0);
    MAILBOX0->write = ((uint32_t)((uint64_t)data) & ~0xf) | (uint32_t)(channel & 0xf);
}

void readMailbox0(uint8_t channel) {
    uint32_t res;
    uint8_t  read_channel;
    wait_mailbox_read(MAILBOX0);

    do {
        res          = MAILBOX0->read;
        read_channel = res & 0xf;
    } while (read_channel != channel);
}

void led(uint32_t onoff) {
    struct mailbox_msg msg;
    msg.msg_size        = sizeof(struct mailbox_msg);
    msg.request_code    = MBOX_REQUEST;
    msg.tag.tag_id      = 0x00038041;
    msg.tag.buffer_size = 0x8;
    msg.tag.data_size   = 0x0;
    msg.tag.dev_id      = 130;
    msg.tag.val         = onoff;
    msg.end_tag         = MBOX_TAG_LAST;

    writeMailbox0((uint32_t *)&msg, MBOX_CH_PROP);
    readMailbox0(MBOX_CH_PROP);
}

void serialNumber(uint32_t serial[2]) {
    struct mailbox_msg msg;
    msg.msg_size        = sizeof(struct mailbox_msg);
    msg.request_code    = MBOX_REQUEST;
    msg.tag.tag_id      = MBOX_TAG_GETSERIAL;
    msg.tag.buffer_size = 0x8;
    msg.tag.data_size   = 0x0;
    msg.tag.dev_id      = 0;
    msg.tag.val         = 0;
    msg.end_tag         = MBOX_TAG_LAST;

    writeMailbox0((uint32_t *)&msg, MBOX_CH_PROP);
    readMailbox0(MBOX_CH_PROP);
    serial[0] = msg.tag.dev_id;
    serial[1] = msg.tag.val;
}

void setUart0Baud() {
    struct mailbox_msg msg;
    /* set up clock for consistent divisor values */
    msg.msg_size        = sizeof(struct mailbox_msg);
    msg.request_code    = MBOX_REQUEST;
    msg.tag.tag_id      = MBOX_TAG_SETCLKRATE;
    msg.tag.buffer_size = 12;
    msg.tag.data_size   = 8;
    msg.tag.dev_id      = 2;
    msg.tag.val         = 4000000;
    msg.end_tag         = MBOX_TAG_LAST;

    writeMailbox0((uint32_t *)&msg, MBOX_CH_PROP);
    readMailbox0(MBOX_CH_PROP);
}

unsigned int getMemorySplit() {
    struct mailbox_msg msg;
    msg.msg_size        = sizeof(struct mailbox_msg);
    msg.request_code    = MBOX_REQUEST;
    msg.tag.tag_id      = MBOX_TAG_MEMSPLIT;
    msg.tag.buffer_size = 0x8;
    msg.tag.data_size   = 0x0;
    msg.tag.dev_id      = 0;
    msg.tag.val         = 0;
    msg.end_tag         = MBOX_TAG_LAST;

    writeMailbox0((uint32_t *)&msg, MBOX_CH_PROP);
    readMailbox0(MBOX_CH_PROP);

    return msg.tag.val;
}

int mbox_call(unsigned char ch) {
    unsigned int r = (((unsigned int)((unsigned long)&mbox) & ~0xF) | (ch & 0xF));
    /* wait until we can write to the mailbox */
    wait_mailbox_write(MAILBOX0);
    /* write the address of our message to the mailbox with channel identifier */
    MAILBOX0->write = r;
    /* now wait for the response */
    while (1) {
        /* is there a response? */
        wait_mailbox_read(MAILBOX0);
        /* is it a response to our message? */
        if (r == MAILBOX0->read)
            /* is it a valid successful response? */
            return mbox[1] == MBOX_RESPONSE;
    }
    return 0;
}

void initIPI() {
    *((uint32_t *)CORE0_MBOX_INTERRUPT_CONTROL) = 1;
    *((uint32_t *)CORE1_MBOX_INTERRUPT_CONTROL) = 1;
    *((uint32_t *)CORE2_MBOX_INTERRUPT_CONTROL) = 1;
    *((uint32_t *)CORE3_MBOX_INTERRUPT_CONTROL) = 1;
}