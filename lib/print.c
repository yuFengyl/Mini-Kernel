#include "print.h"
#include "sbi.h"

void puts(char *s) {
    // unimplemented
	int i;
	for (i=0; s[i+1]!='\0'; i++)
	{
		sbi_ecall(0x1, 0x0, s[i], 0, 0, 0, 0, 0);
	}
	return ;
}

void puti(int x) {
    // unimplemented
	int i;
	int a[100];
	if (x<0)
	{
		sbi_ecall(0x1, 0x0, 0x2d, 0, 0, 0, 0, 0);
		x = -x;
	}
	while (1)
	{
		a[i] = x % 10;
		x = x/10;
		i++;
		if (x == 0)
			break;
	}
	int j;
	for (j=i-1; j>=0; j--)
		sbi_ecall(0x1, 0x0, a[j]+0x30, 0, 0, 0, 0, 0);
	return ;
}
