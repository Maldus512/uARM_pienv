#ifndef __UTILS_H__
#define __UTILS_H__

void  memcpy(void *dest, void *src, int n);
int   memcmp(unsigned char *str1, unsigned char *str2, int count);
char *itoa(long unsigned int value, char *result, int base);
int strlen(char *str);
int strcpy(char *dest, char *src);

#endif