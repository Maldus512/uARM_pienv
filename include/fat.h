#include "sd.h"

int          fat_getpartition(void);
unsigned int fat_getcluster(char *fn);
unsigned int fat_transferfile(unsigned int cluster, unsigned char *buffer, unsigned int num, readwrite_t readwrite);
