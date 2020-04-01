#include <stdio.h>

int main()
{
	FILE *fp = NULL;
	char buf[512];
	size_t s;

	fp = fopen("/dev/ccmaa", "w");
	if (fp == NULL) {
		printf("open file err.\n");
		return -1;
	}

	//s = fread(buf, 1, 512, fp);
	//printf("%d bytes read.\n", s);

	buf[0] = 'a';
	buf[1] = 'b';
	s = fwrite(buf, 1, 512, fp);
	printf("%d bytes written.\n", s);

	fsync(fp);

	fclose(fp);
	return 0;
}
