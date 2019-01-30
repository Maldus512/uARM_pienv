#include "sd.h"

int          fat_getpartition(void);
unsigned int fat_getcluster(char *fn);
unsigned int fat_transferfile(unsigned int cluster, unsigned char *buffer, unsigned int num, readwrite_t readwrite);
void         fat_listdirectory(void);
int          fat_readfile(unsigned int cluster, unsigned char *data, unsigned int seek, unsigned int length);
int          fat_writefile(unsigned int cluster, unsigned char *data, unsigned int seek, unsigned int length);
